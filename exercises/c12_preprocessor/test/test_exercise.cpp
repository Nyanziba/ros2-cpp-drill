// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

extern "C" {
#include "drill/macro_utils.h"
}

TEST(PreprocessorTest, CanPayloadのサイズは8バイト)
{
  // _Static_assert で既にコンパイル時に確認されているが、テストでも確認する
  EXPECT_EQ(sizeof(CanPayload), 8u);
}

TEST(PreprocessorTest, CanPayload構造体が正しく定義されている)
{
  CanPayload payload;
  payload.port_id = 3;
  payload.data[0] = 0xFF;

  EXPECT_EQ(payload.port_id, 3);
  EXPECT_EQ(payload.data[0], 0xFF);
}

TEST(PreprocessorTest, デバッグモードが有効)
{
  int mode = is_debug_mode();
  EXPECT_EQ(mode, 1);
}

TEST(PreprocessorTest, ポート検証_有効なID1)
{
  int result = validate_port_id(1);
  EXPECT_EQ(result, 1);
}

TEST(PreprocessorTest, ポート検証_有効なID8)
{
  int result = validate_port_id(8);
  EXPECT_EQ(result, 1);
}

TEST(PreprocessorTest, ポート検証_有効なID5)
{
  int result = validate_port_id(5);
  EXPECT_EQ(result, 1);
}

TEST(PreprocessorTest, ポート検証_無効なID0)
{
  // assert で abort するはずだが、テストからは呼べない場合がある
  // 実装側が assert で NDEBUG 時に無視する設定になっていることを前提とする
  int result = validate_port_id(0);
  EXPECT_EQ(result, 0);
}

TEST(PreprocessorTest, ポート検証_無効なID9)
{
  // assert で abort するはずだが、テストからは呼べない場合がある
  int result = validate_port_id(9);
  EXPECT_EQ(result, 0);
}

TEST(MacroTest, SQUARE_マクロ_1_0)
{
  int x = 1;
  int result = SQUARE(x);
  EXPECT_EQ(result, 1);
}

TEST(MacroTest, SQUARE_マクロ_5)
{
  int x = 5;
  int result = SQUARE(x);
  EXPECT_EQ(result, 25);
}

TEST(MacroTest, SQUARE_マクロ_負数)
{
  int x = -3;
  int result = SQUARE(x);
  EXPECT_EQ(result, 9);
}

TEST(MacroTest, DOUBLE_SQ_マクロ)
{
  int x = 2;
  DOUBLE_SQ(x);
  EXPECT_EQ(x, 4);
}

TEST(MacroTest, DOUBLE_SQ_マクロ_0)
{
  int x = 0;
  DOUBLE_SQ(x);
  EXPECT_EQ(x, 0);
}

TEST(MacroTest, DOUBLE_SQ_マクロ_複数回呼び出し)
{
  int x = 2;
  DOUBLE_SQ(x);
  EXPECT_EQ(x, 4);
  DOUBLE_SQ(x);
  EXPECT_EQ(x, 16);
}
