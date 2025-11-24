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
#include <filesystem>
#ifndef TFTP_IMPL_HPP
#define TFTP_IMPL_HPP
#include "tftp/detail/dns.hpp"
#include "tftp/tftp.hpp"

#include <spdlog/spdlog.h>
/** @brief TFTP client namespace. */
namespace tftp::client {
/** @brief Internal client implementation details. */
namespace detail {
/**
 * @brief Validates and extracts message string from a TFTP error message.
 * @param error A pointer to a TFTP error message.
 * @param len The length of the TFTP error message.
 * @returns An empty string_view if the message is invalid (not null
 * terminated), a string_view of the message string otherwise.
 */
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

/**
 * @brief Handles exceptions raised by the `handler` callback.
 * @details try_with attempts to run the handler. If the handler raises
 * an exception, try_with will run the cleanup method and forward an
 * appropriate error to the receiver.
 * @tparam Receiver The receiver type.
 * @tparam Fn The handler type. Fn is invoked with no arguments.
 * @tparam Cleanup The cleanup callback.
 * @param receiver The receiver to forward errors to.
 * @param handler The handler to attempt.
 * @param cleanup The handler to run before forwarding the error.
 */
template <typename Receiver, typename Fn, typename Cleanup>
  requires std::invocable<Fn> && std::invocable<Cleanup>
auto try_with(Receiver &&receiver, Fn &&handler,
              Cleanup &&cleanup) noexcept -> void
{
  using namespace stdexec;
  try
  {
    std::forward<Fn>(handler)();
  }
  catch (const std::bad_alloc &)
  {
    std::forward<Cleanup>(cleanup)();
    set_error(std::forward<Receiver>(receiver),
              std::make_error_code(std::errc::not_enough_memory));
  }
  catch (...)
  {
    std::forward<Cleanup>(cleanup)();
    set_error(std::forward<Receiver>(receiver),
              std::make_error_code(std::errc::state_not_recoverable));
  }
}
} // namespace detail

template <typename Receiver>
auto client_sender::client_state<Receiver>::error_handler(
    const char *msg, std::streamsize len) noexcept -> void
{
  using namespace detail;

  const auto *err = reinterpret_cast<const messages::error *>(msg);
  auto code = ntohs(err->error);
  auto message = get_error_message(err, len);

  try_with(
      std::move(receiver), [&] { this->finalize(status_t{code, message}); },
      [&]() noexcept { this->cleanup(); });
}

template <typename Receiver>
auto client_sender::client_state<Receiver>::cleanup() noexcept -> void
{
  auto &timer = session.state.timer;
  timer = ctx->timers.remove(timer);

  auto &tmpfile = session.state.tmp;
  if (!tmpfile.empty())
  {
    auto err = std::error_code();
    std::filesystem::remove(tmpfile, err);
  }
}

template <typename Receiver>
auto client_sender::client_state<Receiver>::finalize(status_t status) noexcept
    -> void
{
  using namespace stdexec;
  cleanup();

  set_value(std::move(receiver), std::move(status));
}

template <typename Receiver>
auto client_sender::client_state<Receiver>::finalize(
    std::error_code error) noexcept -> void
{
  using namespace stdexec;
  cleanup();

  set_error(std::move(receiver), std::move(error));
}

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
          .receiver = std::forward<Receiver>(receiver)};
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::start() noexcept -> void
{
  submit_recvmsg();
  send_wrq();
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::send_wrq() noexcept -> void
{
  using namespace detail;
  auto &receiver = this->receiver;
  auto &state = this->session.state;
  auto &buffer = state.buffer;

  try_with(
      std::move(receiver),
      [&] {
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
      },
      [&]() noexcept { this->cleanup(); });
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::ack_handler(messages::ack ack) noexcept
    -> void
{
  using namespace detail;
  auto &session = this->session;
  auto &receiver = this->receiver;

  try_with(
      std::move(receiver),
      [&] {
        auto error = handle_ack(ack, std::addressof(session));
        if (error || !session.state.file->is_open())
          return this->finalize({error, ""});

        submit_recvmsg();
        submit_sendmsg();
      },
      [&]() noexcept { this->cleanup(); });
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::submit_sendmsg() noexcept -> void
{
  using namespace stdexec;
  using namespace detail;
  auto &ctx = this->ctx;
  auto &receiver = this->receiver;
  auto &state = this->session.state;
  auto &sockmsg = this->sockmsg;
  auto &socket = this->socket;

  const auto &[_, avg_rtt] = session_t::update_statistics(state.statistics);
  auto &timer = state.timer = ctx->timers.remove(state.timer);

  try_with(
      std::move(receiver),
      [&] {
        sender auto sendmsg =
            io::sendmsg(socket,
                        socket_message{.address = sockmsg.address,
                                       .buffers = state.buffer},
                        0) |
            then([](auto) noexcept {}) | upon_error([&](int error) noexcept {
              this->finalize(std::error_code(error, std::system_category()));
            });

        ctx->scope.spawn(std::move(sendmsg));

        // timers.add notifies the ctx event-loop by calling ctx.interrupt().
        timer = ctx->timers.add(
            2 * avg_rtt,
            [&, retries = 0](auto) mutable noexcept {
              constexpr auto MAX_RETRIES = 5;
              try_with(
                  std::move(receiver),
                  [&] {
                    if (retries++ >= MAX_RETRIES)
                      return this->finalize({0, "Timed out"});

                    submit_sendmsg();
                  },
                  [&]() noexcept { this->cleanup(); });
            },
            2 * avg_rtt);
      },
      [&]() noexcept { this->cleanup(); });
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::submit_recvmsg() noexcept -> void
{
  using namespace stdexec;
  using namespace detail;
  auto &ctx = this->ctx;
  auto &receiver = this->receiver;
  auto &sockmsg = this->sockmsg;
  auto &socket = this->socket;
  auto &recv_buffer = this->recv_buffer;

  try_with(
      std::move(receiver),
      [&] {
        sockmsg.buffers = recv_buffer;
        sender auto recvmsg =
            io::recvmsg(socket, sockmsg, 0) | then([&](auto &&len) noexcept {
              try_with(
                  std::move(receiver),
                  [&] {
                    if (len < static_cast<std::streamsize>(
                                  sizeof(std::uint16_t)) ||
                        sockmsg.flags & MSG_TRUNC)
                    {
                      return this->finalize({messages::ILLEGAL_OPERATION,
                                             "Invalid server response."});
                    }

                    route_message(recv_buffer.data(), len);
                  },
                  [&]() noexcept { this->cleanup(); });
            }) |
            upon_error([&](int error) noexcept {
              this->finalize(std::error_code(error, std::system_category()));
            });
        ctx->scope.spawn(std::move(recvmsg));
      },
      [&]() noexcept { this->cleanup(); });
}

template <typename Receiver>
auto put_file_t::state_t<Receiver>::route_message(
    const char *msg, std::streamsize len) noexcept -> void
{
  const auto *ack = reinterpret_cast<const messages::ack *>(msg);
  switch (ntohs(ack->opc))
  {
    case messages::ERROR:
      return this->error_handler(msg, len);

    case messages::ACK:
      return ack_handler(*ack);

    default: // non-ack/error messages are ignored.
      submit_recvmsg();
  }
}

template <typename Receiver>
auto put_file_t::connect(Receiver &&receiver) -> state_t<Receiver>
{
  auto session =
      session_t{.state = {.target = std::move(remote),
                          .file = std::make_shared<std::fstream>(
                              local, std::ios::in | std::ios::binary),
                          .mode = mode}};

  return {{.session = std::move(session),
           .sockmsg = socket_message{.address = {server_addr}},
           .local = std::move(local),
           .socket = ctx->poller.emplace(AF_INET, SOCK_DGRAM, IPPROTO_UDP),
           .receiver = std::forward<Receiver>(receiver),
           .ctx = ctx}};
}

template <typename Receiver>
auto get_file_t::state_t<Receiver>::start() noexcept -> void
{
  submit_recvmsg();
  send_rrq();
}

template <typename Receiver>
auto get_file_t::state_t<Receiver>::send_rrq() noexcept -> void
{
  using namespace detail;
  auto &receiver = this->receiver;
  auto &state = this->session.state;
  auto &buffer = state.buffer;

  try_with(
      std::move(receiver),
      [&] {
        auto opcode = htons(state.opc = messages::RRQ);

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
      },
      [&]() noexcept { this->cleanup(); });
}

template <typename Receiver>
auto get_file_t::state_t<Receiver>::data_handler(
    const char *msg, std::streamsize len) noexcept -> void
{
  using namespace detail;
  auto &session = this->session;
  auto &receiver = this->receiver;
  auto &block_num = session.state.block_num;
  auto &buffer = session.state.buffer;

  try_with(
      std::move(receiver),
      [&] {
        const auto *data = reinterpret_cast<const messages::ack *>(msg);
        auto error = handle_data(data, len, std::addressof(session));
        if (error)
          return this->finalize({error, ""});

        if (ntohs(data->block_num) == block_num)
        {
          buffer.resize(sizeof(messages::ack));

          auto *ack = reinterpret_cast<messages::ack *>(buffer.data());
          ack->opc = htons(messages::ACK);
          ack->block_num = data->block_num;

          submit_sendmsg();
        }

        if (!session.state.file->is_open())
        {
          auto err = std::error_code();
          std::filesystem::rename(session.state.tmp, this->local, err);
          if (err)
            return this->finalize(err);

          return this->finalize();
        }

        submit_recvmsg();
      },
      [&]() noexcept { this->cleanup(); });
}

template <typename Receiver>
auto get_file_t::state_t<Receiver>::submit_sendmsg() noexcept -> void
{
  using namespace stdexec;
  using namespace detail;
  auto &receiver = this->receiver;
  auto &sockmsg = this->sockmsg;
  auto &socket = this->socket;
  auto &state = this->session.state;
  auto &file = state.file;
  auto &ctx = this->ctx;

  const auto &[_, avg_rtt] = session_t::update_statistics(state.statistics);
  auto &timer = state.timer = ctx->timers.remove(state.timer);

  try_with(
      std::move(receiver),
      [&] {
        sender auto sendmsg =
            io::sendmsg(socket,
                        socket_message{.address = sockmsg.address,
                                       .buffers = state.buffer},
                        0) |
            then([](auto) noexcept {}) | upon_error([&](int error) noexcept {
              this->finalize(std::error_code(error, std::system_category()));
            });

        ctx->scope.spawn(std::move(sendmsg));

        // timers.add notifies the ctx event-loop by calling ctx.interrupt().
        timer = ctx->timers.add(
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            5 * avg_rtt, [&](auto) noexcept {
              try_with(
                  std::move(receiver),
                  [&] {
                    if (file->is_open())
                      return this->finalize({0, "Timed out"});

                    this->finalize();
                  },
                  [&]() noexcept { this->cleanup(); });
            });
      },
      [&]() noexcept { this->cleanup(); });
}

template <typename Receiver>
auto get_file_t::state_t<Receiver>::submit_recvmsg() noexcept -> void
{
  using namespace stdexec;
  using namespace detail;
  auto &receiver = this->receiver;
  auto &sockmsg = this->sockmsg;
  auto &socket = this->socket;
  auto &recv_buffer = this->recv_buffer;
  auto &ctx = this->ctx;

  try_with(
      std::move(receiver),
      [&] {
        sockmsg.buffers = recv_buffer;
        sender auto recvmsg =
            io::recvmsg(socket, sockmsg, 0) | then([&](auto &&len) noexcept {
              try_with(
                  std::move(receiver),
                  [&] {
                    if (len < static_cast<std::streamsize>(
                                  sizeof(std::uint16_t)) ||
                        sockmsg.flags & MSG_TRUNC)
                    {
                      return this->finalize({messages::ILLEGAL_OPERATION,
                                             "Invalid server response."});
                    }

                    route_message(recv_buffer.data(), len);
                  },
                  [&]() noexcept { this->cleanup(); });
            }) |
            upon_error([&](int error) noexcept {
              this->finalize(std::error_code(error, std::system_category()));
            });
        ctx->scope.spawn(std::move(recvmsg));
      },
      [&]() noexcept { this->cleanup(); });
}

template <typename Receiver>
auto get_file_t::state_t<Receiver>::route_message(
    const char *msg, std::streamsize len) noexcept -> void
{
  const auto *opc = reinterpret_cast<const std::uint16_t *>(msg);
  switch (ntohs(*opc))
  {
    case messages::ERROR:
      return this->error_handler(msg, len);

    case messages::DATA:
      return data_handler(msg, len);

    default: // non-data/error messages are ignored.
      submit_recvmsg();
  }
}

template <typename Receiver>
auto get_file_t::connect(Receiver &&receiver) -> state_t<Receiver>
{
  if (mode == messages::MAIL)
    throw std::invalid_argument("Mail mode is not allowed in read requests.");

  auto tmpfile = std::filesystem::temp_directory_path() / local.filename();
  auto file = std::make_shared<std::fstream>(
      tmpfile, std::ios::out | std::ios::trunc | std::ios::binary);

  auto session = session_t{.state = {.target = std::move(remote),
                                     .tmp = std::move(tmpfile),
                                     .file = std::move(file),
                                     .mode = mode}};

  return {{.session = std::move(session),
           .sockmsg = socket_message{.address = {server_addr}},
           .local = std::move(local),
           .socket = ctx->poller.emplace(AF_INET, SOCK_DGRAM, IPPROTO_UDP),
           .receiver = std::forward<Receiver>(receiver),
           .ctx = ctx}};
}
} // namespace tftp::client.
#endif // TFTP_IMPL_HPP
