// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/struct_align.h"

TEST(StructAlignTest, Point2D_のサイズは4バイト)
{
  EXPECT_EQ(get_sizeof_point2d(), 4);
}

TEST(StructAlignTest, Point2D_の_x_のオフセットは0)
{
  EXPECT_EQ(get_offset_point2d_x(), 0);
}

TEST(StructAlignTest, Point2D_の_y_のオフセットは2)
{
  EXPECT_EQ(get_offset_point2d_y(), 2);
}

TEST(StructAlignTest, RGB_のサイズは3バイト)
{
  EXPECT_EQ(get_sizeof_rgb(), 3);
}

TEST(StructAlignTest, RGB_の_r_のオフセットは0)
{
  EXPECT_EQ(get_offset_rgb_r(), 0);
}

TEST(StructAlignTest, RGB_の_g_のオフセットは1)
{
  EXPECT_EQ(get_offset_rgb_g(), 1);
}

TEST(StructAlignTest, RGB_の_b_のオフセットは2)
{
  EXPECT_EQ(get_offset_rgb_b(), 2);
}

TEST(StructAlignTest, PackedData_のサイズは24バイト)
{
  EXPECT_EQ(get_sizeof_packed_data(), 24);
}

TEST(StructAlignTest, PackedData_の_flag_のオフセットは0)
{
  EXPECT_EQ(get_offset_packed_data_flag(), 0);
}

TEST(StructAlignTest, PackedData_の_id_のオフセットは8)
{
  EXPECT_EQ(get_offset_packed_data_id(), 8);
}

TEST(StructAlignTest, PackedData_の_counter_のオフセットは16)
{
  EXPECT_EQ(get_offset_packed_data_counter(), 16);
}
