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
#include "tftp/protocol/tftp_protocol.hpp"

#include <gtest/gtest.h>

#include <arpa/inet.h>

using namespace tftp;

// =============================================================================
// errors::errstr Tests
// =============================================================================

TEST(TFTP_Protocol_Test, ErrStrCoverage)
{
  using enum messages::error_t;

  EXPECT_STREQ(errors::errstr(ACCESS_VIOLATION).data(), "Access violation.");
  EXPECT_STREQ(errors::errstr(FILE_NOT_FOUND).data(), "File not found.");
  EXPECT_STREQ(errors::errstr(DISK_FULL).data(), "Disk full.");
  EXPECT_STREQ(errors::errstr(NO_SUCH_USER).data(), "No such user.");
  EXPECT_STREQ(errors::errstr(UNKNOWN_TID).data(), "Unknown TID.");
  EXPECT_STREQ(errors::errstr(ILLEGAL_OPERATION).data(), "Illegal operation.");
  EXPECT_STREQ(errors::errstr(TIMED_OUT).data(), "Timed out.");
  EXPECT_STREQ(errors::errstr(NOT_DEFINED).data(), "Not defined.");
  EXPECT_STREQ(errors::errstr(FILE_ALREADY_EXISTS).data(),
               "File already exists.");
  EXPECT_STREQ(errors::errstr(99).data(), "Not defined.");
}

// =============================================================================
// mode_to_str Tests
// =============================================================================

TEST(ModeToStrTest, NetasciiMode)
{
  auto mode_num = messages::NETASCII;
  const char *mode = messages::mode_to_str(mode_num);
  EXPECT_STREQ(mode, "netascii");
}

TEST(ModeToStrTest, OctetMode)
{
  auto mode_num = messages::OCTET;
  const char *mode = messages::mode_to_str(mode_num);
  EXPECT_STREQ(mode, "octet");
}

TEST(ModeToStrTest, MailMode)
{
  auto mode_num = messages::MAIL;
  const char *mode = messages::mode_to_str(mode_num);
  EXPECT_STREQ(mode, "mail");
}

TEST(ModeToStrTest, InvalidMode)
{
  auto mode_num = static_cast<messages::mode_t>(99);
  const char *mode = messages::mode_to_str(mode_num);
  EXPECT_STREQ(mode, "");
}

TEST(ModeToStrTest, ZeroMode)
{
  auto mode_num = static_cast<messages::mode_t>(0);
  const char *mode = messages::mode_to_str(mode_num);
  EXPECT_STREQ(mode, "");
}

// =============================================================================
// errors::msg Tests
// =============================================================================

TEST(ErrorsMsgTest, BasicErrorMessage)
{
  auto buf = errors::msg(messages::FILE_NOT_FOUND, "File not found.");

  // Check buffer size
  EXPECT_EQ(buf.size(),
            sizeof(messages::error) + 16); // "File not found." + null

  // Check opcode (network byte order)
  const auto *err = reinterpret_cast<const messages::error *>(buf.data());
  EXPECT_EQ(ntohs(err->opc), messages::ERROR);
  EXPECT_EQ(ntohs(err->error), messages::FILE_NOT_FOUND);

  // Check message string
  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "File not found.");
}

TEST(ErrorsMsgTest, EmptyMessage)
{
  auto buf = errors::msg(messages::NOT_DEFINED, "");

  EXPECT_EQ(buf.size(), sizeof(messages::error) + 1);

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "");
}

TEST(ErrorsMsgTest, LongMessage)
{
  const char long_msg[] = "This is a very long error message that contains "
                          "many characters to test buffer handling.";
  auto buf = errors::msg(messages::ACCESS_VIOLATION, long_msg);

  EXPECT_EQ(buf.size(), sizeof(messages::error) + sizeof(long_msg));

  const auto *err = reinterpret_cast<const messages::error *>(buf.data());
  EXPECT_EQ(ntohs(err->opc), messages::ERROR);
  EXPECT_EQ(ntohs(err->error), messages::ACCESS_VIOLATION);

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, long_msg);
}

TEST(ErrorsMsgTest, AllErrorCodes)
{
  // Test that msg() works with all defined error codes
  std::vector<std::uint16_t> error_codes = {
      messages::NOT_DEFINED,         messages::FILE_NOT_FOUND,
      messages::ACCESS_VIOLATION,    messages::DISK_FULL,
      messages::ILLEGAL_OPERATION,   messages::UNKNOWN_TID,
      messages::FILE_ALREADY_EXISTS, messages::NO_SUCH_USER,
  };

  for (auto code : error_codes)
  {
    auto buf = errors::msg(code, "Test error");

    const auto *err = reinterpret_cast<const messages::error *>(buf.data());
    EXPECT_EQ(ntohs(err->opc), messages::ERROR);
    EXPECT_EQ(ntohs(err->error), code);

    const char *msg = buf.data() + sizeof(messages::error);
    EXPECT_STREQ(msg, "Test error");
  }
}

TEST(ErrorsMsgTest, MessageWithSpecialCharacters)
{
  auto buf = errors::msg(messages::ACCESS_VIOLATION, "Path: /root/file.txt");

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "Path: /root/file.txt");
}

TEST(ErrorsMsgTest, NullTerminatorIncluded)
{
  auto buf = errors::msg(messages::FILE_NOT_FOUND, "Test");

  // Verify null terminator is included
  EXPECT_EQ(buf.size(), sizeof(messages::error) + 5); // "Test\0"
  EXPECT_EQ(buf[buf.size() - 1], '\0');
}

// =============================================================================
// Pre-formatted Error Packet Tests
// =============================================================================

TEST(ErrorPacketTest, NotImplemented)
{
  auto &buf = errors::not_implemented();

  const auto *err = reinterpret_cast<const messages::error *>(buf.data());
  EXPECT_EQ(ntohs(err->opc), messages::ERROR);
  EXPECT_EQ(ntohs(err->error), messages::NOT_DEFINED);

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "Not implemented.");
}

TEST(ErrorPacketTest, TimedOut)
{
  auto &buf = errors::timed_out();

  const auto *err = reinterpret_cast<const messages::error *>(buf.data());
  EXPECT_EQ(ntohs(err->opc), messages::ERROR);
  EXPECT_EQ(ntohs(err->error), messages::NOT_DEFINED);

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "Timed Out");
}

TEST(ErrorPacketTest, AccessViolation)
{
  auto &buf = errors::access_violation();

  const auto *err = reinterpret_cast<const messages::error *>(buf.data());
  EXPECT_EQ(ntohs(err->opc), messages::ERROR);
  EXPECT_EQ(ntohs(err->error), messages::ACCESS_VIOLATION);

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "Access violation.");
}

TEST(ErrorPacketTest, FileNotFound)
{
  auto &buf = errors::file_not_found();

  const auto *err = reinterpret_cast<const messages::error *>(buf.data());
  EXPECT_EQ(ntohs(err->opc), messages::ERROR);
  EXPECT_EQ(ntohs(err->error), messages::FILE_NOT_FOUND);

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "File not found.");
}

TEST(ErrorPacketTest, DiskFull)
{
  auto &buf = errors::disk_full();

  const auto *err = reinterpret_cast<const messages::error *>(buf.data());
  EXPECT_EQ(ntohs(err->opc), messages::ERROR);
  EXPECT_EQ(ntohs(err->error), messages::DISK_FULL);

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "No space available.");
}

TEST(ErrorPacketTest, UnknownTID)
{
  auto &buf = errors::unknown_tid();

  const auto *err = reinterpret_cast<const messages::error *>(buf.data());
  EXPECT_EQ(ntohs(err->opc), messages::ERROR);
  EXPECT_EQ(ntohs(err->error), messages::UNKNOWN_TID);

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "Unknown TID.");
}

TEST(ErrorPacketTest, NoSuchUser)
{
  auto &buf = errors::no_such_user();

  const auto *err = reinterpret_cast<const messages::error *>(buf.data());
  EXPECT_EQ(ntohs(err->opc), messages::ERROR);
  EXPECT_EQ(ntohs(err->error), messages::NO_SUCH_USER);

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "No such user.");
}

TEST(ErrorPacketTest, IllegalOperation)
{
  auto &buf = errors::illegal_operation();

  const auto *err = reinterpret_cast<const messages::error *>(buf.data());
  EXPECT_EQ(ntohs(err->opc), messages::ERROR);
  EXPECT_EQ(ntohs(err->error), messages::ILLEGAL_OPERATION);

  const char *msg = buf.data() + sizeof(messages::error);
  EXPECT_STREQ(msg, "Illegal operation.");
}

// =============================================================================
// Pre-formatted Error Packet Consistency Tests
// =============================================================================

TEST(ErrorPacketConsistencyTest, MultipleCalls)
{
  // Ensure calling the same function multiple times returns the same buffer
  auto &buf1 = errors::file_not_found();
  auto &buf2 = errors::file_not_found();

  EXPECT_EQ(&buf1, &buf2); // Should return same static buffer
}

TEST(ErrorPacketConsistencyTest, AllPacketsAreStatic)
{
  // Verify all error packet functions return static buffers
  auto &buf1 = errors::not_implemented();
  auto &buf2 = errors::not_implemented();
  EXPECT_EQ(&buf1, &buf2);

  auto &buf3 = errors::timed_out();
  auto &buf4 = errors::timed_out();
  EXPECT_EQ(&buf3, &buf4);

  auto &buf5 = errors::access_violation();
  auto &buf6 = errors::access_violation();
  EXPECT_EQ(&buf5, &buf6);
}

// =============================================================================
// Protocol Constants Tests
// =============================================================================

TEST(ProtocolConstantsTest, OpcodeValues)
{
  EXPECT_EQ(messages::RRQ, 1);
  EXPECT_EQ(messages::WRQ, 2);
  EXPECT_EQ(messages::DATA, 3);
  EXPECT_EQ(messages::ACK, 4);
  EXPECT_EQ(messages::ERROR, 5);
}

TEST(ProtocolConstantsTest, ModeValues)
{
  EXPECT_EQ(messages::NETASCII, 1);
  EXPECT_EQ(messages::OCTET, 2);
  EXPECT_EQ(messages::MAIL, 3);
}

TEST(ProtocolConstantsTest, ErrorCodeValues)
{
  EXPECT_EQ(messages::NOT_DEFINED, 0);
  EXPECT_EQ(messages::FILE_NOT_FOUND, 1);
  EXPECT_EQ(messages::ACCESS_VIOLATION, 2);
  EXPECT_EQ(messages::DISK_FULL, 3);
  EXPECT_EQ(messages::ILLEGAL_OPERATION, 4);
  EXPECT_EQ(messages::UNKNOWN_TID, 5);
  EXPECT_EQ(messages::FILE_ALREADY_EXISTS, 6);
  EXPECT_EQ(messages::NO_SUCH_USER, 7);
}

TEST(ProtocolConstantsTest, DataLength)
{
  EXPECT_EQ(messages::DATALEN, 512UL);
  EXPECT_EQ(messages::DATAMSG_MAXLEN, sizeof(messages::data) + 512);
}

// =============================================================================
// Constexpr Evaluation Tests
// =============================================================================

TEST(ConstexprTest, ModeToStrIsConstexpr)
{
  // Verify mode_to_str can be evaluated at compile time
  static_assert(messages::mode_to_str(messages::NETASCII)[0] == 'n');
  static_assert(messages::mode_to_str(messages::OCTET)[0] == 'o');
  static_assert(messages::mode_to_str(messages::MAIL)[0] == 'm');
  static_assert(messages::mode_to_str(static_cast<messages::mode_t>(99))[0] ==
                '\0');
}

TEST(ConstexprTest, ErrorsMsgIsConstexpr)
{
  // Verify errors::msg can be evaluated at compile time
  static constexpr auto buf = errors::msg(messages::FILE_NOT_FOUND, "Test");
  static_assert(buf.size() == sizeof(messages::error) + 5);
}

TEST(ConstexprTest, ErrorsErrStrIsConstexpr)
{
  // Verify errors::errstr can be evaluated at compile time
  static_assert(errors::errstr(messages::FILE_NOT_FOUND)[0] == 'F');
  static_assert(errors::errstr(messages::ACCESS_VIOLATION)[0] == 'A');
}

// NOLINTEND
