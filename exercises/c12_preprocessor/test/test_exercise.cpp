// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

extern "C" {
#include "drill/macro_utils.h"
}

TEST(PreprocessorTest, CanPayloadSizeIs8Bytes)
{
  // _Static_assert で既にコンパイル時に確認されているが、テストでも確認する
  EXPECT_EQ(sizeof(CanPayload), 8u);
}

TEST(PreprocessorTest, CanPayloadIsDefinedCorrectly)
{
  CanPayload payload;
  payload.port_id = 3;
  payload.data[0] = 0xFF;

  EXPECT_EQ(payload.port_id, 3);
  EXPECT_EQ(payload.data[0], 0xFF);
}

TEST(PreprocessorTest, DebugModeIsEnabled)
{
  int mode = is_debug_mode();
  EXPECT_EQ(mode, 1);
}

TEST(PreprocessorTest, ValidatesPortId1AsValid)
{
  int result = validate_port_id(1);
  EXPECT_EQ(result, 1);
}

TEST(PreprocessorTest, ValidatesPortId8AsValid)
{
  int result = validate_port_id(8);
  EXPECT_EQ(result, 1);
}

TEST(PreprocessorTest, ValidatesPortId5AsValid)
{
  int result = validate_port_id(5);
  EXPECT_EQ(result, 1);
}

TEST(PreprocessorTest, ValidatesPortId0AsInvalid)
{
  // assert で abort するはずだが、テストからは呼べない場合がある
  // 実装側が assert で NDEBUG 時に無視する設定になっていることを前提とする
  int result = validate_port_id(0);
  EXPECT_EQ(result, 0);
}

TEST(PreprocessorTest, ValidatesPortId9AsInvalid)
{
  // assert で abort するはずだが、テストからは呼べない場合がある
  int result = validate_port_id(9);
  EXPECT_EQ(result, 0);
}

TEST(MacroTest, SquareOfOne)
{
  int x = 1;
  int result = SQUARE(x);
  EXPECT_EQ(result, 1);
}

TEST(MacroTest, SquareOfFive)
{
  int x = 5;
  int result = SQUARE(x);
  EXPECT_EQ(result, 25);
}

TEST(MacroTest, SquareOfNegativeNumber)
{
  int x = -3;
  int result = SQUARE(x);
  EXPECT_EQ(result, 9);
}

TEST(MacroTest, DoubleSquareSquaresVariable)
{
  int x = 2;
  DOUBLE_SQ(x);
  EXPECT_EQ(x, 4);
}

TEST(MacroTest, DoubleSquareOfZero)
{
  int x = 0;
  DOUBLE_SQ(x);
  EXPECT_EQ(x, 0);
}

TEST(MacroTest, DoubleSquareCalledRepeatedly)
{
  int x = 2;
  DOUBLE_SQ(x);
  EXPECT_EQ(x, 4);
  DOUBLE_SQ(x);
  EXPECT_EQ(x, 16);
}
