// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/counter.hpp"

TEST(StaticTest, 関数内Staticは値を保持)
{
  // static なので、関数を抜けても値が残ります。
  EXPECT_EQ(next_id(), 1);
  EXPECT_EQ(next_id(), 2);
  EXPECT_EQ(next_id(), 3);
}

TEST(StaticTest, クラスStaticメンバは共有される)
{
  IdGenerator::reset();

  IdGenerator gen1;
  EXPECT_EQ(gen1.id(), 1);
  EXPECT_EQ(gen1.id(), 2);

  // gen2 は別のインスタンスだが、count_ は共有されているので 3 から続きます。
  IdGenerator gen2;
  EXPECT_EQ(gen2.id(), 3);

  // インスタンス無しでも読めます。
  EXPECT_EQ(IdGenerator::get_count(), 3);
}

TEST(StaticTest, 関数内StaticはResetの影響を受けない)
{
  // next_id() の static は IdGenerator::reset() とは無関係です。
  // 1 つ目のテストで 3 まで進んでいるので、ここは 4 から続きます。
  IdGenerator::reset();
  EXPECT_EQ(next_id(), 4);
  EXPECT_EQ(IdGenerator::get_count(), 0);
}
