// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

// 2 回 include します。ヘッダガードが無いと、ここでコンパイルが止まります。
#include "drill/counter.h"
#include "drill/counter.h"

TEST(SplitCompileTest, カウンタが進む)
{
  struct Counter c;
  counter_init(&c, 10, 3);
  EXPECT_EQ(counter_value(&c), 10);
  counter_advance(&c);
  EXPECT_EQ(counter_value(&c), 13);
  counter_advance(&c);
  EXPECT_EQ(counter_value(&c), 16);
}

TEST(SplitCompileTest, 負のstepでも動く)
{
  struct Counter c;
  counter_init(&c, 0, -5);
  counter_advance(&c);
  EXPECT_EQ(counter_value(&c), -5);
}

TEST(SplitCompileTest, ファイルスコープStaticが呼び出し回数を数える)
{
  // 上の 2 つのテストでは呼んでいないので、ここが 1 回目から始まります。
  EXPECT_EQ(counter_call_count(), 1);
  EXPECT_EQ(counter_call_count(), 2);
  EXPECT_EQ(counter_call_count(), 3);
}
