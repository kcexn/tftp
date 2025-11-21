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
#include <io/execution/poll_multiplexer.hpp>
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
auto handle_request(messages::request req, session_t *siter) -> std::uint16_t;

/**
 * @brief Processes an ack message.
 * @param ack The TFTP ack to process.
 * @param siter An iterator pointing to the session.
 * @returns 0 if successful, a non-zero TFTP error otherwise.
 */
auto handle_ack(messages::ack ack, session_t *siter) -> std::uint16_t;

/**
 * @brief Processes a data message.
 * @param data A pointer to the beginning of the TFTP data frame.
 * @param len The length of the data frame including the TFTP header.
 * @param siter An iterator pointing to the session.
 * @returns 0 if successful, a non-zero TFTP error otherwise.
 */
auto handle_data(const messages::data *data, std::size_t len,
                 session_t *siter) -> std::uint16_t;

/** @brief TFTP client namespace. */
namespace client {
/** @brief async context type. */
using async_context = net::service::async_context;
/** @brief socket address type. */
template <typename T> using socket_address = io::socket::socket_address<T>;
/** @brief The asynchronous client socket. */
using socket_dialog = async_context::socket_dialog;
/** @brief The socket message type. */
using socket_message = io::socket::socket_message<sockaddr_in6>;

/** @brief common elements for all tftp client senders. */
struct client_sender {
  /**
   * @brief tftp status response.
   * @details tftp status is an error code followed by an error message,
   * an error code of 0 AND an empty string indicates an OK response.
   */
  using status_t = std::pair<std::uint16_t, std::string>;
  /** @brief sender concept. */
  using sender_concept = stdexec::sender_t;
  /** @brief completion signature set value types. */
  using set_value_t = stdexec::set_value_t;
  /** @brief set error types. */
  using set_error_t = stdexec::set_error_t;

  /** @brief common elements for all tftp client operation states. */
  template <typename Receiver> struct client_state {
    /** @brief The receive buffer. */
    std::array<char, messages::DATAMSG_MAXLEN> recv_buffer{};
    /** @brief The tftp client session details */
    session_t session;
    /** @brief The socket message type. */
    socket_message sockmsg;
    /** @brief The client socket. */
    socket_dialog socket;
    /** @brief The operation receiver. */
    Receiver receiver;
    /** @brief The asynchronous context. */
    async_context *ctx = nullptr;

    /**
     * @brief handle error messages
     * @param msg The error message.
     * @param len The length of the error message.
     */
    auto error_handler(const char *msg, std::streamsize len) noexcept -> void;

    /** @brief cleanup any lingering session state. */
    auto cleanup() noexcept -> void;

    /**
     * @brief Finalize the client session with a tftp status code.
     * @param status The status message to finalize with. (default: OK)
     */
    auto finalize(status_t status = {}) noexcept -> void;

    /**
     * @brief Finalize the client session with a system error.
     * @param error The error code to finalize with.
     */
    auto finalize(std::error_code error) noexcept -> void;
  };

  // common client sender members.
  /** @brief address of the tftp server. */
  socket_address<sockaddr_in6> server_addr;
  /** @brief local file path to send. */
  std::filesystem::path local;
  /** @brief remote file path to write to. */
  std::filesystem::path remote;
  /** @brief The asynchronous context. */
  async_context *ctx = nullptr;
  /** @brief The tftp transmission mode. */
  std::uint8_t mode = 0;
};

/** @brief The sender for an asynchronous connect. */
struct connect_t {
  /** @brief sender concept. */
  using sender_concept = stdexec::sender_t;
  /** @brief completion signature set value types. */
  using set_value_t = stdexec::set_value_t;
  /** @brief set error types. */
  using set_error_t = stdexec::set_error_t;
  /** @brief Sender completion signature. */
  using completion_signatures =
      stdexec::completion_signatures<set_value_t(socket_address<sockaddr_in6>),
                                     set_error_t(std::error_code)>;

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
};

/** @brief The sender for an asynchronous put. */
struct put_file_t : client_sender {
  /** @brief Sender completion signature. */
  using completion_signatures =
      stdexec::completion_signatures<set_value_t(status_t),
                                     set_error_t(std::error_code)>;

  /** @brief sender operation state. */
  template <typename Receiver> struct state_t : client_state<Receiver> {
    /** @brief Initiate the asynchronous put. */
    auto start() noexcept -> void;

    /** @brief Send a WRQ. */
    auto send_wrq() noexcept -> void;

    /**
     * @brief Ack handler.
     * @param ack The ack message.
     */
    auto ack_handler(messages::ack ack) noexcept -> void;

    /** @brief Submit an asynchronous sendmsg to the context. */
    auto submit_sendmsg() noexcept -> void;

    /** @brief Submit async recvmsg to the context. */
    auto submit_recvmsg() noexcept -> void;

    /**
     * @brief Dispatch a received message to the correct handler.
     * @param msg The received message.
     * @param len The length of the received message.
     */
    auto route_message(const char *msg, std::streamsize len) noexcept -> void;
  };

  /**
   * @brief Connect this sender to a receiver.
   * @tparam Receiver receiver type.
   * @param receiver The receiver to connnect to.
   * @returns An operation state for receiver.
   */
  template <typename Receiver>
  auto connect(Receiver &&receiver) -> state_t<Receiver>;
};

/** @brief The sender for an asynchronous get. */
struct get_file_t : client_sender {
  /** @brief Sender completion signature. */
  using completion_signatures =
      stdexec::completion_signatures<set_value_t(status_t),
                                     set_error_t(std::error_code)>;

  /** @brief sender operation state. */
  template <typename Receiver> struct state_t : client_state<Receiver> {
    /** @brief Initiate the asynchronous put. */
    auto start() noexcept -> void;

    /** @brief Send an RRQ. */
    auto send_rrq() noexcept -> void;

    /**
     * @brief Handle a DATA message.
     * @param msg The data message.
     * @param len the length of the message.
     */
    auto data_handler(const char *msg, std::streamsize len) noexcept -> void;

    /** @brief Submit an asynchronous sendmsg to the context. */
    auto submit_sendmsg() noexcept -> void;

    /** @brief Submit async recvmsg to the context. */
    auto submit_recvmsg() noexcept -> void;

    /**
     * @brief Dispatch a received message to the correct handler.
     * @param msg The received message.
     * @param len The length of the received message.
     */
    auto route_message(const char *msg, std::streamsize len) noexcept -> void;
  };

  /**
   * @brief Connect this sender to a receiver.
   * @tparam Receiver receiver type.
   * @param receiver The receiver to connnect to.
   * @returns An operation state for receiver.
   */
  template <typename Receiver>
  auto connect(Receiver &&receiver) -> state_t<Receiver>;
};

} // namespace client.
} // namespace tftp

#include "impl/tftp_impl.hpp" // IWYU pragma: export

#endif // TFTP_HPP
