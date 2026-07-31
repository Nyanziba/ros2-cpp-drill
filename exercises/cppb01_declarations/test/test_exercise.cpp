// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/reader.hpp"

TEST(DeclarationsTest, ConstPtrは読み取り専用)
{
  int value = 42;
  EXPECT_EQ(read_value(&value), 42);
}

TEST(DeclarationsTest, 非ConstPtrは変更可能)
{
  int value = 10;
  modify_value(&value, 99);
  EXPECT_EQ(value, 99);
}

TEST(DeclarationsTest, 戻り値もConstPtrで正しい)
{
  int orig = 7;
  const int * p = get_constant_ptr(&orig);
  EXPECT_EQ(*p, 7);
}
