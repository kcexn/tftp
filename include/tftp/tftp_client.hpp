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
class client_manager {
public:
  /** @brief async context type. */
  using async_context = net::service::async_context;
  /** @brief context thread type. */
  using context_thread = net::service::context_thread;
  /** @brief Socket message type. */
  using socket_message = io::socket::socket_message<sockaddr_in6>;

  /** @brief A TFTP client. */
  struct client_t {
    /** @brief asynchronous context pointer. */
    async_context *ctx = nullptr;

    /**
     * @brief connect the client to a TFTP server.
     * @param hostname the hostname of the server.
     * @param port the port the remote host is listening on.
     * @returns A sender for the connect operation.
     */
    [[nodiscard]] auto
    connect(std::string hostname,
            std::string port = "69") const noexcept -> client::connect_t;

    /**
     * @brief send a file to the tftp server.
     * @param server_addr The address to send the file to.
     * @param local the local file to send.
     * @param remote the remote path to write to.
     * @param mode the tftp transmission mode (default: netascii)
     * @returns A sender for the put file operation.
     */
    [[nodiscard]] auto put(io::socket::socket_address<sockaddr_in6> server_addr,
                           std::string local, std::string remote,
                           std::uint8_t mode = messages::NETASCII)
        const noexcept -> client::put_file_t;

    /**
     * @brief get a file from the tftp server.
     * @param server_addr The address of the TFTP server.
     * @param remote the remote path to read from.
     * @param local the local path to write to.
     * @param mode the tftp transmission mode (default: netascii)
     * @returns A sender for the put file operation.
     */
    [[nodiscard]] auto get(io::socket::socket_address<sockaddr_in6> server_addr,
                           std::string remote, std::string local,
                           std::uint8_t mode = messages::NETASCII)
        const noexcept -> client::get_file_t;
  };

  /**
   * @brief client factory.
   * @returns A TFTP client.
   */
  auto make_client() -> client_t;

private:
  context_thread ctx_;
};
} // namespace tftp
#endif // TFTP_CLIENT_HPP
