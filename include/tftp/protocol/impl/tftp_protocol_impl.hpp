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
/**
 * @file tftp_protocol_impl.hpp
 * @brief This file declares utilities for the TFTP protocol.
 */
#pragma once
#ifndef TFTP_PROTOCOL_IMPL_HPP
#define TFTP_PROTOCOL_IMPL_HPP
#include "tftp/protocol/tftp_protocol.hpp"
/** @brief TFTP related utilities. */
namespace tftp {
constexpr auto messages::mode_to_str(messages::mode_t mode) -> const char *
{
  switch (mode)
  {
    case NETASCII:
      return "netascii";

    case OCTET:
      return "octet";

    case MAIL:
      return "mail";

    default:
      return "";
  }
}
} // namespace tftp
#endif // TFTP_PROTOCOL_IMPL_HPP
