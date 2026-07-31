// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/checker.hpp"

TEST(PointerTest, 両方がvalidな場合)
{
  int val = 42;
  int result = 0;
  EXPECT_TRUE(try_read(&val, &result));
  EXPECT_EQ(result, 42);
}

TEST(PointerTest, 読み込みポインタがnullptr)
{
  int result = 0;
  EXPECT_FALSE(try_read(nullptr, &result));
  EXPECT_EQ(result, 0);
}

TEST(PointerTest, 書き込みポインタがnullptr)
{
  int val = 42;
  EXPECT_FALSE(try_read(&val, nullptr));
}

TEST(PointerTest, 両方がnullptr)
{
  EXPECT_FALSE(try_read(nullptr, nullptr));
}
