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
 * @file tftp_client.hpp
 * @brief This file declares the TFTP client.
 */
#pragma once
#ifndef TFTP_CLIENT_HPP
#define TFTP_CLIENT_HPP
#include "tftp.hpp"
/** @namespace For top-level tftp services. */
namespace tftp {
/** @brief The service type to use. */
template <typename UDPStreamHandler>
using udp_base = net::service::async_udp_service<UDPStreamHandler, 0>;

/** @brief A TFTP client manager. */
class client_manager : public udp_base<client_manager> {
public:
  /** @brief The base class. */
  using Base = udp_base<client_manager>;
  /** @brief Socket message type. */
  using socket_message = io::socket::socket_message<sockaddr_in6>;

  /** @brief A TFTP client. */
  struct client_t {
    /** @brief A TFTP session. */
    session_t session;
    /** @brief Client context for reading. */
    struct {
      /** @brief Socket message to read into. */
      socket_message msg;
      /** @brief client read buffer. */
      std::vector<std::byte> buffer;
    } rctx;
    /** @brief destination hostname. */
    std::string hostname;
    /** @brief destination filename */
    std::string destination;
    /** @brief asynchronous context pointer. */
    async_context *ctx = nullptr;
    /** @brief destination port number. */
    unsigned short port = 0;

    /**
     * @brief connect the client to a TFTP server.
     * @param hostname the hostname of the server.
     * @param port the port the remote host is listening on.
     * @returns A sender for the connect operation.
     */
    auto connect(std::string hostname,
                 std::string port = {}) -> client::connect_t;
  };

  /**
   * @brief client factory.
   * @param ctx An async context to construct the client on.
   * @returns A TFTP client.
   */
  static auto make_client(async_context &ctx) -> client_t;
};
} // namespace tftp
#endif // TFTP_CLIENT_HPP
