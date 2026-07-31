// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/buffer.hpp"

TEST(MoveTest, ムーブコンストラクタがデータを転送する)
{
  Buffer buf1(10);
  int* original_ptr = buf1.data();
  int size1 = buf1.size();

  Buffer buf2 = std::move(buf1);

  EXPECT_EQ(buf2.size(), size1);
  EXPECT_EQ(buf2.data(), original_ptr);
  EXPECT_TRUE(buf1.empty());
  EXPECT_EQ(buf1.data(), nullptr);
}

TEST(MoveTest, ムーブ代入がデータを転送する)
{
  Buffer buf1(10);
  Buffer buf2(5);
  int* original_ptr = buf1.data();
  int original_size = buf1.size();

  buf2 = std::move(buf1);

  EXPECT_EQ(buf2.size(), original_size);
  EXPECT_EQ(buf2.data(), original_ptr);
  EXPECT_TRUE(buf1.empty());
}
