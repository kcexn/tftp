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
 * @file tftp_client.cpp
 * @brief This file defines the TFTP client.
 */
#include "tftp/tftp_client.hpp"
namespace tftp {

auto client_manager::client_t::connect(std::string hostname,
                                       std::string port) -> client::connect_t
{
  return {.hostname = std::move(hostname), .port = std::move(port), .ctx = ctx};
}

auto client_manager::make_client(async_context &ctx) -> client_t
{
  auto tmp = client_t();

  tmp.ctx = std::addressof(ctx);
  tmp.rctx.buffer.resize(messages::DATAMSG_MAXLEN);
  tmp.rctx.msg.buffers = tmp.rctx.buffer;

  return tmp;
} // GCOVR_EXCL_LINE

} // namespace tftp
