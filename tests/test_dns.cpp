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
#include "tftp/detail/dns.hpp"
#include "tftp/detail/impl/dns_impl.hpp"
#include <gtest/gtest.h>

using namespace tftp;

TEST(DnsTest, Name) { EXPECT_STREQ(dns::dns_category().name(), "dns"); }

TEST(DnsTest, Message)
{
  std::error_code ec = dns::make_error_code(dns::errc::permanent_failure);
  EXPECT_EQ(ec.message(),
            gai_strerror(static_cast<int>(dns::errc::permanent_failure)));
}

TEST(DnsTest, Equivalent)
{
  EXPECT_TRUE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::address_family_not_supported),
      std::errc::address_family_not_supported));
  EXPECT_TRUE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::address_family_not_supported_by_host),
      std::errc::address_family_not_supported));
  EXPECT_TRUE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::resource_unavailable_try_again),
      std::errc::resource_unavailable_try_again));
  EXPECT_TRUE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::bad_flags), std::errc::invalid_argument));
  EXPECT_TRUE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::permanent_failure),
      std::errc::state_not_recoverable));
  EXPECT_TRUE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::not_enough_memory),
      std::errc::not_enough_memory));
  EXPECT_TRUE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::service_not_found),
      std::errc::address_not_available));
  EXPECT_TRUE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::address_not_found),
      std::errc::address_not_available));
  EXPECT_TRUE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::name_not_found),
      std::errc::address_not_available));
  EXPECT_FALSE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::address_family_not_supported),
      std::errc::invalid_argument));
  EXPECT_FALSE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::system_error), std::errc::invalid_argument));
}

TEST(DnsTest, EquivalentWrongCategory)
{
  std::error_condition condition(static_cast<int>(dns::errc::permanent_failure),
                                 std::system_category());
  EXPECT_FALSE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::permanent_failure), condition));
}

TEST(DnsTest, EquivalentSameCategory)
{
  std::error_condition condition(static_cast<int>(dns::errc::permanent_failure),
                                 dns::dns_category());
  EXPECT_TRUE(dns::dns_category().equivalent(
      static_cast<int>(dns::errc::permanent_failure), condition));
}

TEST(DnsTest, MakeErrorCode)
{
  std::error_code ec = dns::make_error_code(dns::errc::permanent_failure);
  EXPECT_EQ(ec.value(), static_cast<int>(dns::errc::permanent_failure));
  EXPECT_EQ(ec.category(), dns::dns_category());
}

TEST(DnsTest, MakeErrorCodeSystem)
{
  errno = EAFNOSUPPORT;
  std::error_code ec = dns::make_error_code(dns::errc::system_error);
  EXPECT_EQ(ec.value(), EAFNOSUPPORT);
  EXPECT_EQ(ec.category(), std::system_category());
}
// NOLINTEND
