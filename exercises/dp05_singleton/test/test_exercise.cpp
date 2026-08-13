// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <vector>

#include "drill/uart_port.hpp"

// --- コンパイル時の検査 -------------------------------------------------
//
// Java では「コピーできてしまう」事故が起きません（オブジェクトは常に参照だから）。
// C++ では = delete を書かないと UartPort p = UartPort::instance(); が通り、
// シングルトンでなくなります。ヘッダの 4 行が消えたらここで落ちます。

static_assert(
  !std::is_copy_constructible<UartPort>::value,
  "UartPort はコピー構築できてはいけません（コピーコンストラクタを = delete に）");
static_assert(
  !std::is_copy_assignable<UartPort>::value,
  "UartPort はコピー代入できてはいけません（コピー代入演算子を = delete に）");
static_assert(
  !std::is_move_constructible<UartPort>::value,
  "UartPort はムーブ構築できてはいけません");
static_assert(
  !std::is_move_assignable<UartPort>::value,
  "UartPort はムーブ代入できてはいけません");

// 外から勝手に作れないこと。
static_assert(
  !std::is_default_constructible<UartPort>::value,
  "UartPort は外から構築できてはいけません（コンストラクタを private に）");

// instance() はポインタではなく参照を返す。
// 参照なら「呼び出し側は解放しない」ことが型に書かれています。
static_assert(
  std::is_same<decltype(UartPort::instance()), UartPort &>::value,
  "instance() は UartPort & を返してください");

namespace
{

/// 各テストの先頭で状態を切り離すための土台。
///
/// シングルトンは状態がテストからテストへ漏れます。
/// これを書かずに済ませられるのが「引数で渡す設計」の利点です。
class UartPortTest : public ::testing::Test
{
protected:
  void SetUp() override { UartPort::instance().reset(); }
};

}  // namespace

TEST_F(UartPortTest, instanceは何度呼んでも同じオブジェクトを返す)
{
  UartPort & first = UartPort::instance();
  UartPort & second = UartPort::instance();

  EXPECT_EQ(&first, &second)
    << "instance() が呼ぶたびに別のオブジェクトを返しています。"
       "関数ローカル static（Meyers Singleton）にしてください";
}

TEST_F(UartPortTest, 状態が唯一のインスタンスで共有される)
{
  UartPort::instance().set_baud_rate(9600);

  // 別の経路から取っても同じ状態が見えるはず。
  EXPECT_EQ(UartPort::instance().baud_rate(), 9600u);
}

TEST_F(UartPortTest, 初期化は何度instanceを呼んでも一度しか走らない)
{
  for (int i = 0; i < 100; ++i) {
    UartPort::instance().write_line("ping");
  }

  EXPECT_EQ(UartPort::construction_count(), 1)
    << "コンストラクタが " << UartPort::construction_count()
    << " 回走っています。1 回だけになるようにしてください";
}

TEST_F(UartPortTest, resetでボーレートが既定値に戻る)
{
  UartPort::instance().set_baud_rate(9600);
  UartPort::instance().reset();

  EXPECT_EQ(UartPort::instance().baud_rate(), kDefaultBaudRate);
}

TEST_F(UartPortTest, resetで送信履歴が空になる)
{
  UartPort::instance().write_line("hello");
  UartPort::instance().write_line("world");
  ASSERT_EQ(UartPort::instance().sent_lines().size(), 2u);

  UartPort::instance().reset();

  EXPECT_TRUE(UartPort::instance().sent_lines().empty());
}

TEST_F(UartPortTest, resetはオブジェクトを作り直さない)
{
  const UartPort * before = &UartPort::instance();
  const int count_before = UartPort::construction_count();

  UartPort::instance().reset();

  EXPECT_EQ(&UartPort::instance(), before)
    << "reset() でインスタンスが作り直されています。状態だけ戻してください";
  EXPECT_EQ(UartPort::construction_count(), count_before);
}

TEST_F(UartPortTest, 前のテストの状態が残っていない)
{
  // SetUp が reset() を呼んでいるので、ここでは必ず既定値のはず。
  // reset() が空実装だと、前のテストが書いた値が漏れてきます。
  EXPECT_EQ(UartPort::instance().baud_rate(), kDefaultBaudRate);
  EXPECT_TRUE(UartPort::instance().sent_lines().empty());
}

// LazyProbe は「遅延初期化」を見るためだけの型なので、
// このテスト以外からは絶対に触りません。
TEST(LazySingletonTest, 初期化は最初のinstance呼び出しまで走らない)
{
  EXPECT_FALSE(LazyProbe::was_constructed())
    << "instance() を呼ぶ前に構築されています。"
       "グローバルオブジェクトではなく関数ローカル static にしてください";

  LazyProbe & first = LazyProbe::instance();

  EXPECT_TRUE(LazyProbe::was_constructed());
  EXPECT_EQ(&first, &LazyProbe::instance());
}
