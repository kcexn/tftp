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
#include "tftp/tftp_client.hpp"

#include <spdlog/cfg/helpers.h>
#include <spdlog/spdlog.h>

#include <csignal>
#include <cstdlib>

using namespace net::service;
using namespace tftp;

static constexpr unsigned short PORT = 69;

struct config {
  unsigned short port = PORT;
};

auto main(int argc, char *argv[]) -> int
{
  using namespace stdexec;

  auto manager = client_manager();
  auto client = manager.make_client();

  {
    sender auto put_file =
        client.connect("localhost", "6969") | let_value([&](auto addr) {
          return client.put(addr, "./tmp", "/tmp/test1", messages::OCTET);
        });

    auto [status] = sync_wait(std::move(put_file)).value();

    auto &[error, message] = status;
    if (error || !message.empty())
    {
      spdlog::error("{} {}", error, message);
      exit(0);
    }
  }

  {
    sender auto get_file =
        client.connect("localhost", "6969") | let_value([&](auto addr) {
          return client.get(addr, "/tmp/test1", "./test", messages::OCTET);
        });

    auto [status] = sync_wait(std::move(get_file)).value();

    auto &[error, message] = status;
    if (error || !message.empty())
    {
      spdlog::error("{} {}", error, message);
      exit(0);
    }
  }

  return 0;
}
