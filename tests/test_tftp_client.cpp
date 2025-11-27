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

// NOLINTBEGIN
#include "tftp/tftp_client.hpp"

#include <gtest/gtest.h>

using namespace tftp;

TEST(TftpClientTest, MakeClient)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  EXPECT_NE(client.ctx, nullptr);
}

TEST(TftpClientTest, Connect)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  std::string hostname = "localhost";
  std::string port = "69";
  auto connect_sender = client.connect(hostname, port);

  EXPECT_EQ(connect_sender.hostname, hostname);
  EXPECT_EQ(connect_sender.port, port);
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

  auto manager = client_manager();
  auto client = manager.make_client();
  auto connect_sender = client.connect("localhost", "69");

  EXPECT_THROW(sync_wait(std::move(connect_sender)), std::system_error);
}

TEST(ConnectTest, GetAddrInfoEmpty)
{
  getaddrinfo_err = 0;
  getaddrinfo_res = nullptr;

  auto manager = client_manager();
  auto client = manager.make_client();
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

  auto manager = client_manager();
  auto client = manager.make_client();
  auto connect_sender = client.connect("localhost", "69");

  auto [addr] = stdexec::sync_wait(std::move(connect_sender)).value();
  ASSERT_EQ(addr->sin6_family, AF_INET);
  auto *ptr = reinterpret_cast<sockaddr_in *>(std::ranges::data(addr));
  EXPECT_EQ(ptr->sin_port, htons(69));
  EXPECT_EQ(ptr->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
}

TEST(TftpClientTest, PutFile)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  std::string local = "/tmp/local.txt";
  std::string remote = "/tmp/remote.txt";

  auto put_sender = client.put(server_addr, local, remote, messages::OCTET);

  EXPECT_EQ(put_sender.local, local);
  EXPECT_EQ(put_sender.remote, remote);
  EXPECT_EQ(put_sender.mode, messages::OCTET);
  EXPECT_EQ(put_sender.ctx, client.ctx);
}

TEST(TftpClientTest, PutFileDefaultMode)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  std::string local = "local.txt";
  std::string remote = "remote.txt";

  auto put_sender = client.put(server_addr, local, remote);

  EXPECT_EQ(put_sender.local, local);
  EXPECT_EQ(put_sender.remote, remote);
  EXPECT_EQ(put_sender.mode, messages::NETASCII);
  EXPECT_EQ(put_sender.ctx, client.ctx);
}

TEST(TftpClientTest, GetFile)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  std::string local = "/tmp/local.txt";
  std::string remote = "/tmp/remote.txt";

  auto get_sender = client.get(server_addr, remote, local, messages::OCTET);

  EXPECT_EQ(get_sender.local, local);
  EXPECT_EQ(get_sender.remote, remote);
  EXPECT_EQ(get_sender.mode, messages::OCTET);
  EXPECT_EQ(get_sender.ctx, client.ctx);
}

TEST(TftpClientTest, GetFileDefaultMode)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  std::string local = "local.txt";
  std::string remote = "remote.txt";

  auto get_sender = client.get(server_addr, remote, local);

  EXPECT_EQ(get_sender.local, local);
  EXPECT_EQ(get_sender.remote, remote);
  EXPECT_EQ(get_sender.mode, messages::NETASCII);
  EXPECT_EQ(get_sender.ctx, client.ctx);
}

TEST(TftpClientTest, GetFileMailMode)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;

  auto get_sender =
      client.get(server_addr, "remote.txt", "local.txt", messages::MAIL);

  EXPECT_EQ(get_sender.mode, messages::MAIL);
}

TEST(TftpClientTest, MultipleClients)
{
  auto manager = client_manager();
  auto client1 = manager.make_client();
  auto client2 = manager.make_client();

  EXPECT_NE(client1.ctx, nullptr);
  EXPECT_NE(client2.ctx, nullptr);
  EXPECT_EQ(client1.ctx, client2.ctx);
}

TEST(TftpClientTest, PutFileParameterOrder)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;

  auto put_sender =
      client.put(server_addr, "source.txt", "destination.txt", messages::OCTET);

  EXPECT_EQ(put_sender.local, "source.txt");
  EXPECT_EQ(put_sender.remote, "destination.txt");
}

TEST(TftpClientTest, GetFileParameterOrder)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;

  auto get_sender =
      client.get(server_addr, "source.txt", "destination.txt", messages::OCTET);

  EXPECT_EQ(get_sender.remote, "source.txt");
  EXPECT_EQ(get_sender.local, "destination.txt");
}

TEST(TftpClientTest, ConnectDefaultPort)
{
  std::string hostname = "example.com";
  auto connect_sender = client_manager::client_t::connect(hostname);

  EXPECT_EQ(connect_sender.hostname, hostname);
  EXPECT_EQ(connect_sender.port, "69");
}

TEST(TftpClientTest, ConnectCustomPort)
{
  std::string hostname = "example.com";
  std::string port = "6969";
  auto connect_sender = client_manager::client_t::connect(hostname, port);

  EXPECT_EQ(connect_sender.hostname, hostname);
  EXPECT_EQ(connect_sender.port, port);
}
// NOLINTEND
