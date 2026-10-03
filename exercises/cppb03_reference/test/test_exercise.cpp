// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/swapper.hpp"

TEST(ReferenceTest, SwapsTwoVariablesByReference)
{
  int x = 10, y = 20;
  swap_values(x, y);
  EXPECT_EQ(x, 20);
  EXPECT_EQ(y, 10);
}

TEST(ReferenceTest, ReturnsReferenceSoCallerCanModify)
{
  int a = 3, b = 7;
  int & ref = largest(a, b);
  ref = 99;
  EXPECT_EQ(b, 99);  // b への参照が返されたので b が変わる
}

TEST(ReferenceTest, ReturnsFirstWhenEqual)
{
  int a = 5, b = 5;
  int & ref = largest(a, b);
  ref = 100;
  EXPECT_EQ(a, 100);  // 最初の参照が返される
}
