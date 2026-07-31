// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/stopwatch.hpp"

TEST(StopwatchTest, コンストラクタでmaxTimeが設定される)
{
  Stopwatch sw(5000);
  EXPECT_EQ(sw.max_time(), 5000);
}

TEST(StopwatchTest, 初期状態では経過時間は0)
{
  Stopwatch sw(5000);
  EXPECT_EQ(sw.elapsed(), 0);
}

TEST(StopwatchTest, advanceで経過時間が増える)
{
  Stopwatch sw(5000);
  sw.advance(100);
  EXPECT_EQ(sw.elapsed(), 100);

  sw.advance(200);
  EXPECT_EQ(sw.elapsed(), 300);
}

TEST(StopwatchTest, constメンバ関数で値を取得できる)
{
  const Stopwatch sw(3000);
  EXPECT_EQ(sw.max_time(), 3000);
  EXPECT_EQ(sw.elapsed(), 0);
}
