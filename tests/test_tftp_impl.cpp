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
#include "tftp/impl/tftp_impl.hpp"
#include "tftp/protocol/tftp_protocol.hpp"

#include <gtest/gtest.h>

using namespace tftp;
using namespace tftp::client;
using namespace tftp::client::detail;

// NOTE: This file tests the utility functions in tftp_impl.hpp that can be
// unit tested in isolation. The client_state template methods (cleanup,
// submit_sendmsg, submit_recvmsg, ack_handler, data_handler, error_handler)
// and put_file_t/get_file_t state methods require integration testing with:
// - Async I/O operations (io::sendmsg, io::recvmsg)
// - Timer management (ctx->timers)
// - Socket operations (io::socket)
// - File system operations (std::fstream, std::filesystem)
// - Stdexec sender/receiver framework
//
// These methods are marked with GCOVR_EXCL_START/STOP in tftp_impl.hpp
// and should be tested via integration tests.

// =============================================================================
// get_error_message Tests
// =============================================================================

TEST(GetErrorMessageTest, ValidErrorMessage)
{
  // Create a valid error message with null terminator
  std::vector<char> buffer(sizeof(messages::error) + 20);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::FILE_NOT_FOUND);

  const char *msg_start = buffer.data() + sizeof(messages::error);
  std::strcpy(const_cast<char *>(msg_start), "File not found");

  auto message = get_error_message(error, buffer.size());

  EXPECT_EQ(message, "File not found");
}

TEST(GetErrorMessageTest, EmptyErrorMessage)
{
  // Create error with just null terminator
  std::vector<char> buffer(sizeof(messages::error) + 1);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::ACCESS_VIOLATION);

  const char *msg_start = buffer.data() + sizeof(messages::error);
  *const_cast<char *>(msg_start) = '\0';

  auto message = get_error_message(error, buffer.size());

  EXPECT_TRUE(message.empty());
}

TEST(GetErrorMessageTest, NoNullTerminator)
{
  // Create error message without null terminator
  std::vector<char> buffer(sizeof(messages::error) + 10);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::DISK_FULL);

  const char *msg_start = buffer.data() + sizeof(messages::error);
  std::memset(const_cast<char *>(msg_start), 'A', 10);

  auto message = get_error_message(error, buffer.size());

  EXPECT_TRUE(message.empty());
}

TEST(GetErrorMessageTest, MinimalValidMessage)
{
  // Minimum valid error: header + single char + null
  std::vector<char> buffer(sizeof(messages::error) + 2);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::ILLEGAL_OPERATION);

  const char *msg_start = buffer.data() + sizeof(messages::error);
  std::strcpy(const_cast<char *>(msg_start), "X");

  auto message = get_error_message(error, buffer.size());

  EXPECT_EQ(message, "X");
}

TEST(GetErrorMessageTest, LongErrorMessage)
{
  // Create a longer error message
  std::string long_msg =
      "This is a very long error message that contains many characters";
  std::vector<char> buffer(sizeof(messages::error) + long_msg.size() + 1);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::NOT_DEFINED);

  const char *msg_start = buffer.data() + sizeof(messages::error);
  std::strcpy(const_cast<char *>(msg_start), long_msg.c_str());

  auto message = get_error_message(error, buffer.size());

  EXPECT_EQ(message, long_msg);
}

TEST(GetErrorMessageTest, MessageWithSpaces)
{
  std::vector<char> buffer(sizeof(messages::error) + 30);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::FILE_NOT_FOUND);

  const char *msg_start = buffer.data() + sizeof(messages::error);
  std::strcpy(const_cast<char *>(msg_start), "File not found in directory");

  auto message = get_error_message(error, buffer.size());

  EXPECT_EQ(message, "File not found in directory");
}

TEST(GetErrorMessageTest, MessageWithSpecialCharacters)
{
  std::vector<char> buffer(sizeof(messages::error) + 50);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::ACCESS_VIOLATION);

  const char *msg_start = buffer.data() + sizeof(messages::error);
  std::strcpy(const_cast<char *>(msg_start), "Permission denied: /root/file.txt");

  auto message = get_error_message(error, buffer.size());

  EXPECT_EQ(message, "Permission denied: /root/file.txt");
}

TEST(GetErrorMessageTest, NullTerminatorAtStart)
{
  // Null terminator immediately after header
  std::vector<char> buffer(sizeof(messages::error) + 10);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::NOT_DEFINED);

  const char *msg_start = buffer.data() + sizeof(messages::error);
  *const_cast<char *>(msg_start) = '\0';
  // Fill rest with non-null chars
  std::memset(const_cast<char *>(msg_start + 1), 'X', 9);

  auto message = get_error_message(error, buffer.size());

  EXPECT_TRUE(message.empty());
}

TEST(GetErrorMessageTest, MultipleNullTerminators)
{
  // Message with multiple null terminators (should stop at first)
  std::vector<char> buffer(sizeof(messages::error) + 20);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::NOT_DEFINED);

  const char *msg_start = buffer.data() + sizeof(messages::error);
  std::strcpy(const_cast<char *>(msg_start), "first");
  // Add another string after first null
  std::strcpy(const_cast<char *>(msg_start + 6), "second");

  auto message = get_error_message(error, buffer.size());

  EXPECT_EQ(message, "first");
}

// =============================================================================
// try_with Exception Handling Tests
// =============================================================================

struct MockReceiver {
  bool error_called = false;
  std::error_code error_code;

  friend auto tag_invoke(stdexec::set_error_t, MockReceiver &self,
                         std::error_code ec) noexcept -> void
  {
    self.error_called = true;
    self.error_code = ec;
  }
};

TEST(TryWithTest, SuccessfulExecution)
{
  MockReceiver receiver;
  bool handler_called = false;
  bool cleanup_called = false;

  auto handler = [&]() { handler_called = true; };
  auto cleanup = [&]() noexcept { cleanup_called = true; };

  try_with(receiver, handler, cleanup);

  EXPECT_TRUE(handler_called);
  EXPECT_FALSE(cleanup_called);
  EXPECT_FALSE(receiver.error_called);
}

TEST(TryWithTest, BadAllocException)
{
  MockReceiver receiver;
  bool cleanup_called = false;

  auto handler = [&]() { throw std::bad_alloc(); };
  auto cleanup = [&]() noexcept { cleanup_called = true; };

  try_with(receiver, handler, cleanup);

  EXPECT_TRUE(cleanup_called);
  EXPECT_TRUE(receiver.error_called);
  EXPECT_EQ(receiver.error_code, std::make_error_code(std::errc::not_enough_memory));
}

TEST(TryWithTest, GenericException)
{
  MockReceiver receiver;
  bool cleanup_called = false;

  auto handler = [&]() { throw std::runtime_error("test error"); };
  auto cleanup = [&]() noexcept { cleanup_called = true; };

  try_with(receiver, handler, cleanup);

  EXPECT_TRUE(cleanup_called);
  EXPECT_TRUE(receiver.error_called);
  EXPECT_EQ(receiver.error_code,
            std::make_error_code(std::errc::state_not_recoverable));
}

TEST(TryWithTest, CleanupDoesNotThrow)
{
  MockReceiver receiver;
  bool cleanup_called = false;

  auto handler = [&]() { throw std::runtime_error("test"); };
  auto cleanup = [&]() noexcept { cleanup_called = true; };

  // Should not throw, even though handler throws
  EXPECT_NO_THROW(try_with(receiver, handler, cleanup));

  EXPECT_TRUE(cleanup_called);
  EXPECT_TRUE(receiver.error_called);
}

TEST(TryWithTest, MultipleExceptionTypes)
{
  MockReceiver receiver;
  bool cleanup_called = false;

  auto handler = [&]() { throw std::logic_error("logic error"); };
  auto cleanup = [&]() noexcept { cleanup_called = true; };

  try_with(receiver, handler, cleanup);

  EXPECT_TRUE(cleanup_called);
  EXPECT_TRUE(receiver.error_called);
  EXPECT_EQ(receiver.error_code,
            std::make_error_code(std::errc::state_not_recoverable));
}

// =============================================================================
// Buffer Boundary Tests
// =============================================================================

TEST(BufferBoundaryTest, MinimumBufferSize)
{
  // Minimum buffer: just the header, no room for message
  std::vector<char> buffer(sizeof(messages::error));
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::NOT_DEFINED);

  auto message = get_error_message(error, buffer.size());
  EXPECT_TRUE(message.empty());
}

TEST(BufferBoundaryTest, OneBytePastHeader)
{
  // Header + 1 byte (null terminator)
  std::vector<char> buffer(sizeof(messages::error) + 1);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::NOT_DEFINED);

  const char *msg_start = buffer.data() + sizeof(messages::error);
  *const_cast<char *>(msg_start) = '\0';

  auto message = get_error_message(error, buffer.size());
  EXPECT_TRUE(message.empty());
}

TEST(BufferBoundaryTest, LargeBuffer)
{
  // Very large buffer
  std::vector<char> buffer(sizeof(messages::error) + 1000);
  auto *error = reinterpret_cast<messages::error *>(buffer.data());
  error->opc = htons(messages::ERROR);
  error->error = htons(messages::NOT_DEFINED);

  std::string large_msg(500, 'X');
  const char *msg_start = buffer.data() + sizeof(messages::error);
  std::strcpy(const_cast<char *>(msg_start), large_msg.c_str());

  auto message = get_error_message(error, buffer.size());
  EXPECT_EQ(message, large_msg);
}

// NOLINTEND
