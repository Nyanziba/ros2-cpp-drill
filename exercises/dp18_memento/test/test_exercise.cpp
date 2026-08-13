// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "drill/gain_tuner.hpp"

namespace
{

GainTuner make_tuner()
{
  return GainTuner{1.0, 0.1, 0.01, "初期値"};
}

}  // namespace

TEST(MementoTest, スナップショットの時点に戻せる)
{
  GainTuner tuner = make_tuner();

  const GainSnapshot saved = tuner.create_snapshot();

  tuner.set_gains(9.0, 9.0, 9.0);
  tuner.set_label("暴れた設定");
  EXPECT_DOUBLE_EQ(tuner.kp(), 9.0);

  tuner.restore(saved);

  EXPECT_DOUBLE_EQ(tuner.kp(), 1.0);
  EXPECT_DOUBLE_EQ(tuner.ki(), 0.1);
  EXPECT_DOUBLE_EQ(tuner.kd(), 0.01);
  EXPECT_EQ(tuner.label(), "初期値");
}

TEST(MementoTest, 保存後に元を変えてもスナップショットは変わらない)
{
  GainTuner tuner = make_tuner();

  const GainSnapshot saved = tuner.create_snapshot();
  EXPECT_EQ(saved.label(), "初期値")
    << "create_snapshot() が現在のラベルを値でコピーしていません";

  // 元をいじる。Memento が参照や shared_ptr を持っていると、ここで一緒に変わります。
  tuner.set_label("いじったあと");
  tuner.set_gains(5.0, 5.0, 5.0);

  EXPECT_EQ(saved.label(), "初期値")
    << "Memento が元と状態を共有しています。値で持ってください";

  tuner.restore(saved);
  EXPECT_DOUBLE_EQ(tuner.kp(), 1.0);
  EXPECT_EQ(tuner.label(), "初期値");
}

TEST(MementoTest, 同じスナップショットに何度でも戻せる)
{
  GainTuner tuner = make_tuner();

  const GainSnapshot saved = tuner.create_snapshot();

  tuner.set_gains(3.0, 3.0, 3.0);
  tuner.restore(saved);
  EXPECT_DOUBLE_EQ(tuner.kp(), 1.0);

  tuner.set_gains(7.0, 7.0, 7.0);
  tuner.restore(saved);
  EXPECT_DOUBLE_EQ(tuner.kp(), 1.0);
  EXPECT_EQ(tuner.label(), "初期値");
}

TEST(MementoTest, Undoスタックで任意の時点に戻せる)
{
  GainTuner tuner = make_tuner();

  std::vector<GainSnapshot> undo_stack;

  undo_stack.push_back(tuner.create_snapshot());  // [0] 初期値

  tuner.set_gains(2.0, 0.2, 0.02);
  tuner.set_label("2回目");
  undo_stack.push_back(tuner.create_snapshot());  // [1]

  tuner.set_gains(3.0, 0.3, 0.03);
  tuner.set_label("3回目");
  undo_stack.push_back(tuner.create_snapshot());  // [2]

  tuner.set_gains(99.0, 99.0, 99.0);

  ASSERT_EQ(undo_stack.size(), 3u);
  EXPECT_EQ(undo_stack[0].label(), "初期値");
  EXPECT_EQ(undo_stack[1].label(), "2回目");
  EXPECT_EQ(undo_stack[2].label(), "3回目");

  // 真ん中の時点に直接戻す。
  tuner.restore(undo_stack[1]);
  EXPECT_DOUBLE_EQ(tuner.kp(), 2.0);
  EXPECT_DOUBLE_EQ(tuner.ki(), 0.2);
  EXPECT_EQ(tuner.label(), "2回目");

  // そのあと最初の時点に戻す。
  tuner.restore(undo_stack[0]);
  EXPECT_DOUBLE_EQ(tuner.kp(), 1.0);
  EXPECT_EQ(tuner.label(), "初期値");
}

TEST(MementoTest, ムーブ版のrestoreはMementoから状態を奪う)
{
  GainTuner tuner = make_tuner();

  GainSnapshot saved = tuner.create_snapshot();
  ASSERT_EQ(saved.label(), "初期値");

  tuner.set_gains(4.0, 4.0, 4.0);
  tuner.set_label("いじったあと");

  tuner.restore(std::move(saved));

  EXPECT_DOUBLE_EQ(tuner.kp(), 1.0);
  EXPECT_EQ(tuner.label(), "初期値");

  // ヘッダの約束: ムーブ版で戻したあと、Memento のラベルは空になる。
  EXPECT_TRUE(saved.label().empty())
    << "ムーブ版の restore が label を奪ったあと clear() していません";
}

TEST(MementoTest, Mementoの中身はOriginator以外から触れない)
{
  // wide interface（4 つの値を渡すコンストラクタ）は private なので、
  // GainTuner 以外からは呼べません。Java の package private に対応するのが friend です。
  static_assert(
    !std::is_constructible<GainSnapshot, double, double, double, std::string>::value,
    "GainSnapshot のコンストラクタが public になっています");
  static_assert(
    !std::is_default_constructible<GainSnapshot>::value,
    "GainSnapshot がデフォルト構築できてしまいます");

  // Caretaker はコピーと保持だけできればよい。
  static_assert(
    std::is_copy_constructible<GainSnapshot>::value,
    "GainSnapshot はコピーできる必要があります（Undo スタックに積むため）");

  // narrow interface は label() だけ。中身は Originator を通してしか観測できない。
  GainTuner tuner = make_tuner();
  const GainSnapshot saved = tuner.create_snapshot();
  EXPECT_EQ(saved.label(), "初期値");

  // ↓ コンパイルエラーになります（private メンバ）。試すならコメントを外してください。
  // EXPECT_DOUBLE_EQ(saved.kp_, 1.0);
}

TEST(MementoTest, POD状態を取り出して書き戻せる)
{
  static_assert(
    std::is_trivially_copyable<GainState>::value,
    "GainState は memcpy で保存するので trivially copyable でなければなりません");

  GainTuner tuner = make_tuner();

  const GainState state = tuner.capture_state();
  EXPECT_DOUBLE_EQ(state.kp, 1.0);
  EXPECT_DOUBLE_EQ(state.ki, 0.1);
  EXPECT_DOUBLE_EQ(state.kd, 0.01);

  tuner.set_gains(8.0, 8.0, 8.0);
  tuner.set_label("ラベルは戻らない");

  tuner.restore_state(state);
  EXPECT_DOUBLE_EQ(tuner.kp(), 1.0);
  EXPECT_DOUBLE_EQ(tuner.kd(), 0.01);
  EXPECT_EQ(tuner.label(), "ラベルは戻らない")
    << "restore_state はゲインだけを戻します。label は触りません";
}

TEST(MementoTest, 固定長リングバッファでも同じ復元ができる)
{
  GainTuner tuner = make_tuner();
  GainHistory history;

  EXPECT_TRUE(history.empty());

  history.push(tuner.capture_state());  // kp = 1.0

  tuner.set_gains(2.0, 0.2, 0.02);
  history.push(tuner.capture_state());  // kp = 2.0

  tuner.set_gains(99.0, 99.0, 99.0);

  ASSERT_EQ(history.size(), 2u);
  EXPECT_FALSE(history.empty());

  EXPECT_DOUBLE_EQ(history.recent(0).kp, 2.0);
  EXPECT_DOUBLE_EQ(history.recent(1).kp, 1.0);

  tuner.restore_state(history.recent(1));
  EXPECT_DOUBLE_EQ(tuner.kp(), 1.0);
  EXPECT_DOUBLE_EQ(tuner.ki(), 0.1);
  EXPECT_DOUBLE_EQ(tuner.kd(), 0.01);
}

TEST(MementoTest, リングバッファは容量を超えると古いものから消える)
{
  GainHistory history;

  // 容量 + 2 件積む。
  for (std::size_t i = 0; i < GainHistory::kCapacity + 2; ++i) {
    const double value = static_cast<double>(i);
    history.push(GainState{value, value, value});
  }

  EXPECT_EQ(history.size(), GainHistory::kCapacity)
    << "size() が容量を超えています。リングバッファになっていません";

  // 最後に積んだのは kCapacity + 1。そこから kCapacity 件だけ残っている。
  for (std::size_t back = 0; back < GainHistory::kCapacity; ++back) {
    const double expected =
      static_cast<double>(GainHistory::kCapacity + 1 - back);
    EXPECT_DOUBLE_EQ(history.recent(back).kp, expected)
      << "back_index = " << back << " の中身が違います";
  }
}
