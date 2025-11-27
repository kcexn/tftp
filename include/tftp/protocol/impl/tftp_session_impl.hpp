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
/** @file tftp_session_impl.hpp
 * @brief This file implements the TFTP session handle.
 */
#pragma once
#ifndef TFTP_SESSION_IMPL_HPP
#define TFTP_SESSION_IMPL_HPP
#include "tftp/protocol/tftp_session.hpp"
/** @brief TFTP related utilities. */
namespace tftp {
inline auto session_t::update_statistics(
    session_t::state_t::statistics_t &statistics) noexcept
    -> session_t::state_t::statistics_t &
{
  using namespace std::chrono;

  auto &[start_time, avg_rtt] = statistics;
  auto now = clock::now();
  auto rtt = duration_cast<duration>(now - start_time);

  avg_rtt = avg_rtt * 3 / 4 + rtt / 4;
  avg_rtt = std::min(avg_rtt, TIMEOUT_MAX);
  avg_rtt = std::max(avg_rtt, TIMEOUT_MIN);

  start_time = now;

  return statistics;
}
} // namespace tftp
#endif // TFTP_SESSION_IMPL_HPP
