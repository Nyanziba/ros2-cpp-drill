// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "drill/link_stats.hpp"
#include "drill/telemetry_view.hpp"

namespace
{

/// **機能側の手順。** この関数の中には実装（Sink）の型名が 1 つも出てきません。
/// 実装を差し替えたければ、呼ぶ側が渡す unique_ptr を変えるだけです。
std::vector<std::string> run_show(std::unique_ptr<TelemetrySink> sink, std::vector<std::string> & log)
{
  TelemetryView view(std::move(sink));
  view.show("v=12.4");
  return log;
}

/// 機能を 1 つ増やした版。やはり実装の型名は出てきません。
std::vector<std::string> run_show_repeat(
  std::unique_ptr<TelemetrySink> sink, std::vector<std::string> & log, int times)
{
  RepeatView view(std::move(sink), times);
  view.show_repeat("v=12.4");
  return log;
}

}  // namespace

TEST(BridgeTest, 機能側はopenとput_lineとcloseをこの順で呼ぶ)
{
  std::vector<std::string> log;
  const std::vector<std::string> result = run_show(std::make_unique<RecordingSink>(log), log);

  const std::vector<std::string> expected = {"<open>", "v=12.4", "<close>"};
  EXPECT_EQ(result, expected);
}

TEST(BridgeTest, 機能を増やしても実装側は変えずに済む)
{
  std::vector<std::string> log;
  const std::vector<std::string> result =
    run_show_repeat(std::make_unique<RecordingSink>(log), log, 3);

  // open は 1 回、close も 1 回。put_line だけ 3 回。
  const std::vector<std::string> expected = {
    "<open>", "v=12.4", "v=12.4", "v=12.4", "<close>"};
  EXPECT_EQ(result, expected);
}

TEST(BridgeTest, 繰り返し回数が0でもopenとcloseは呼ばれる)
{
  std::vector<std::string> log;
  const std::vector<std::string> result =
    run_show_repeat(std::make_unique<RecordingSink>(log), log, 0);

  const std::vector<std::string> expected = {"<open>", "<close>"};
  EXPECT_EQ(result, expected);
}

TEST(BridgeTest, 実装を差し替えても機能側のコードは1行も変わらない)
{
  // 呼んでいる関数は上のテストとまったく同じ run_show。渡す実装だけが違う。
  std::vector<std::string> log;
  const std::vector<std::string> result = run_show(std::make_unique<NumberedSink>(log), log);

  const std::vector<std::string> expected = {"<open>", "0: v=12.4", "<close>"};
  EXPECT_EQ(result, expected);
}

TEST(BridgeTest, 機能2つと実装2つの組み合わせが4通りとも動く)
{
  {
    std::vector<std::string> log;
    const std::vector<std::string> result = run_show(std::make_unique<RecordingSink>(log), log);
    EXPECT_EQ(result, (std::vector<std::string>{"<open>", "v=12.4", "<close>"}));
  }
  {
    std::vector<std::string> log;
    const std::vector<std::string> result = run_show(std::make_unique<NumberedSink>(log), log);
    EXPECT_EQ(result, (std::vector<std::string>{"<open>", "0: v=12.4", "<close>"}));
  }
  {
    std::vector<std::string> log;
    const std::vector<std::string> result =
      run_show_repeat(std::make_unique<RecordingSink>(log), log, 2);
    EXPECT_EQ(result, (std::vector<std::string>{"<open>", "v=12.4", "v=12.4", "<close>"}));
  }
  {
    std::vector<std::string> log;
    const std::vector<std::string> result =
      run_show_repeat(std::make_unique<NumberedSink>(log), log, 2);
    EXPECT_EQ(
      result, (std::vector<std::string>{"<open>", "0: v=12.4", "1: v=12.4", "<close>"}));
  }
}

TEST(BridgeTest, 機能側の基底ポインタから解放しても派生が正しく壊れる)
{
  // 仮想デストラクタの確認。無いと RepeatView のメンバが解放されません。
  EXPECT_TRUE(std::has_virtual_destructor<TelemetryView>::value);
  EXPECT_TRUE(std::has_virtual_destructor<TelemetrySink>::value);

  std::vector<std::string> log;
  {
    std::unique_ptr<TelemetryView> view =
      std::make_unique<RepeatView>(std::make_unique<RecordingSink>(log), 1);
    view->show("hb");
  }
  EXPECT_EQ(log, (std::vector<std::string>{"<open>", "hb", "<close>"}));
}

TEST(PimplTest, ヘッダに実装の詳細が出ていない)
{
  // LinkStats が持っているのはポインタ 1 個だけ。
  // Impl に何個メンバを足しても、このサイズは変わりません。
  static_assert(
    sizeof(LinkStats) == sizeof(std::unique_ptr<void *>),
    "LinkStats はポインタ 1 個分のはずです。実装をヘッダに書いていませんか");

  LinkStats stats;
  EXPECT_EQ(stats.count(), 0u);
  stats.add_sample(1.0);
  EXPECT_EQ(stats.count(), 1u) << "コンストラクタで impl_ を作っていますか";
}

TEST(PimplTest, ムーブ可能でコピー不可)
{
  static_assert(std::is_move_constructible<LinkStats>::value, "ムーブ構築できるはずです");
  static_assert(std::is_move_assignable<LinkStats>::value, "ムーブ代入できるはずです");
  static_assert(!std::is_copy_constructible<LinkStats>::value, "コピーは禁止のはずです");
  static_assert(!std::is_copy_assignable<LinkStats>::value, "コピー代入は禁止のはずです");

  LinkStats stats;
  stats.add_sample(2.0);
  stats.add_sample(4.0);

  LinkStats moved = std::move(stats);
  EXPECT_EQ(moved.count(), 2u);
  EXPECT_DOUBLE_EQ(moved.mean(), 3.0);

  LinkStats assigned;
  assigned = std::move(moved);
  EXPECT_EQ(assigned.count(), 2u);
  EXPECT_DOUBLE_EQ(assigned.max(), 4.0);
}

TEST(PimplTest, 統計の計算が合う)
{
  LinkStats stats;
  EXPECT_EQ(stats.count(), 0u);
  EXPECT_DOUBLE_EQ(stats.mean(), 0.0);
  EXPECT_DOUBLE_EQ(stats.max(), 0.0);

  stats.add_sample(1.5);
  stats.add_sample(3.5);
  stats.add_sample(2.0);

  EXPECT_EQ(stats.count(), 3u);
  EXPECT_DOUBLE_EQ(stats.mean(), 7.0 / 3.0);
  EXPECT_DOUBLE_EQ(stats.max(), 3.5);
}
