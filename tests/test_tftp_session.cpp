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
#include "tftp/protocol/tftp_session.hpp"

#include <gtest/gtest.h>

#include <thread>

using namespace tftp;

// =============================================================================
// session_t Constants Tests
// =============================================================================

TEST(SessionConstantsTest, TimeoutValues)
{
  EXPECT_EQ(session_t::TIMEOUT_MIN, session_t::duration(2));
  EXPECT_EQ(session_t::TIMEOUT_MAX, session_t::duration(200));
}

TEST(SessionConstantsTest, InvalidValues)
{
  EXPECT_EQ(session_t::INVALID_TIMER, net::timers::INVALID_TIMER);
  EXPECT_EQ(session_t::INVALID_SOCKET, io::socket::INVALID_SOCKET);
}

// =============================================================================
// session_t::state_t Construction Tests
// =============================================================================

TEST(SessionStateTest, DefaultConstruction)
{
  session_t::state_t state;

  EXPECT_TRUE(state.target.empty());
  EXPECT_TRUE(state.tmp.empty());
  EXPECT_TRUE(state.buffer.empty());
  EXPECT_EQ(state.file, nullptr);
  EXPECT_EQ(state.timer, session_t::INVALID_TIMER);
  EXPECT_EQ(state.socket, session_t::INVALID_SOCKET);
  EXPECT_EQ(state.block_num, 0);
  EXPECT_EQ(state.opc, 0);
  EXPECT_EQ(state.mode, 0);
}

TEST(SessionStateTest, StatisticsInitialization)
{
  session_t::state_t state;

  // avg_rtt should be initialized to TIMEOUT_MAX
  EXPECT_EQ(state.statistics.avg_rtt, session_t::TIMEOUT_MAX);

  // start_time should be initialized to some time in the past
  auto now = session_t::clock::now();
  EXPECT_LT(state.statistics.start_time, now);
}

TEST(SessionStateTest, PathAssignment)
{
  session_t::state_t state;
  state.target = "/tmp/test.txt";
  state.tmp = "/tmp/test.txt.tmp";

  EXPECT_EQ(state.target, "/tmp/test.txt");
  EXPECT_EQ(state.tmp, "/tmp/test.txt.tmp");
}

TEST(SessionStateTest, BufferOperations)
{
  session_t::state_t state;
  state.buffer.resize(512);
  state.buffer[0] = 'A';
  state.buffer[511] = 'Z';

  EXPECT_EQ(state.buffer.size(), 512);
  EXPECT_EQ(state.buffer[0], 'A');
  EXPECT_EQ(state.buffer[511], 'Z');
}

TEST(SessionStateTest, FilePointerAssignment)
{
  session_t::state_t state;
  state.file = std::make_shared<std::fstream>();

  EXPECT_NE(state.file, nullptr);
  EXPECT_EQ(state.file.use_count(), 1);
}

TEST(SessionStateTest, FieldAssignments)
{
  session_t::state_t state;
  state.block_num = 42;
  state.opc = 3;  // DATA
  state.mode = 2; // OCTET

  EXPECT_EQ(state.block_num, 42);
  EXPECT_EQ(state.opc, 3);
  EXPECT_EQ(state.mode, 2);
}

// =============================================================================
// update_statistics Tests
// =============================================================================

TEST(UpdateStatisticsTest, BasicUpdate)
{
  session_t::state_t::statistics_t stats;
  stats.start_time = session_t::clock::now();
  stats.avg_rtt = session_t::duration(100);

  // Sleep for a short time to create measurable RTT
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto &result = session_t::update_statistics(stats);

  // Result should be reference to same stats
  EXPECT_EQ(&result, &stats);

  // avg_rtt should be updated (EWMA: 75% old + 25% new)
  EXPECT_GE(result.avg_rtt, session_t::TIMEOUT_MIN);
  EXPECT_LE(result.avg_rtt, session_t::TIMEOUT_MAX);

  // start_time should be updated to now
  auto now = session_t::clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - result.start_time);
  EXPECT_LT(diff, std::chrono::milliseconds(5));
}

TEST(UpdateStatisticsTest, EWMACalculation)
{
  session_t::state_t::statistics_t stats;
  stats.avg_rtt = session_t::duration(100);
  stats.start_time = session_t::clock::now() - session_t::duration(50);

  session_t::update_statistics(stats);

  // EWMA formula: new_avg = old_avg * 3/4 + rtt * 1/4
  // old_avg = 100, rtt ≈ 50, so new_avg ≈ 100*0.75 + 50*0.25 = 87.5
  EXPECT_GE(stats.avg_rtt.count(), 80);
  EXPECT_LE(stats.avg_rtt.count(), 95);
}

TEST(UpdateStatisticsTest, MinimumClamp)
{
  session_t::state_t::statistics_t stats;
  stats.avg_rtt = session_t::duration(5);
  stats.start_time = session_t::clock::now() - session_t::duration(1);

  session_t::update_statistics(stats);

  // EWMA: new_avg = old_avg * 3/4 + rtt * 1/4 = 5*0.75 + 1*0.25 = 4
  // Should be clamped to TIMEOUT_MIN (2ms) or slightly higher due to timing
  EXPECT_GE(stats.avg_rtt, session_t::TIMEOUT_MIN);
  EXPECT_LE(stats.avg_rtt, session_t::duration(5));
}

TEST(UpdateStatisticsTest, MaximumClamp)
{
  session_t::state_t::statistics_t stats;
  stats.avg_rtt = session_t::TIMEOUT_MAX;
  stats.start_time = session_t::clock::now() - session_t::duration(300);

  session_t::update_statistics(stats);

  // Should be clamped to TIMEOUT_MAX (200ms)
  EXPECT_EQ(stats.avg_rtt, session_t::TIMEOUT_MAX);
}

TEST(UpdateStatisticsTest, MultipleUpdates)
{
  session_t::state_t::statistics_t stats;
  stats.avg_rtt = session_t::duration(100);
  stats.start_time = session_t::clock::now();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  session_t::update_statistics(stats);
  auto rtt1 = stats.avg_rtt;

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  session_t::update_statistics(stats);
  auto rtt2 = stats.avg_rtt;

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  session_t::update_statistics(stats);
  auto rtt3 = stats.avg_rtt;

  // Each update should result in a different RTT
  // (unless by coincidence they're the same after clamping)
  EXPECT_GE(rtt1, session_t::TIMEOUT_MIN);
  EXPECT_GE(rtt2, session_t::TIMEOUT_MIN);
  EXPECT_GE(rtt3, session_t::TIMEOUT_MIN);
}

TEST(UpdateStatisticsTest, VeryShortRTT)
{
  session_t::state_t::statistics_t stats;
  stats.avg_rtt = session_t::duration(100);
  stats.start_time = session_t::clock::now();

  // Call immediately (RTT ≈ 0)
  session_t::update_statistics(stats);

  // EWMA: new_avg = 100*0.75 + 0*0.25 = 75
  // But timing may vary slightly, so allow a range
  EXPECT_GE(stats.avg_rtt, session_t::duration(70));
  EXPECT_LE(stats.avg_rtt, session_t::duration(80));
}

TEST(UpdateStatisticsTest, ReferenceReturn)
{
  session_t::state_t::statistics_t stats;
  stats.avg_rtt = session_t::duration(100);
  stats.start_time = session_t::clock::now();

  auto &result = session_t::update_statistics(stats);

  // Modifying result should modify stats
  result.avg_rtt = session_t::duration(50);
  EXPECT_EQ(stats.avg_rtt, session_t::duration(50));
}

TEST(UpdateStatisticsTest, PreservesAfterClamp)
{
  session_t::state_t::statistics_t stats;
  stats.avg_rtt = session_t::TIMEOUT_MAX;
  stats.start_time = session_t::clock::now() - session_t::duration(300);

  session_t::update_statistics(stats);

  // After clamping to MAX, verify subsequent updates still work
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  session_t::update_statistics(stats);

  // Should still be within valid range
  EXPECT_GE(stats.avg_rtt, session_t::TIMEOUT_MIN);
  EXPECT_LE(stats.avg_rtt, session_t::TIMEOUT_MAX);
}

// =============================================================================
// session_t Construction Tests
// =============================================================================

TEST(SessionTest, DefaultConstruction)
{
  session_t session;

  EXPECT_TRUE(session.state.target.empty());
  EXPECT_TRUE(session.state.buffer.empty());
  EXPECT_EQ(session.state.block_num, 0);
}

TEST(SessionTest, StateAccess)
{
  session_t session;
  session.state.block_num = 10;
  session.state.opc = 3;
  session.state.mode = 2;

  EXPECT_EQ(session.state.block_num, 10);
  EXPECT_EQ(session.state.opc, 3);
  EXPECT_EQ(session.state.mode, 2);
}

TEST(SessionTest, StatisticsAccess)
{
  session_t session;
  session.state.statistics.avg_rtt = session_t::duration(50);

  EXPECT_EQ(session.state.statistics.avg_rtt, session_t::duration(50));
}

// =============================================================================
// Type Alias Tests
// =============================================================================

TEST(TypeAliasTest, ClockType)
{
  EXPECT_TRUE((std::is_same_v<session_t::clock, std::chrono::steady_clock>));
}

TEST(TypeAliasTest, TimestampType)
{
  EXPECT_TRUE((std::is_same_v<session_t::timestamp,
                              std::chrono::steady_clock::time_point>));
}

TEST(TypeAliasTest, DurationType)
{
  EXPECT_TRUE((std::is_same_v<session_t::duration, std::chrono::milliseconds>));
}

TEST(TypeAliasTest, TimerIdType)
{
  EXPECT_TRUE((std::is_same_v<session_t::timer_id, net::timers::timer_id>));
}

TEST(TypeAliasTest, SocketType)
{
  EXPECT_TRUE(
      (std::is_same_v<session_t::socket_type, io::socket::native_socket_type>));
}

// =============================================================================
// Statistics EWMA Edge Cases
// =============================================================================

TEST(StatisticsEWMATest, VeryLargeRTT)
{
  session_t::state_t::statistics_t stats;
  stats.avg_rtt = session_t::duration(100);
  stats.start_time = session_t::clock::now() - session_t::duration(1000);

  session_t::update_statistics(stats);

  // Should be clamped to TIMEOUT_MAX
  EXPECT_EQ(stats.avg_rtt, session_t::TIMEOUT_MAX);
}

TEST(StatisticsEWMATest, GradualIncrease)
{
  session_t::state_t::statistics_t stats;
  stats.avg_rtt = session_t::TIMEOUT_MIN;
  stats.start_time = session_t::clock::now();

  // Simulate gradually increasing RTT
  for (int i = 0; i < 10; i++)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    session_t::update_statistics(stats);
  }

  // avg_rtt should have increased from minimum or stayed at minimum
  // depending on actual measured RTT (which can be very small in tests)
  EXPECT_GE(stats.avg_rtt, session_t::TIMEOUT_MIN);
  EXPECT_LE(stats.avg_rtt, session_t::TIMEOUT_MAX);
}

TEST(StatisticsEWMATest, StartTimeUpdated)
{
  session_t::state_t::statistics_t stats;
  auto initial_time = session_t::clock::now();
  stats.start_time = initial_time;
  stats.avg_rtt = session_t::duration(100);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  session_t::update_statistics(stats);

  // start_time should be updated to approximately now
  EXPECT_GT(stats.start_time, initial_time);
}

// NOLINTEND
