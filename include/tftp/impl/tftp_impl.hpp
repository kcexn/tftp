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
 * @file tftp_impl.hpp
 * @brief This file defines TFTP application logic.
 */
#pragma once
#ifndef TFTP_IMPL_HPP
#define TFTP_IMPL_HPP
#include "tftp/detail/dns.hpp"
#include "tftp/tftp.hpp"
/** @brief TFTP client namespace. */
namespace tftp::client {

template <typename Receiver>
auto connect_t::state_t<Receiver>::start() noexcept -> void
{
  using namespace stdexec;

  // Resolve IPv4 address unless otherwise configured.
  struct addrinfo hints = {.ai_family = AF_INET,
                           .ai_socktype = SOCK_DGRAM,
                           .ai_protocol = IPPROTO_UDP};

  struct addrinfo *result = nullptr;
  auto err = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
  if (err)
  {
    return set_error(std::move(receiver),
                     make_error_code(static_cast<dns::errc>(err)));
  }

  if (result == nullptr)
  {
    return set_error(std::move(receiver),
                     make_error_code(dns::errc::address_not_found));
  }

  socket_address<sockaddr_in6> address =
      socket_address<sockaddr_in>(result->ai_addr, result->ai_addrlen);
  freeaddrinfo(result);

  set_value(std::move(receiver), std::move(address));
}

template <typename Receiver>
auto connect_t::connect(Receiver &&receiver) noexcept -> state_t<Receiver>
{
  return {.hostname = std::move(hostname),
          .port = std::move(port),
          .receiver = std::forward<Receiver>(receiver),
          .ctx = ctx};
}
} // namespace tftp::client.
#endif // TFTP_IMPL_HPP
