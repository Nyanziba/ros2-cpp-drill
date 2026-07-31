// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/counter.hpp"

TEST(CounterTest, add_oneが1増やす)
{
  EXPECT_EQ(add_one(0), 1);
  EXPECT_EQ(add_one(5), 6);
  EXPECT_EQ(add_one(-1), 0);
}

TEST(CounterTest, next_idが順番に増える)
{
  EXPECT_EQ(next_id(), 1);
  EXPECT_EQ(next_id(), 2);
  EXPECT_EQ(next_id(), 3);
}
