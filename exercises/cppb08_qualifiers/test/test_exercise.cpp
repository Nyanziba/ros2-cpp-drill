// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <type_traits>

#include "drill/qualifiers.hpp"

TEST(QualifiersTest, Explicitが暗黙変換を止める)
{
  static_assert(
    !std::is_convertible_v<double, Meters>,
    "Meters のコンストラクタに explicit を付けてください");

  // explicit を付けても、明示的な構築はもちろん通ります。
  Meters m(1.5);
  EXPECT_DOUBLE_EQ(m.value(), 1.5);
}

TEST(QualifiersTest, Const関数はConstオブジェクトから呼べる)
{
  const Meters m(2.5);
  EXPECT_DOUBLE_EQ(m.value(), 2.5);
}

TEST(QualifiersTest, Constexprはコンパイル時に評価される)
{
  static_assert(square(5) == 25, "square に constexpr を付けてください");
  static_assert(square(0) == 0, "square(0) は 0 です");

  // constexpr にしても、実行時の引数で普通に呼べます。
  int n = 7;
  EXPECT_EQ(square(n), 49);
}

TEST(QualifiersTest, Inlineで多重定義を避ける)
{
  // twice() の実体はヘッダにあり、2 つの翻訳単位から include されています。
  EXPECT_EQ(twice(21), 42);      // このファイル側から呼ぶ
  EXPECT_EQ(use_twice(21), 42);  // src/qualifiers.cpp 側から呼ぶ
}
