// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/struct_align.h"

TEST(StructAlignTest, Point2dSizeIs4Bytes)
{
  EXPECT_EQ(get_sizeof_point2d(), 4);
}

TEST(StructAlignTest, Point2dXOffsetIs0)
{
  EXPECT_EQ(get_offset_point2d_x(), 0);
}

TEST(StructAlignTest, Point2dYOffsetIs2)
{
  EXPECT_EQ(get_offset_point2d_y(), 2);
}

TEST(StructAlignTest, RgbSizeIs3Bytes)
{
  EXPECT_EQ(get_sizeof_rgb(), 3);
}

TEST(StructAlignTest, RgbROffsetIs0)
{
  EXPECT_EQ(get_offset_rgb_r(), 0);
}

TEST(StructAlignTest, RgbGOffsetIs1)
{
  EXPECT_EQ(get_offset_rgb_g(), 1);
}

TEST(StructAlignTest, RgbBOffsetIs2)
{
  EXPECT_EQ(get_offset_rgb_b(), 2);
}

TEST(StructAlignTest, PackedDataSizeIs24Bytes)
{
  EXPECT_EQ(get_sizeof_packed_data(), 24);
}

TEST(StructAlignTest, PackedDataFlagOffsetIs0)
{
  EXPECT_EQ(get_offset_packed_data_flag(), 0);
}

TEST(StructAlignTest, PackedDataIdOffsetIs8)
{
  EXPECT_EQ(get_offset_packed_data_id(), 8);
}

TEST(StructAlignTest, PackedDataCounterOffsetIs16)
{
  EXPECT_EQ(get_offset_packed_data_counter(), 16);
}
