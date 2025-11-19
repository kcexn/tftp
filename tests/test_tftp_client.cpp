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

// NOLINTBEGIN
#include "tftp/tftp_client.hpp"

#include <gtest/gtest.h>
#include <net/cppnet.hpp>

using namespace tftp;

TEST(TftpClientTest, MakeClient)
{
  net::service::async_context ctx;
  auto client = client_manager::make_client(ctx);

  ASSERT_NE(client.ctx, nullptr);
  EXPECT_EQ(client.ctx, &ctx);
  EXPECT_FALSE(client.rctx.buffer.empty());
  EXPECT_EQ(client.rctx.buffer.size(), tftp::messages::DATAMSG_MAXLEN);
}

TEST(TftpClientTest, Connect)
{
  net::service::async_context ctx;
  auto client = client_manager::make_client(ctx);

  std::string hostname = "localhost";
  std::string port = "69";
  auto connect_sender = client.connect(hostname, port);

  EXPECT_EQ(connect_sender.hostname, hostname);
  EXPECT_EQ(connect_sender.port, port);
  EXPECT_EQ(connect_sender.ctx, &ctx);
}

// Mock getaddrinfo
int getaddrinfo_err = 0;
struct addrinfo *getaddrinfo_res = nullptr;

extern "C" {
int __wrap_getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints, struct addrinfo **res)
{
  if (getaddrinfo_err)
  {
    return getaddrinfo_err;
  }
  *res = getaddrinfo_res;
  return 0;
}

void __wrap_freeaddrinfo(struct addrinfo *res)
{
  if (res)
  {
    free(res->ai_addr);
    free(res);
  }
}
}

TEST(ConnectTest, GetAddrInfoError)
{
  using namespace stdexec;

  getaddrinfo_err = EAI_FAIL;
  getaddrinfo_res = nullptr;

  net::service::async_context ctx;
  auto client = client_manager::make_client(ctx);
  auto connect_sender = client.connect("localhost", "69");

  EXPECT_THROW(sync_wait(std::move(connect_sender)), std::system_error);
}

TEST(ConnectTest, GetAddrInfoEmpty)
{
  getaddrinfo_err = 0;
  getaddrinfo_res = nullptr;

  net::service::async_context ctx;
  auto client = client_manager::make_client(ctx);
  auto connect_sender = client.connect("localhost", "69");

  try
  {
    stdexec::sync_wait(std::move(connect_sender));
  }
  catch (const std::system_error &err)
  {
    EXPECT_EQ(err.code(), make_error_code(tftp::dns::errc::address_not_found));
  }
}

TEST(ConnectTest, Connects)
{
  getaddrinfo_err = 0;

  auto ai = new addrinfo;
  ai->ai_family = AF_INET;
  ai->ai_socktype = SOCK_DGRAM;
  ai->ai_protocol = IPPROTO_UDP;
  ai->ai_addrlen = sizeof(sockaddr_in);
  ai->ai_addr = (struct sockaddr *)new sockaddr_in;
  ((struct sockaddr_in *)ai->ai_addr)->sin_family = AF_INET;
  ((struct sockaddr_in *)ai->ai_addr)->sin_port = htons(69);
  ((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ai->ai_next = nullptr;
  getaddrinfo_res = ai;

  net::service::async_context ctx;
  auto client = client_manager::make_client(ctx);
  auto connect_sender = client.connect("localhost", "69");

  auto [addr] = stdexec::sync_wait(std::move(connect_sender)).value();
  ASSERT_EQ(addr->sin6_family, AF_INET);
  auto *ptr = reinterpret_cast<sockaddr_in *>(std::ranges::data(addr));
  EXPECT_EQ(ptr->sin_port, htons(69));
  EXPECT_EQ(ptr->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
}
// NOLINTEND
