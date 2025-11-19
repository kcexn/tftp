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
 * @file dns.hpp
 * @brief This file defines utilities for working with DNS.
 */
#pragma once
#ifndef TFTP_DNS_HPP
#define TFTP_DNS_HPP
#include <system_error>

#include <netdb.h>
/** @brief Defines internal dns implementation details. */
namespace tftp::dns {
/** @brief dns error conditions. */
enum class errc : int {
  address_family_not_supported = EAI_FAMILY,
  address_family_not_supported_by_host = EAI_ADDRFAMILY,
  resource_unavailable_try_again = EAI_AGAIN,
  bad_flags = EAI_BADFLAGS,
  permanent_failure = EAI_FAIL,
  not_enough_memory = EAI_MEMORY,
  address_not_found = EAI_NODATA,
  name_not_found = EAI_NONAME,
  service_not_found = EAI_SERVICE,
  socket_type_not_supported = EAI_SOCKTYPE,
  system_error = EAI_SYSTEM
};

/** @brief Dns error category. */
class dns_category_impl : public std::error_category {
public:
  /**
   * @brief The error category name.
   * @return The error category name.
   */
  [[nodiscard]] inline auto name() const noexcept -> const char * override;

  /**
   * @brief Generic error messages for DNS errors.
   * @param errv The error value.
   * @return A string describing the error.
   */
  [[nodiscard]] inline auto message(int errv) const -> std::string override;

  /**
   * @brief Error equivalences between dns_error and generic error conditions.
   * @param code The error code.
   * @param condition The error condition to compare against.
   * @return true if the error code is equivalent to the error condition, false
   * otherwise.
   */
  [[nodiscard]] inline auto equivalent(
      int code,
      const std::error_condition &condition) const noexcept -> bool override;
};

/**
 * @brief singleton dns category.
 * @return A reference to the dns_category singleton.
 */
inline auto dns_category() -> const std::error_category &;

/**
 * @brief Construct a standard error code from a dns error condition.
 * @param err The dns error enum value.
 * @return An std::error_code representing the dns error.
 */
inline auto make_error_code(errc err) -> std::error_code;

} // namespace tftp::dns

#include "impl/dns_impl.hpp" // IWYU pragma: export

#endif // TFTP_DNS_HPP
