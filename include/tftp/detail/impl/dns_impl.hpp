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
 * @file dns_impl.hpp
 * @brief This file defines utilities for working with DNS.
 */
#pragma once
#ifndef TFTP_DNS_IMPL_HPP
#define TFTP_DNS_IMPL_HPP
#include "tftp/detail/dns.hpp"
namespace tftp::dns {
[[nodiscard]] inline auto dns_category_impl::name() const noexcept -> const
    char *
{
  return "dns";
}

[[nodiscard]] inline auto
dns_category_impl::message(int errv) const -> std::string
{
  return gai_strerror(errv);
}

[[nodiscard]] inline auto dns_category_impl::equivalent(
    int code, const std::error_condition &condition) const noexcept -> bool
{
  if (condition.category() == dns_category())
    return condition.value() == code;

  if (condition.category() != std::generic_category())
    return false;

  switch (static_cast<errc>(code))
  {
    case errc::address_family_not_supported:
    case errc::address_family_not_supported_by_host:
      return condition == std::errc::address_family_not_supported;

    case errc::resource_unavailable_try_again:
      return condition == std::errc::resource_unavailable_try_again;

    case errc::bad_flags:
      return condition == std::errc::invalid_argument;

    case errc::permanent_failure:
      return condition == std::errc::state_not_recoverable;

    case errc::not_enough_memory:
      return condition == std::errc::not_enough_memory;

    case errc::service_not_found:
    case errc::address_not_found:
    case errc::name_not_found:
      return condition == std::errc::address_not_available;

    default:
      return false;
  }
}

inline auto dns_category() -> const std::error_category &
{
  static dns_category_impl instance;
  return instance;
}

inline auto make_error_code(errc err) -> std::error_code
{
  if (err == errc::system_error)
    return {errno, std::system_category()};

  return {static_cast<int>(err), dns_category()};
}

} // namespace tftp::dns

namespace std {
/** @brief declares dns::errc as an error condition. */
template <> struct is_error_code_enum<tftp::dns::errc> : true_type {};
} // namespace std

#endif // TFTP_DNS_IMPL_HPP
