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
// This file tests static inline functions in tftp.cpp
// We define TFTP_SERVER_STATIC_TEST to expose these functions for testing
#define TFTP_SERVER_STATIC_TEST

#include "tftp/filesystem.hpp"
#include "tftp/protocol/tftp_protocol.hpp"
#include "tftp/protocol/tftp_session.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

// Include the implementation to get access to static functions
#include "../src/tftp.cpp"

using namespace tftp;

class TestTftpStatic : public ::testing::Test {
protected:
  void TearDown() override
  {
    // Clean up any files created during tests
    for (const auto &file : created_files)
    {
      if (std::filesystem::exists(file))
      {
        std::filesystem::remove(file);
      }
    }
    created_files.clear();
  }

  // Helper to create a test file
  auto create_test_file(const std::string &content = "test content")
      -> std::filesystem::path
  {
    const auto path = filesystem::tmpname();
    std::ofstream(path) << content;
    created_files.push_back(path);
    return path;
  }

  std::vector<std::filesystem::path> created_files;
};

// =============================================================================
// insert_data Tests
// =============================================================================
using enum messages::mode_t;
using enum messages::opcode_t;

TEST_F(TestTftpStatic, InsertData_OctetModeCopiesDirectly)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = std::string("Hello\0World\r\n", 13);
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, OCTET);

  // In OCTET mode, data should be copied as-is
  EXPECT_EQ(buffer.size(), sizeof(messages::data) + test_data.size());
  EXPECT_EQ(std::string(buffer.begin() + sizeof(messages::data), buffer.end()),
            test_data);
}

TEST_F(TestTftpStatic, InsertData_NetasciiSkipsBareNullBytes)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = std::string("Hello\0World", 11);
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Line 59 in tftp.cpp: Bare \0 bytes should be skipped in NETASCII mode
  // Expected result: "Hello" + "World" (no null byte)
  const auto result =
      std::string(buffer.begin() + sizeof(messages::data), buffer.end());
  EXPECT_EQ(result, "HelloWorld");
}

TEST_F(TestTftpStatic, InsertData_NetasciiConvertsBareLineFeed)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = "Line1\nLine2";
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Bare \n should be converted to \r\n
  const auto result =
      std::string(buffer.begin() + sizeof(messages::data), buffer.end());
  EXPECT_EQ(result, "Line1\r\nLine2");
}

TEST_F(TestTftpStatic, InsertData_NetasciiConvertsBareCarriageReturn)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = "Text\rMore";
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Line 80 in tftp.cpp: Bare \r should be followed by \0
  const auto result =
      std::string(buffer.begin() + sizeof(messages::data), buffer.end());
  EXPECT_EQ(result, std::string("Text\r\0More", 10));
}

TEST_F(TestTftpStatic, InsertData_NetasciiHandlesCrlfSequence)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = "Line1\r\nLine2";
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Line 66 in tftp.cpp: \r\n in input should produce \r\n in output
  // The \r produces \r\0, then \n sees the \0 and pops it, then adds \r\n
  const auto result =
      std::string(buffer.begin() + sizeof(messages::data), buffer.end());
  EXPECT_EQ(result, "Line1\r\nLine2");
}

TEST_F(TestTftpStatic, InsertData_NetasciiMultipleBareLineFeeds)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = "A\nB\nC";
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Each \n should be converted to \r\n
  const auto result =
      std::string(buffer.begin() + sizeof(messages::data), buffer.end());
  EXPECT_EQ(result, "A\r\nB\r\nC");
}

TEST_F(TestTftpStatic, InsertData_NetasciiMultipleBareCarriageReturns)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = "A\rB\rC";
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Each \r should be followed by \0
  const auto result =
      std::string(buffer.begin() + sizeof(messages::data), buffer.end());
  EXPECT_EQ(result, std::string("A\r\0B\r\0C", 7));
}

TEST_F(TestTftpStatic, InsertData_NetasciiMixedLineEndings)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = "Unix\nWindows\r\nMac\r";
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Unix \n -> \r\n, Windows \r\n -> \r\n, Mac \r -> \r\0
  const auto result =
      std::string(buffer.begin() + sizeof(messages::data), buffer.end());
  EXPECT_EQ(result, std::string("Unix\r\nWindows\r\nMac\r\0", 20));
}

TEST_F(TestTftpStatic, InsertData_NetasciiEmptyInput)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = "";
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Empty input should leave buffer unchanged
  EXPECT_EQ(buffer.size(), sizeof(messages::data));
}

TEST_F(TestTftpStatic, InsertData_NetasciiOnlyNullBytes)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = std::string("\0\0\0", 3);
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Line 59 in tftp.cpp: All null bytes should be skipped
  EXPECT_EQ(buffer.size(), sizeof(messages::data));
}

TEST_F(TestTftpStatic, InsertData_MailModeUsesNetasciiConversion)
{
  std::vector<char> buffer(sizeof(messages::data));
  const std::string test_data = "Mail\nBody\r";
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, MAIL);

  // MAIL mode should behave like NETASCII
  const auto result =
      std::string(buffer.begin() + sizeof(messages::data), buffer.end());
  EXPECT_EQ(result, std::string("Mail\r\nBody\r\0", 12));
}

TEST_F(TestTftpStatic, InsertData_NetasciiComplexSequence)
{
  std::vector<char> buffer(sizeof(messages::data));
  // Test a complex sequence with all special cases
  const std::string test_data = std::string("A\0B\rC\nD\r\nE", 11);
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Expected:
  // A -> A
  // \0 -> (skipped, line 59)
  // B -> B
  // \r -> \r\0 (line 80)
  // C -> C
  // \n -> \r\n (bare \n)
  // D -> D
  // \r -> \r\0 (line 80), then \n sees \0 and pops it (line 66), adds \r\n
  // E -> E
  const auto result =
      std::string(buffer.begin() + sizeof(messages::data), buffer.end());
  EXPECT_EQ(result, std::string("AB\r\0C\r\nD\r\nE", 11));
}

TEST_F(TestTftpStatic, InsertData_NetasciiLargeDataWithMixedEndings)
{
  std::vector<char> buffer(sizeof(messages::data));
  std::string test_data;
  for (int i = 0; i < 100; i++)
  {
    test_data += "Line" + std::to_string(i) + "\n";
  }
  std::span<const char> data_span(test_data.data(), test_data.size());

  insert_data(buffer, data_span, NETASCII);

  // Verify buffer has grown appropriately (each \n becomes \r\n)
  EXPECT_GT(buffer.size(), sizeof(messages::data) + test_data.size());
}

// =============================================================================
// send_next Tests
// =============================================================================

TEST_F(TestTftpStatic, SendNext_IncrementsBlockNumber)
{
  session_t session;
  const auto test_file = create_test_file("Hello World");

  session.state.file = std::make_shared<std::fstream>(
      test_file, std::ios::in | std::ios::binary);
  session.state.mode = OCTET;
  session.state.block_num = 0;
  session.state.buffer.resize(sizeof(messages::data));

  const auto result = send_next(&session);

  EXPECT_EQ(result, 0);
  EXPECT_EQ(session.state.block_num, 1);
}

TEST_F(TestTftpStatic, SendNext_ReadsFileData)
{
  session_t session;
  const std::string content = "Test data content";
  const auto test_file = create_test_file(content);

  session.state.file = std::make_shared<std::fstream>(
      test_file, std::ios::in | std::ios::binary);
  session.state.mode = OCTET;
  session.state.block_num = 0;
  session.state.buffer.resize(sizeof(messages::data));

  const auto result = send_next(&session);

  EXPECT_EQ(result, 0);
  EXPECT_GT(session.state.buffer.size(), sizeof(messages::data));

  // Verify data packet structure
  const auto *msg =
      reinterpret_cast<const messages::data *>(session.state.buffer.data());
  EXPECT_EQ(ntohs(msg->opc), DATA);
  EXPECT_EQ(ntohs(msg->block_num), 1);

  // Verify content
  const auto data_content =
      std::string(session.state.buffer.begin() + sizeof(messages::data),
                  session.state.buffer.end());
  EXPECT_EQ(data_content, content);
}

TEST_F(TestTftpStatic, SendNext_HandlesNetasciiConversion)
{
  session_t session;
  const std::string content = "Line1\nLine2\n";
  const auto test_file = create_test_file(content);

  session.state.file = std::make_shared<std::fstream>(
      test_file, std::ios::in | std::ios::binary);
  session.state.mode = NETASCII;
  session.state.block_num = 0;
  session.state.buffer.resize(sizeof(messages::data));

  const auto result = send_next(&session);

  EXPECT_EQ(result, 0);

  // Verify NETASCII conversion occurred (\n -> \r\n)
  const auto data_content =
      std::string(session.state.buffer.begin() + sizeof(messages::data),
                  session.state.buffer.end());
  EXPECT_EQ(data_content, "Line1\r\nLine2\r\n");
}

TEST_F(TestTftpStatic, SendNext_HandlesOverflowData)
{
  session_t session;
  // Create a file larger than one data block (512 bytes)
  const std::string content(1024, 'X');
  const auto test_file = create_test_file(content);

  session.state.file = std::make_shared<std::fstream>(
      test_file, std::ios::in | std::ios::binary);
  session.state.mode = OCTET;
  session.state.block_num = 0;
  session.state.buffer.resize(sizeof(messages::data));

  // First call should read 512 bytes
  auto result = send_next(&session);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(session.state.block_num, 1);
  EXPECT_EQ(session.state.buffer.size(), messages::DATAMSG_MAXLEN);

  // Second call should read remaining data
  result = send_next(&session);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(session.state.block_num, 2);
  EXPECT_EQ(session.state.buffer.size(), messages::DATAMSG_MAXLEN);
}

TEST_F(TestTftpStatic, SendNext_OverflowHandlingWithNetascii)
{
  session_t session;
  // Create file with line feeds that will expand during NETASCII conversion
  // Each \n becomes \r\n, so 300 \n chars become 600 chars
  std::string content;
  for (int i = 0; i < 300; i++)
  {
    content += "Line\n"; // 5 bytes becomes 6 bytes in NETASCII
  }
  const auto test_file = create_test_file(content);

  session.state.file = std::make_shared<std::fstream>(
      test_file, std::ios::in | std::ios::binary);
  session.state.mode = NETASCII;
  session.state.block_num = 0;
  session.state.buffer.resize(sizeof(messages::data));

  // First send_next call - should cause overflow due to NETASCII expansion
  auto result = send_next(&session);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(session.state.block_num, 1);

  // Buffer should be at max size with potential overflow
  EXPECT_GE(session.state.buffer.size(), messages::DATAMSG_MAXLEN);

  // Second call should handle overflow data (lines 127-128 in tftp.cpp)
  // This moves overflow data to the beginning of the buffer
  result = send_next(&session);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(session.state.block_num, 2);

  // Verify the overflow was properly moved
  EXPECT_GE(session.state.buffer.size(), sizeof(messages::data));
}

TEST_F(TestTftpStatic, SendNext_HandlesBlockNumberWrapAround)
{
  session_t session;
  const auto test_file = create_test_file("Test");

  session.state.file = std::make_shared<std::fstream>(
      test_file, std::ios::in | std::ios::binary);
  session.state.mode = OCTET;
  session.state.block_num = 0xFFFF;
  session.state.buffer.resize(sizeof(messages::data));

  const auto result = send_next(&session);

  EXPECT_EQ(result, 0);
  EXPECT_EQ(session.state.block_num, 0); // Wraps around
}

TEST_F(TestTftpStatic, SendNext_SetsCorrectOpcodeAndBlockNum)
{
  session_t session;
  const auto test_file = create_test_file("Data");

  session.state.file = std::make_shared<std::fstream>(
      test_file, std::ios::in | std::ios::binary);
  session.state.mode = OCTET;
  session.state.block_num = 42;
  session.state.buffer.resize(sizeof(messages::data));

  const auto result = send_next(&session);

  EXPECT_EQ(result, 0);

  const auto *msg =
      reinterpret_cast<const messages::data *>(session.state.buffer.data());
  EXPECT_EQ(ntohs(msg->opc), DATA);
  EXPECT_EQ(ntohs(msg->block_num), 43);
}

TEST_F(TestTftpStatic, SendNext_HandlesEmptyFile)
{
  session_t session;
  const auto test_file = create_test_file("");

  session.state.file = std::make_shared<std::fstream>(
      test_file, std::ios::in | std::ios::binary);
  session.state.mode = OCTET;
  session.state.block_num = 0;
  session.state.buffer.resize(sizeof(messages::data));

  const auto result = send_next(&session);

  EXPECT_EQ(result, 0);
  EXPECT_EQ(session.state.block_num, 1);
  EXPECT_EQ(session.state.buffer.size(), sizeof(messages::data));
}

TEST_F(TestTftpStatic, SendNext_PreservesBufferCapacity)
{
  session_t session;
  const auto test_file = create_test_file("Test content");

  session.state.file = std::make_shared<std::fstream>(
      test_file, std::ios::in | std::ios::binary);
  session.state.mode = OCTET;
  session.state.block_num = 0;
  session.state.buffer.resize(sizeof(messages::data));

  send_next(&session);

  // Buffer capacity should be reserved for efficiency
  EXPECT_GE(session.state.buffer.capacity(),
            messages::DATAMSG_MAXLEN + messages::DATALEN);
}

TEST_F(TestTftpStatic, SendNext_HandlesMultipleCallsOnLargeFile)
{
  session_t session;
  // Create a 2KB file
  const std::string content(2048, 'A');
  const auto test_file = create_test_file(content);

  session.state.file = std::make_shared<std::fstream>(
      test_file, std::ios::in | std::ios::binary);
  session.state.mode = OCTET;
  session.state.block_num = 0;
  session.state.buffer.resize(sizeof(messages::data));

  // Should be able to call send_next multiple times
  for (int i = 1; i <= 4; i++)
  {
    const auto result = send_next(&session);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(session.state.block_num, i);
  }
}

// NOLINTEND
