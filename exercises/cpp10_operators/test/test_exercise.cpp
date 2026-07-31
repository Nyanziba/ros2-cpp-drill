// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <algorithm>
#include <sstream>
#include <vector>

#include "drill/vec2.hpp"

TEST(OperatorsTest, 足し算と引き算)
{
  const Vec2 a{1.0, 2.0};
  const Vec2 b{3.0, 4.0};

  const Vec2 sum = a + b;
  EXPECT_DOUBLE_EQ(sum.x, 4.0);
  EXPECT_DOUBLE_EQ(sum.y, 6.0);

  const Vec2 diff = b - a;
  EXPECT_DOUBLE_EQ(diff.x, 2.0);
  EXPECT_DOUBLE_EQ(diff.y, 2.0);
}

TEST(OperatorsTest, スカラー倍は左右どちらの順番でも書ける)
{
  const Vec2 a{2.0, 3.0};

  // メンバ関数として実装すると、この 2 行目が書けない。
  const Vec2 right = a * 2.0;
  const Vec2 left = 2.0 * a;

  EXPECT_DOUBLE_EQ(right.x, 4.0);
  EXPECT_DOUBLE_EQ(right.y, 6.0);
  EXPECT_DOUBLE_EQ(left.x, 4.0);
  EXPECT_DOUBLE_EQ(left.y, 6.0);
}

TEST(OperatorsTest, 加算代入は自分自身への参照を返す)
{
  Vec2 a{1.0, 2.0};
  const Vec2 b{3.0, 4.0};

  Vec2 & returned = (a += b);

  EXPECT_DOUBLE_EQ(a.x, 4.0);
  EXPECT_DOUBLE_EQ(a.y, 6.0);
  // 返ってきた参照が a そのものを指していること（コピーを返していないこと）。
  EXPECT_EQ(&returned, &a);
}

TEST(OperatorsTest, 等価比較と非等価比較)
{
  const Vec2 a{1.0, 2.0};
  const Vec2 b{1.0, 2.0};
  const Vec2 c{2.0, 3.0};

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
  EXPECT_FALSE(a != b);
  EXPECT_TRUE(a != c);

  // x だけ、y だけが違う場合も検出できること。
  // 波かっこの中のカンマはマクロの引数区切りと解釈されるので、変数に取ってから渡す。
  const Vec2 y_differs{1.0, 99.0};
  const Vec2 x_differs{99.0, 2.0};
  EXPECT_TRUE(a != y_differs);
  EXPECT_TRUE(a != x_differs);
}

TEST(OperatorsTest, 大小比較は厳密弱順序である)
{
  const Vec2 small{1.0, 1.0};   // length_squared = 2
  const Vec2 large{3.0, 4.0};   // length_squared = 25

  EXPECT_TRUE(small < large);
  EXPECT_FALSE(large < small);

  // 自分自身より小さくないこと。<= で実装するとここで落ちる。
  EXPECT_FALSE(small < small);

  // 長さが同じで向きが違う場合も、どちらも「小さくない」こと。
  const Vec2 p{3.0, 4.0};
  const Vec2 q{4.0, 3.0};
  EXPECT_FALSE(p < q);
  EXPECT_FALSE(q < p);
}

TEST(OperatorsTest, std_sortで並べられる)
{
  std::vector<Vec2> v{{3.0, 4.0}, {1.0, 0.0}, {0.0, 2.0}};
  std::sort(v.begin(), v.end());

  ASSERT_EQ(v.size(), 3u);
  EXPECT_DOUBLE_EQ(v[0].length_squared(), 1.0);
  EXPECT_DOUBLE_EQ(v[1].length_squared(), 4.0);
  EXPECT_DOUBLE_EQ(v[2].length_squared(), 25.0);
}

TEST(OperatorsTest, ostreamに流せる)
{
  const Vec2 v{1.5, 2.5};
  std::ostringstream oss;
  oss << v;
  EXPECT_EQ(oss.str(), "(1.5, 2.5)");
}

TEST(OperatorsTest, ostream演算子は繋げられる)
{
  // os を返していなければ、この行はコンパイルできないか結果が壊れる。
  const Vec2 a{1.0, 2.0};
  const Vec2 b{3.0, 4.0};
  std::ostringstream oss;
  oss << "a=" << a << " b=" << b;
  EXPECT_EQ(oss.str(), "a=(1, 2) b=(3, 4)");
}
