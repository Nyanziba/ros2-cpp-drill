// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "drill/endian_serialize.h"
}

TEST(EndianTest, float_1_0_往復)
{
  uint8_t data[8] = {0};
  write_float_le(1.0f, data, 0);

  // 期待するバイト列: 1.0f = 0x3f800000 (LE)
  EXPECT_EQ(data[0], 0x00);
  EXPECT_EQ(data[1], 0x00);
  EXPECT_EQ(data[2], 0x80);
  EXPECT_EQ(data[3], 0x3f);

  // 復元
  float f = read_float_le(data, 0);
  EXPECT_FLOAT_EQ(f, 1.0f);
}

TEST(EndianTest, float_2_5_往復)
{
  uint8_t data[8] = {0};
  write_float_le(2.5f, data, 0);

  // 期待するバイト列: 2.5f = 0x40200000 (LE)
  EXPECT_EQ(data[0], 0x00);
  EXPECT_EQ(data[1], 0x00);
  EXPECT_EQ(data[2], 0x20);
  EXPECT_EQ(data[3], 0x40);

  // 復元
  float f = read_float_le(data, 0);
  EXPECT_FLOAT_EQ(f, 2.5f);
}

TEST(EndianTest, float_負数_往復)
{
  uint8_t data[8] = {0};
  write_float_le(-1.0f, data, 0);

  // 期待するバイト列: -1.0f = 0xbf800000 (LE)
  EXPECT_EQ(data[0], 0x00);
  EXPECT_EQ(data[1], 0x00);
  EXPECT_EQ(data[2], 0x80);
  EXPECT_EQ(data[3], 0xbf);

  // 復元
  float f = read_float_le(data, 0);
  EXPECT_FLOAT_EQ(f, -1.0f);
}

TEST(EndianTest, float_ゼロ_往復)
{
  uint8_t data[8] = {0};
  write_float_le(0.0f, data, 0);

  // 期待するバイト列: 0.0f = 0x00000000 (LE)
  EXPECT_EQ(data[0], 0x00);
  EXPECT_EQ(data[1], 0x00);
  EXPECT_EQ(data[2], 0x00);
  EXPECT_EQ(data[3], 0x00);

  // 復元
  float f = read_float_le(data, 0);
  EXPECT_FLOAT_EQ(f, 0.0f);
}

TEST(EndianTest, float_オフセット付き_往復)
{
  uint8_t data[8] = {0};
  // offset 2 から書き込む
  write_float_le(1.0f, data, 2);

  EXPECT_EQ(data[0], 0x00);
  EXPECT_EQ(data[1], 0x00);
  EXPECT_EQ(data[2], 0x00);  // offset 2
  EXPECT_EQ(data[3], 0x00);
  EXPECT_EQ(data[4], 0x80);
  EXPECT_EQ(data[5], 0x3f);

  // 復元
  float f = read_float_le(data, 2);
  EXPECT_FLOAT_EQ(f, 1.0f);
}

TEST(EndianTest, uint32_往復)
{
  uint8_t data[8] = {0};
  uint32_t original = 0x12345678;
  write_uint32_le(original, data, 0);

  // 期待するバイト列: 0x12345678 (LE)
  EXPECT_EQ(data[0], 0x78);
  EXPECT_EQ(data[1], 0x56);
  EXPECT_EQ(data[2], 0x34);
  EXPECT_EQ(data[3], 0x12);

  // 復元
  uint32_t restored = read_uint32_le(data, 0);
  EXPECT_EQ(restored, original);
}

TEST(EndianTest, uint32_オフセット付き_往復)
{
  uint8_t data[8] = {0};
  uint32_t original = 0xdeadbeef;
  write_uint32_le(original, data, 2);

  EXPECT_EQ(data[2], 0xef);
  EXPECT_EQ(data[3], 0xbe);
  EXPECT_EQ(data[4], 0xad);
  EXPECT_EQ(data[5], 0xde);

  // 復元
  uint32_t restored = read_uint32_le(data, 2);
  EXPECT_EQ(restored, original);
}

TEST(EndianTest, uint32_ゼロ_往復)
{
  uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  write_uint32_le(0, data, 1);

  EXPECT_EQ(data[1], 0x00);
  EXPECT_EQ(data[2], 0x00);
  EXPECT_EQ(data[3], 0x00);
  EXPECT_EQ(data[4], 0x00);

  uint32_t restored = read_uint32_le(data, 1);
  EXPECT_EQ(restored, 0u);
}

TEST(EndianTest, 速度目標コマンド構築)
{
  uint8_t payload[8];
  build_speed_target_command(3, 1.0f, payload);

  // Byte 0: ポート ID
  EXPECT_EQ(payload[0], 3);

  // Byte 1-4: 1.0f (LE)
  EXPECT_EQ(payload[1], 0x00);
  EXPECT_EQ(payload[2], 0x00);
  EXPECT_EQ(payload[3], 0x80);
  EXPECT_EQ(payload[4], 0x3f);

  // Byte 5-7: 0 埋め
  EXPECT_EQ(payload[5], 0);
  EXPECT_EQ(payload[6], 0);
  EXPECT_EQ(payload[7], 0);
}

TEST(EndianTest, 速度目標コマンド構築_負の速度)
{
  uint8_t payload[8];
  build_speed_target_command(1, -2.5f, payload);

  // Byte 0: ポート ID
  EXPECT_EQ(payload[0], 1);

  // Byte 1-4: -2.5f (LE) = 0xc0200000
  EXPECT_EQ(payload[1], 0x00);
  EXPECT_EQ(payload[2], 0x00);
  EXPECT_EQ(payload[3], 0x20);
  EXPECT_EQ(payload[4], 0xc0);

  // Byte 5-7: 0 埋め
  EXPECT_EQ(payload[5], 0);
  EXPECT_EQ(payload[6], 0);
  EXPECT_EQ(payload[7], 0);
}

TEST(EndianTest, 速度目標コマンド往路)
{
  uint8_t payload[8];
  float original_speed = 5.0f;
  uint8_t original_port = 7;

  build_speed_target_command(original_port, original_speed, payload);

  // 復元
  uint8_t extracted_port = payload[0];
  float extracted_speed = read_float_le(payload, 1);

  EXPECT_EQ(extracted_port, original_port);
  EXPECT_FLOAT_EQ(extracted_speed, original_speed);
}
