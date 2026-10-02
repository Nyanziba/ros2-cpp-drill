// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/math.hpp"

TEST(HeaderSplitTest, AddsIntegers)
{
  EXPECT_EQ(add(2, 3), 5);
  EXPECT_EQ(add(0, 0), 0);
  EXPECT_EQ(add(-1, 1), 0);
}

TEST(HeaderSplitTest, MultipliesIntegers)
{
  EXPECT_EQ(multiply(2, 3), 6);
  EXPECT_EQ(multiply(0, 10), 0);
  EXPECT_EQ(multiply(-2, 3), -6);
}

TEST(HeaderSplitTest, UsesAddAndMultiplyTogether)
{
  int sum = add(10, 20);
  int product = multiply(sum, 2);
  EXPECT_EQ(product, 60);
}
