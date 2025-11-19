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
 * @file tftp.hpp
 * @brief This file declares TFTP application logic.
 */
#pragma once
#ifndef TFTP_HPP
#define TFTP_HPP
#include "protocol/tftp_protocol.hpp"
#include "protocol/tftp_session.hpp"

#include <net/cppnet.hpp>

#include <map>
#include <netdb.h>
/** @namespace For top-level tftp services. */
namespace tftp {
/** @brief The TFTP sessions container. */
using sessions_t =
    std::multimap<io::socket::socket_address<sockaddr_in6>, session_t>;
/** @brief The TFTP sessions iterator. */
using iterator_t = sessions_t::iterator;

/**
 * @brief Processes a request.
 * @param req The TFTP request to process.
 * @param siter An iterator pointing to the session.
 * @returns 0 if successful, a non-zero TFTP error otherwise.
 */
auto handle_request(messages::request req, iterator_t siter) -> std::uint16_t;

/**
 * @brief Processes an ack message.
 * @param ack The TFTP ack to process.
 * @param siter An iterator pointing to the session.
 * @returns 0 if successful, a non-zero TFTP error otherwise.
 */
auto handle_ack(messages::ack ack, iterator_t siter) -> std::uint16_t;

/**
 * @brief Processes a data message.
 * @param data A pointer to the beginning of the TFTP data frame.
 * @param len The length of the data frame including the TFTP header.
 * @param siter An iterator pointing to the session.
 * @returns 0 if successful, a non-zero TFTP error otherwise.
 */
auto handle_data(const messages::data *data, std::size_t len,
                 iterator_t siter) -> std::uint16_t;

/** @brief TFTP client namespace. */
namespace client {
/** @brief async context type. */
using async_context = net::service::async_context;
/** @brief socket address type. */
template <typename T> using socket_address = io::socket::socket_address<T>;

/** @brief The sender for an asynchronous connect. */
struct connect_t {
  /** @brief sender concept. */
  using sender_concept = stdexec::sender_t;
  /** @brief Sender completion signature. */
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(
                                         socket_address<sockaddr_in6>),
                                     stdexec::set_error_t(std::error_code)>;

  /** @brief sender operation state. */
  template <typename Receiver> struct state_t {
    /** @brief Initiate the asynchronous connect. */
    auto start() noexcept -> void;

    /** @brief hostname to connect to. */
    std::string hostname;
    /** @brief port to connect to. */
    std::string port;
    /** @brief The receiver to send the final value to. */
    Receiver receiver;
    /** @brief The asynchronous context. */
    async_context *ctx = nullptr;
  };

  /**
   * @brief Connect this sender to a receiver.
   * @tparam Receiver receiver type.
   * @param receiver The receiver to connnect to.
   * @returns An operation state for receiver.
   */
  template <typename Receiver>
  auto connect(Receiver &&receiver) noexcept -> state_t<Receiver>;

  /** @brief hostname to connect to. */
  std::string hostname;
  /** @brief port to connect to. */
  std::string port;
  /** @brief The asynchronous context. */
  async_context *ctx = nullptr;
};
} // namespace client.
} // namespace tftp

#include "impl/tftp_impl.hpp" // IWYU pragma: export

#endif // TFTP_HPP
