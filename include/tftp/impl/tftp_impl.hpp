/* Copyright (C) 2025 Kevin Exton (kevin.exton@pm.me)
 *
 * tftpd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tftpd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tftpd.  If not, see <https://www.gnu.org/licenses/>.
 */
/**
 * @file tftp_impl.hpp
 * @brief This file defines TFTP application logic.
 */
#pragma once
#ifndef TFTP_IMPL_HPP
#define TFTP_IMPL_HPP
#include "tftp/detail/dns.hpp"
#include "tftp/tftp.hpp"

#include <spdlog/spdlog.h>
/** @brief TFTP client namespace. */
namespace tftp::client {

namespace detail {
static inline auto
get_error_message(const messages::error *error,
                  std::streamsize len) noexcept -> std::string_view
{
  const char *begin = reinterpret_cast<const char *>(error) + sizeof(*error);
  const char *end = begin + len - sizeof(*error);
  if (std::find(begin, end, '\0') == end)
    return "";

  return begin;
}

template <typename Receiver, typename Fn>
  requires std::invocable<Fn>
auto try_with(Receiver &&receiver, Fn &&handler) noexcept -> void
{
  using namespace stdexec;
  try
  {
    std::forward<Fn>(handler)();
  }
  catch (const std::bad_alloc &)
  {
    set_error(std::forward<Receiver>(receiver),
              std::make_error_code(std::errc::not_enough_memory));
  }
  catch (...)
  {
    set_error(std::forward<Receiver>(receiver),
              std::make_error_code(std::errc::state_not_recoverable));
  }
}
} // namespace detail

template <typename Receiver>
auto connect_t::state_t<Receiver>::start() noexcept -> void
{
  using namespace stdexec;

  // Resolve IPv4 address unless otherwise configured.
  struct addrinfo hints = {.ai_family = AF_INET,
                           .ai_socktype = SOCK_DGRAM,
                           .ai_protocol = IPPROTO_UDP};

  struct addrinfo *result = nullptr;
  auto err = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
  if (err)
  {
    return set_error(std::move(receiver),
                     make_error_code(static_cast<dns::errc>(err)));
  }

  if (result == nullptr)
  {
    return set_error(std::move(receiver),
                     make_error_code(dns::errc::address_not_found));
  }

  socket_address<sockaddr_in6> address =
      socket_address<sockaddr_in>(result->ai_addr, result->ai_addrlen);
  freeaddrinfo(result);
  result = nullptr;

  set_value(std::move(receiver), std::move(address));
}

template <typename Receiver>
auto connect_t::connect(Receiver &&receiver) noexcept -> state_t<Receiver>
{
  return {.hostname = std::move(hostname),
          .port = std::move(port),
          .receiver = std::forward<Receiver>(receiver),
          .ctx = ctx};
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::start() noexcept -> void
{
  send_wrq();
  submit_recvmsg();
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::send_wrq() noexcept -> void
{
  using namespace detail;

  try_with(std::move(receiver), [&] {
    auto &state = session.state;
    auto &buffer = state.buffer;
    auto opcode = htons(state.opc = messages::WRQ);

    buffer.resize(sizeof(opcode));

    auto *msg = buffer.data();
    std::memcpy(msg, &opcode, sizeof(opcode));

    const auto *begin = state.target.c_str();
    const auto *end = begin + std::strlen(begin) + 1;
    buffer.insert(buffer.end(), begin, end);

    begin = messages::mode_to_str(state.mode);
    end = begin + std::strlen(begin) + 1;
    buffer.insert(buffer.end(), begin, end);

    submit_sendmsg();
  });
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::submit_sendmsg() noexcept -> void
{
  using namespace stdexec;
  using namespace detail;
  auto &state = session.state;

  try_with(std::move(receiver), [&] {
    sender auto sendmsg =
        io::sendmsg(
            socket,
            socket_message{.address = sockmsg.address, .buffers = state.buffer},
            0) |
        then([&](auto &&len) noexcept {
          using namespace std::chrono;

          const auto &[_, avg_rtt] =
              session_t::update_statistics(state.statistics);

          try_with(std::move(receiver), [&] {
            auto &timer = state.timer = ctx->timers.remove(state.timer);
            timer = ctx->timers.add(
                2 * avg_rtt,
                [&, retries = 0](auto &&tid) mutable {
                  constexpr auto MAX_RETRIES = 5;
                  try_with(std::move(receiver), [&] {
                    if (retries++ >= MAX_RETRIES)
                    {
                      cleanup();
                      return set_value(std::move(receiver),
                                       status_t{0, "Timed out"});
                    }
                    submit_sendmsg();
                  });
                },
                2 * avg_rtt);
          });
        }) |
        upon_error([&](auto &&error) noexcept {
          cleanup();
          set_error(std::move(receiver), std::forward<decltype(error)>(error));
        });
    ctx->scope.spawn(std::move(sendmsg));
  });
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::submit_recvmsg() noexcept -> void
{
  using namespace stdexec;
  using namespace detail;
  try_with(std::move(receiver), [&] {
    sockmsg.buffers = recv_buffer;
    sender auto recvmsg =
        io::recvmsg(socket, sockmsg, 0) | then([&](auto &&len) noexcept {
          if (len < static_cast<std::streamsize>(sizeof(std::uint16_t)) ||
              sockmsg.flags & MSG_TRUNC)
          {
            cleanup();
            return set_value(std::move(receiver),
                             status_t{messages::ILLEGAL_OPERATION,
                                      "Invalid server response."});
          }

          try_with(std::move(receiver), [&] {
            auto *msg = reinterpret_cast<std::uint16_t *>(recv_buffer.data());
            if (ntohs(*msg) == messages::ERROR)
            {
              cleanup();
              auto *errmsg = reinterpret_cast<messages::error *>(msg);
              auto error_code = ntohs(errmsg->error);
              auto error_message = get_error_message(errmsg, len);

              return set_value(std::move(receiver),
                               status_t{error_code, error_message});
            }

            if (ntohs(*msg) == messages::ACK)
            {
              auto *ackmsg = reinterpret_cast<messages::ack *>(msg);
              auto error = handle_ack(*ackmsg, std::addressof(session));
              if (error || !session.state.file->is_open())
              {
                cleanup();
                if (!error)
                  return set_value(std::move(receiver), status_t{});

                return set_value(std::move(receiver), status_t{error, ""});
              }

              submit_sendmsg();
            }

            submit_recvmsg();
          });
        }) |
        upon_error([&](auto &&error) noexcept {
          cleanup();
          set_error(std::move(receiver), std::forward<decltype(error)>(error));
        });
    ctx->scope.spawn(std::move(recvmsg));
  });
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::cleanup() noexcept -> void
{
  auto &timer = session.state.timer;
  timer = ctx->timers.remove(timer);
}

template <typename Receiver>
auto put_file_t::connect(Receiver &&receiver) -> state_t<Receiver>
{
  auto buffer = std::vector<char>();
  buffer.reserve(messages::DATAMSG_MAXLEN + messages::DATALEN);

  auto session =
      session_t{.state = {.target = std::move(remote),
                          .buffer = std::move(buffer),
                          .file = std::make_shared<std::fstream>(
                              local, std::ios::in | std::ios::binary),
                          .mode = mode}};

  auto recvbuf = std::vector<char>();
  recvbuf.resize(messages::DATAMSG_MAXLEN);

  return {.session = std::move(session),
          .sockmsg = socket_message{.address = {server_addr}},
          .recv_buffer = std::move(recvbuf),
          .socket = ctx->poller.emplace(AF_INET, SOCK_DGRAM, IPPROTO_UDP),
          .receiver = std::forward<Receiver>(receiver),
          .ctx = ctx};
}
} // namespace tftp::client.
#endif // TFTP_IMPL_HPP
