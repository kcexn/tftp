/* Copyright (C) 2025 Kevin Exton (kevin.exton@pm.me)
 *
 * tftp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tftp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tftp.  If not, see <https://www.gnu.org/licenses/>.
 */
/**
 * @file tftp_client.cpp
 * @brief This file defines the TFTP client.
 */
#include "tftp/tftp_client.hpp"
namespace tftp {

auto client_manager::client_t::connect(
    std::string hostname, std::string port) noexcept -> client::connect_t
{
  return {.hostname = std::move(hostname), .port = std::move(port)};
}

auto client_manager::client_t::put(
    io::socket::socket_address<sockaddr_in6> server_addr, std::string local,
    std::string remote, std::uint8_t mode) const noexcept -> client::put_file_t
{
  return {{.server_addr = server_addr,
           .local = std::move(local),
           .remote = std::move(remote),
           .ctx = ctx,
           .mode = mode}};
}

auto client_manager::client_t::get(
    io::socket::socket_address<sockaddr_in6> server_addr, std::string remote,
    std::string local, std::uint8_t mode) const noexcept -> client::get_file_t
{
  return {{.server_addr = server_addr,
           .local = std::move(local),
           .remote = std::move(remote),
           .ctx = ctx,
           .mode = mode}};
}

auto client_manager::make_client() -> client_t
{
  if (ctx_.state == ctx_.PENDING)
    ctx_.start();

  return {.ctx = std::addressof(ctx_)};
} // GCOVR_EXCL_LINE

} // namespace tftp
