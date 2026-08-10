// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/bit_ops.h"

/* ===== CAN ID テスト ===== */

TEST(CanIdTest, 組み立てと抽出が往復する)
{
  uint16_t id = can_id_create(3, 5, 7);
  EXPECT_EQ(can_id_extract_type(id), 3);
  EXPECT_EQ(can_id_extract_device(id), 5);
  EXPECT_EQ(can_id_extract_cmd(id), 7);
}

TEST(CanIdTest, ReceiverType_MDの例)
{
  // MD = 1
  uint16_t id = can_id_create(1, 0, 0);
  EXPECT_EQ(can_id_extract_type(id), 1);
  EXPECT_EQ(can_id_extract_device(id), 0);
  EXPECT_EQ(can_id_extract_cmd(id), 0);
}

TEST(CanIdTest, 複合的な値)
{
  // Type=6 (MAINBOARD_PC), Device=15 (max), Cmd=14
  uint16_t id = can_id_create(6, 15, 14);
  EXPECT_EQ(can_id_extract_type(id), 6);
  EXPECT_EQ(can_id_extract_device(id), 15);
  EXPECT_EQ(can_id_extract_cmd(id), 14);
}

TEST(CanIdTest, ReadModifyWrite_Device_だけ変更)
{
  uint16_t id = can_id_create(2, 3, 5);  // Type=2, Device=3, Cmd=5
  id = can_id_set_device(id, 10);       // Device を 3 → 10

  EXPECT_EQ(can_id_extract_type(id), 2);
  EXPECT_EQ(can_id_extract_device(id), 10);  // 変更された
  EXPECT_EQ(can_id_extract_cmd(id), 5);      // 変わらない
}

TEST(CanIdTest, ReadModifyWrite_他のビットを壊さない)
{
  uint16_t id = can_id_create(7, 1, 15);
  id = can_id_set_device(id, 8);

  // Type と Cmd は保存されたままのはず
  EXPECT_EQ(can_id_extract_type(id), 7);
  EXPECT_EQ(can_id_extract_cmd(id), 15);
}

/* ===== レジスタ操作テスト ===== */

TEST(RegisterTest, ビットを立てる)
{
  uint16_t reg = 0x0000;
  reg = register_set_bit(reg, 3);
  EXPECT_EQ(reg, 0x0008);  // bit 3 = 1

  reg = register_set_bit(reg, 7);
  EXPECT_EQ(reg, 0x0088);  // bit 3 と 7
}

TEST(RegisterTest, ビットを落とす)
{
  uint16_t reg = 0xFFFF;
  reg = register_clear_bit(reg, 3);
  EXPECT_EQ(reg, 0xFFF7);  // bit 3 だけ 0

  reg = register_clear_bit(reg, 7);
  EXPECT_EQ(reg, 0xFF77);  // bit 3 と 7
}

TEST(RegisterTest, ビットを読む)
{
  uint16_t reg = 0x0088;  // bit 3 = 1, bit 7 = 1
  EXPECT_EQ(register_read_bit(reg, 3), 1);
  EXPECT_EQ(register_read_bit(reg, 7), 1);
  EXPECT_EQ(register_read_bit(reg, 0), 0);
}

TEST(RegisterTest, ビット範囲を設定_単一ビット)
{
  uint16_t reg = 0x0000;
  reg = register_set_bits(reg, 5, 5, 1);  // bit [5:5] = 1
  EXPECT_EQ(reg, 0x0020);
}

TEST(RegisterTest, ビット範囲を設定_複数ビット)
{
  uint16_t reg = 0x0000;
  reg = register_set_bits(reg, 7, 4, 0x0F);  // bits [7:4] = 0xF
  EXPECT_EQ(reg, 0x00F0);
}

TEST(RegisterTest, ビット範囲を設定_既存値を保持)
{
  uint16_t reg = 0xF00F;  // [15:12] = F, [3:0] = F
  reg = register_set_bits(reg, 7, 4, 0x05);  // [7:4] = 0x5
  EXPECT_EQ(reg, 0xF05F);  // [15:12], [7:4], [3:0] が保存される
}

TEST(RegisterTest, ビット範囲を設定_値のマスキング)
{
  uint16_t reg = 0x0000;
  reg = register_set_bits(reg, 3, 0, 0xFF);  // 4bit 領域に 0xFF を設定（マスクされる）
  EXPECT_EQ(reg, 0x000F);  // 下 4bit だけが 1
}
