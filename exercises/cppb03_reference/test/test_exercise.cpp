// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/swapper.hpp"

TEST(ReferenceTest, 参照経由のSwap)
{
  int x = 10, y = 20;
  swap_values(x, y);
  EXPECT_EQ(x, 20);
  EXPECT_EQ(y, 10);
}

TEST(ReferenceTest, 参照を返して呼び出し元を変更)
{
  int a = 3, b = 7;
  int & ref = largest(a, b);
  ref = 99;
  EXPECT_EQ(b, 99);  // b への参照が返されたので b が変わる
}

TEST(ReferenceTest, 等しい場合は最初の方)
{
  int a = 5, b = 5;
  int & ref = largest(a, b);
  ref = 100;
  EXPECT_EQ(a, 100);  // 最初の参照が返される
}
