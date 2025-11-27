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
#include "tftp/tftp.hpp"
#include "tftp/tftp_client.hpp"

#include <gtest/gtest.h>

using namespace tftp;
using namespace tftp::client;

// =============================================================================
// connect_t Sender Tests
// =============================================================================

TEST(ConnectSenderTest, DefaultConstruction)
{
  connect_t sender{.hostname = "example.com", .port = "69"};

  EXPECT_EQ(sender.hostname, "example.com");
  EXPECT_EQ(sender.port, "69");
}

TEST(ConnectSenderTest, EmptyHostname)
{
  connect_t sender{.hostname = "", .port = "69"};

  EXPECT_TRUE(sender.hostname.empty());
  EXPECT_EQ(sender.port, "69");
}

TEST(ConnectSenderTest, CustomPort)
{
  connect_t sender{.hostname = "localhost", .port = "6969"};

  EXPECT_EQ(sender.hostname, "localhost");
  EXPECT_EQ(sender.port, "6969");
}

TEST(ConnectSenderTest, HostnamePreserved)
{
  std::string hostname = "test.example.com";
  std::string port = "12345";

  connect_t sender{.hostname = hostname, .port = port};

  EXPECT_EQ(sender.hostname, hostname);
  EXPECT_EQ(sender.port, port);
}

// =============================================================================
// put_file_t Sender Tests
// =============================================================================

TEST(PutFileSenderTest, BasicConstruction)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  put_file_t sender{{.server_addr = server_addr,
                     .local = "/tmp/local.txt",
                     .remote = "/tmp/remote.txt",
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_EQ(sender.local, "/tmp/local.txt");
  EXPECT_EQ(sender.remote, "/tmp/remote.txt");
  EXPECT_EQ(sender.mode, messages::OCTET);
  EXPECT_EQ(sender.ctx, client.ctx);
}

TEST(PutFileSenderTest, NetasciiMode)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  put_file_t sender{{.server_addr = server_addr,
                     .local = "file.txt",
                     .remote = "remote.txt",
                     .ctx = client.ctx,
                     .mode = messages::NETASCII}};

  EXPECT_EQ(sender.mode, messages::NETASCII);
}

TEST(PutFileSenderTest, MailMode)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  put_file_t sender{{.server_addr = server_addr,
                     .local = "mail.txt",
                     .remote = "inbox",
                     .ctx = client.ctx,
                     .mode = messages::MAIL}};

  EXPECT_EQ(sender.mode, messages::MAIL);
}

TEST(PutFileSenderTest, PathsPreserved)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  std::filesystem::path local = "/path/to/source.dat";
  std::filesystem::path remote = "/destination/target.dat";

  put_file_t sender{{.server_addr = server_addr,
                     .local = local,
                     .remote = remote,
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_EQ(sender.local, local);
  EXPECT_EQ(sender.remote, remote);
}

TEST(PutFileSenderTest, ContextNotNull)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  put_file_t sender{{.server_addr = server_addr,
                     .local = "local.txt",
                     .remote = "remote.txt",
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_NE(sender.ctx, nullptr);
}

TEST(PutFileSenderTest, RelativePaths)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  put_file_t sender{{.server_addr = server_addr,
                     .local = "local.txt",
                     .remote = "remote.txt",
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_EQ(sender.local, "local.txt");
  EXPECT_EQ(sender.remote, "remote.txt");
}

// =============================================================================
// get_file_t Sender Tests
// =============================================================================

TEST(GetFileSenderTest, BasicConstruction)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  get_file_t sender{{.server_addr = server_addr,
                     .local = "/tmp/local.txt",
                     .remote = "/tmp/remote.txt",
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_EQ(sender.local, "/tmp/local.txt");
  EXPECT_EQ(sender.remote, "/tmp/remote.txt");
  EXPECT_EQ(sender.mode, messages::OCTET);
  EXPECT_EQ(sender.ctx, client.ctx);
}

TEST(GetFileSenderTest, NetasciiMode)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  get_file_t sender{{.server_addr = server_addr,
                     .local = "file.txt",
                     .remote = "remote.txt",
                     .ctx = client.ctx,
                     .mode = messages::NETASCII}};

  EXPECT_EQ(sender.mode, messages::NETASCII);
}

TEST(GetFileSenderTest, MailMode)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  get_file_t sender{{.server_addr = server_addr,
                     .local = "mail.txt",
                     .remote = "inbox",
                     .ctx = client.ctx,
                     .mode = messages::MAIL}};

  EXPECT_EQ(sender.mode, messages::MAIL);
}

TEST(GetFileSenderTest, PathsPreserved)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  std::filesystem::path local = "/path/to/destination.dat";
  std::filesystem::path remote = "/source/origin.dat";

  get_file_t sender{{.server_addr = server_addr,
                     .local = local,
                     .remote = remote,
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_EQ(sender.local, local);
  EXPECT_EQ(sender.remote, remote);
}

TEST(GetFileSenderTest, ContextNotNull)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  get_file_t sender{{.server_addr = server_addr,
                     .local = "local.txt",
                     .remote = "remote.txt",
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_NE(sender.ctx, nullptr);
}

TEST(GetFileSenderTest, RelativePaths)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  get_file_t sender{{.server_addr = server_addr,
                     .local = "local.txt",
                     .remote = "remote.txt",
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_EQ(sender.local, "local.txt");
  EXPECT_EQ(sender.remote, "remote.txt");
}

// =============================================================================
// Sender Type Trait Tests
// =============================================================================

TEST(SenderTypeTraitTest, ConnectHasSenderConcept)
{
  EXPECT_TRUE((std::is_same_v<connect_t::sender_concept, stdexec::sender_t>));
}

TEST(SenderTypeTraitTest, PutFileHasSenderConcept)
{
  EXPECT_TRUE(
      (std::is_same_v<put_file_t::sender_concept, stdexec::sender_t>));
}

TEST(SenderTypeTraitTest, GetFileHasSenderConcept)
{
  EXPECT_TRUE(
      (std::is_same_v<get_file_t::sender_concept, stdexec::sender_t>));
}

TEST(SenderTypeTraitTest, PutFileInheritsClientSender)
{
  EXPECT_TRUE((std::is_base_of_v<client_sender, put_file_t>));
}

TEST(SenderTypeTraitTest, GetFileInheritsClientSender)
{
  EXPECT_TRUE((std::is_base_of_v<client_sender, get_file_t>));
}

// =============================================================================
// Sender Comparison Tests
// =============================================================================

TEST(SenderComparisonTest, PutAndGetSameModeType)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;

  put_file_t put_sender{{.server_addr = server_addr,
                         .local = "local.txt",
                         .remote = "remote.txt",
                         .ctx = client.ctx,
                         .mode = messages::OCTET}};

  get_file_t get_sender{{.server_addr = server_addr,
                         .local = "local.txt",
                         .remote = "remote.txt",
                         .ctx = client.ctx,
                         .mode = messages::OCTET}};

  EXPECT_EQ(put_sender.mode, get_sender.mode);
}

TEST(SenderComparisonTest, PutAndGetShareContext)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;

  put_file_t put_sender{{.server_addr = server_addr,
                         .local = "local.txt",
                         .remote = "remote.txt",
                         .ctx = client.ctx,
                         .mode = messages::OCTET}};

  get_file_t get_sender{{.server_addr = server_addr,
                         .local = "local.txt",
                         .remote = "remote.txt",
                         .ctx = client.ctx,
                         .mode = messages::OCTET}};

  EXPECT_EQ(put_sender.ctx, get_sender.ctx);
}

TEST(SenderComparisonTest, DifferentModes)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;

  put_file_t put_octet{{.server_addr = server_addr,
                        .local = "local.txt",
                        .remote = "remote.txt",
                        .ctx = client.ctx,
                        .mode = messages::OCTET}};

  put_file_t put_netascii{{.server_addr = server_addr,
                           .local = "local.txt",
                           .remote = "remote.txt",
                           .ctx = client.ctx,
                           .mode = messages::NETASCII}};

  EXPECT_NE(put_octet.mode, put_netascii.mode);
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST(SenderEdgeCaseTest, EmptyPaths)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  put_file_t sender{{.server_addr = server_addr,
                     .local = "",
                     .remote = "",
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_TRUE(sender.local.empty());
  EXPECT_TRUE(sender.remote.empty());
}

TEST(SenderEdgeCaseTest, VeryLongPaths)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  std::string long_path(1000, 'a');

  get_file_t sender{{.server_addr = server_addr,
                     .local = long_path,
                     .remote = long_path,
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_EQ(sender.local.string().length(), 1000);
  EXPECT_EQ(sender.remote.string().length(), 1000);
}

TEST(SenderEdgeCaseTest, SpecialCharactersInPaths)
{
  auto manager = client_manager();
  auto client = manager.make_client();

  io::socket::socket_address<sockaddr_in6> server_addr;
  put_file_t sender{{.server_addr = server_addr,
                     .local = "path/with spaces/file.txt",
                     .remote = "remote/with-dashes_and.dots.txt",
                     .ctx = client.ctx,
                     .mode = messages::OCTET}};

  EXPECT_EQ(sender.local, "path/with spaces/file.txt");
  EXPECT_EQ(sender.remote, "remote/with-dashes_and.dots.txt");
}

TEST(SenderEdgeCaseTest, ConnectEmptyPort)
{
  connect_t sender{.hostname = "example.com", .port = ""};

  EXPECT_EQ(sender.hostname, "example.com");
  EXPECT_TRUE(sender.port.empty());
}

// NOLINTEND
