// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include "drill/data.hpp"

TEST(ValueSemanticsTest, CopyCtorが数えられる)
{
  Data::reset();
  Data d1(10);
  Data d2(d1);  // まだ構築されていない d2 を作るので copy ctor
  EXPECT_EQ(Data::copy_ctor_count, 1);
  EXPECT_EQ(Data::copy_assign_count, 0);
  EXPECT_EQ(d2.value(), 10);
}

TEST(ValueSemanticsTest, CopyAssignが数えられる)
{
  Data::reset();
  Data d1(10);
  Data d2(20);
  d2 = d1;  // 構築済みのオブジェクトへの代入なので copy assign
  EXPECT_EQ(Data::copy_assign_count, 1);
  EXPECT_EQ(Data::copy_ctor_count, 0);
  EXPECT_EQ(d2.value(), 10);
}

TEST(ValueSemanticsTest, 値渡しはコピーが2回起きる)
{
  Data::reset();
  Data orig(5);
  auto result = process_by_value(orig);

  // 1 回目: orig → 引数 d
  // 2 回目: return d → 戻り値。引数には NRVO が効かず、Data はコピーコンストラクタを
  //         自分で書いているためムーブコンストラクタが暗黙生成されず、コピーになる
  // 戻り値 → result は C++17 のコピー省略で消えるので、ここは数に入らない
  EXPECT_EQ(Data::copy_ctor_count, 2);
  EXPECT_EQ(result.value(), 5);
}

TEST(ValueSemanticsTest, ConstRefはコピーが起きない)
{
  Data::reset();
  Data orig(5);
  auto result = process_by_const_ref(orig);

  // 引数は参照なのでコピーされない。
  // 戻り値の result も NRVO で呼び出し元に直接構築されるので、コピーは 1 回も起きない。
  EXPECT_EQ(Data::copy_ctor_count, 0);
  EXPECT_EQ(Data::copy_assign_count, 0);
  EXPECT_EQ(result.value(), 10);
}
