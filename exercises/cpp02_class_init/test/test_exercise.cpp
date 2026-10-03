// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/stopwatch.hpp"

TEST(StopwatchTest, ConstructorSetsMaxTime)
{
  Stopwatch sw(5000);
  EXPECT_EQ(sw.max_time(), 5000);
}

TEST(StopwatchTest, ElapsedTimeIsZeroInitially)
{
  Stopwatch sw(5000);
  EXPECT_EQ(sw.elapsed(), 0);
}

TEST(StopwatchTest, AdvanceIncreasesElapsedTime)
{
  Stopwatch sw(5000);
  sw.advance(100);
  EXPECT_EQ(sw.elapsed(), 100);

  sw.advance(200);
  EXPECT_EQ(sw.elapsed(), 300);
}

TEST(StopwatchTest, ConstMemberFunctionsReturnValues)
{
  const Stopwatch sw(3000);
  EXPECT_EQ(sw.max_time(), 3000);
  EXPECT_EQ(sw.elapsed(), 0);
}
