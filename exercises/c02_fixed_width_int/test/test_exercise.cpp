// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include <climits>

#include "drill/fixed_width.h"

TEST(FixedWidthTest, 符号なし8ビット整数の加算で自動wrap)
{
  EXPECT_EQ(add_modulo_256(100, 100), 200);
  EXPECT_EQ(add_modulo_256(200, 100), 44);  // 300 % 256 = 44
  EXPECT_EQ(add_modulo_256(255, 1), 0);     // (255 + 1) % 256 = 0
  EXPECT_EQ(add_modulo_256(0, 0), 0);
}

TEST(FixedWidthTest, 符号付き16ビット整数の加算で飽和)
{
  EXPECT_EQ(saturate_add(100, 200), 300);
  EXPECT_EQ(saturate_add(-100, -200), -300);

  // オーバーフロー時は飽和
  EXPECT_EQ(saturate_add(INT16_MAX, 1000), INT16_MAX);
  EXPECT_EQ(saturate_add(INT16_MIN, -1000), INT16_MIN);
  EXPECT_EQ(saturate_add(32000, 1000), INT16_MAX);
  EXPECT_EQ(saturate_add(-32000, -1000), INT16_MIN);
}

TEST(FixedWidthTest, 最上位ビット判定)
{
  EXPECT_EQ(check_high_bit(0x80), 1);  // 10000000
  EXPECT_EQ(check_high_bit(0xFF), 1);  // 11111111
  EXPECT_EQ(check_high_bit(0x7F), 0);  // 01111111
  EXPECT_EQ(check_high_bit(0x00), 0);  // 00000000
  EXPECT_EQ(check_high_bit(0x81), 1);  // 10000001
  EXPECT_EQ(check_high_bit(0x7E), 0);  // 01111110
}
