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
#include <thread>

using namespace net::service;
using namespace tftp;

static constexpr unsigned short PORT = 69;

struct config {
  unsigned short port = PORT;
};

auto main(int argc, char *argv[]) -> int
{
  using namespace stdexec;
  using socket_address = io::socket::socket_address<sockaddr_in6>;

  auto address = socket_address();
  address->sin6_family = AF_INET;

  auto ctx = async_context();
  client_manager manager{address};
  auto &sockets = ctx.timers.sockets;
  ::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets.data());
  ctx.isr(ctx.poller.emplace(sockets[0]), [&]() noexcept -> bool {
    auto sigmask_ = ctx.sigmask.exchange(0);
    auto mask = sigmask_;
    for (int signum = 0; (mask = sigmask_ >> signum); ++signum)
    {
      if (mask & (1 << 0))
        manager.signal_handler(signum);
    }
    if (sigmask_ & (1 << ctx.terminate))
      ctx.scope.request_stop();

    return !ctx.scope.get_stop_token().stop_requested();
  });

  manager.start(ctx);

  auto thread = std::thread([&] { ctx.run(); });

  auto client = client_manager::make_client(ctx);

  sender auto put_file =
      client.connect("localhost", "6969") | let_value([&](auto &&addr) {
        return client.put(std::forward<decltype(addr)>(addr), "./tmp",
                          "/tmp/test", messages::OCTET);
      });
  auto [status] = sync_wait(std::move(put_file)).value();

  auto &[error, message] = status;
  if (error || !message.empty())
    spdlog::error("{} {}", error, message);

  ctx.signal(ctx.terminate);
  thread.join();
  return 0;
}
