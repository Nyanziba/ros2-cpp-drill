// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/pointer.h"

TEST(PointerTest, 2つの変数を交換する)
{
  int a = 10;
  int b = 20;
  EXPECT_EQ(swap_values(&a, &b), 0);
  EXPECT_EQ(a, 20);
  EXPECT_EQ(b, 10);
}

TEST(PointerTest, swap時にNULLチェック)
{
  int a = 10;
  EXPECT_EQ(swap_values(NULL, &a), -1);
  EXPECT_EQ(swap_values(&a, NULL), -1);
  EXPECT_EQ(swap_values(NULL, NULL), -1);
  EXPECT_EQ(a, 10);  // 変更されていない
}

TEST(PointerTest, multiplyで値を2倍にする)
{
  int result = 0;
  EXPECT_EQ(multiply(5, &result), 0);
  EXPECT_EQ(result, 10);

  EXPECT_EQ(multiply(-3, &result), 0);
  EXPECT_EQ(result, -6);

  EXPECT_EQ(multiply(0, &result), 0);
  EXPECT_EQ(result, 0);
}

TEST(PointerTest, multiplyでNULLチェック)
{
  EXPECT_EQ(multiply(5, NULL), -1);
}

TEST(PointerTest, tripleで値を3倍にする)
{
  int p = 5;
  EXPECT_EQ(triple_pointer(&p), 0);
  EXPECT_EQ(p, 15);

  p = -2;
  EXPECT_EQ(triple_pointer(&p), 0);
  EXPECT_EQ(p, -6);
}

TEST(PointerTest, tripleでNULLチェック)
{
  EXPECT_EQ(triple_pointer(NULL), -1);
}
