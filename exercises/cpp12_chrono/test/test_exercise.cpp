// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/timer.hpp"

using namespace std::chrono_literals;

TEST(ChronoTest, ticksをカウントする)
{
  int ticks = count_ticks(1000ms, 100ms);
  EXPECT_EQ(ticks, 10);

  ticks = count_ticks(500ms, 250ms);
  EXPECT_EQ(ticks, 2);

  ticks = count_ticks(999ms, 100ms);
  EXPECT_EQ(ticks, 9);  // 切り下げ
}

TEST(ChronoTest, 秒をmillisecondsに変換する)
{
  auto ms = seconds_to_ms(1.0);
  EXPECT_EQ(ms.count(), 1000);

  ms = seconds_to_ms(1.5);
  EXPECT_EQ(ms.count(), 1500);

  ms = seconds_to_ms(0.5);
  EXPECT_EQ(ms.count(), 500);
}
