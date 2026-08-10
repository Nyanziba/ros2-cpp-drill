// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <type_traits>
#include <vector>

#include "drill/velocity_filter.hpp"

namespace
{

const std::vector<double> kRawCommands = {2.0, 2.0, 2.0, -3.0, 0.4};

/// 同じ生指令の列を Context に流し、出力の列を集める。
template <typename Commander>
std::vector<double> run(Commander & commander)
{
  std::vector<double> outputs;
  for (const double raw : kRawCommands) {
    outputs.push_back(commander.update(raw));
  }
  return outputs;
}

void expect_near_all(const std::vector<double> & actual, const std::vector<double> & expected)
{
  ASSERT_EQ(actual.size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_NEAR(actual[i], expected[i], 1e-9) << "index=" << i;
  }
}

}  // namespace

// --- アルゴリズムそのもの ---------------------------------------------------

TEST(StrategyTest, clampは絶対値で頭打ちにする)
{
  const ClampFilter filter{1.0};
  EXPECT_NEAR(filter.apply(0.0, 2.0), 1.0, 1e-9);
  EXPECT_NEAR(filter.apply(0.0, -3.0), -1.0, 1e-9);
  EXPECT_NEAR(filter.apply(0.0, 0.4), 0.4, 1e-9);

  // previous を見ないアルゴリズムなので、previous を変えても結果は変わらない。
  EXPECT_NEAR(filter.apply(99.0, 0.4), 0.4, 1e-9);
}

TEST(StrategyTest, slewrateは前回値からの変化量を制限する)
{
  const SlewRateFilter filter{0.5};
  EXPECT_NEAR(filter.apply(0.0, 2.0), 0.5, 1e-9);
  EXPECT_NEAR(filter.apply(0.5, 2.0), 1.0, 1e-9);
  EXPECT_NEAR(filter.apply(1.0, -3.0), 0.5, 1e-9);
  EXPECT_NEAR(filter.apply(1.0, 1.2), 1.2, 1e-9);
}

// --- 3 つの手段が同じ結果を出すこと ------------------------------------------

TEST(StrategyTest, 仮想関数版とfunction版とテンプレート版がclampで一致する)
{
  const std::vector<double> expected = {1.0, 1.0, 1.0, -1.0, 0.4};

  const ClampFilter filter{1.0};
  VirtualCommander virtual_commander{filter};
  expect_near_all(run(virtual_commander), expected);

  FunctionCommander function_commander{make_clamp_fn(1.0)};
  ASSERT_TRUE(function_commander.has_filter()) << "make_clamp_fn が空の std::function を返しています";
  expect_near_all(run(function_commander), expected);

  StaticCommander<ClampPolicy> static_commander{ClampPolicy{1.0}};
  expect_near_all(run(static_commander), expected);
}

TEST(StrategyTest, 仮想関数版とテンプレート版がslewrateで一致する)
{
  // 0.5 ずつしか動けない: 0.5 → 1.0 → 1.5 → 1.0 → 0.5
  const std::vector<double> expected = {0.5, 1.0, 1.5, 1.0, 0.5};

  const SlewRateFilter filter{0.5};
  VirtualCommander virtual_commander{filter};
  expect_near_all(run(virtual_commander), expected);

  StaticCommander<SlewRatePolicy> static_commander{SlewRatePolicy{0.5}};
  expect_near_all(run(static_commander), expected);
}

// --- 実行時の差し替え -------------------------------------------------------

TEST(StrategyTest, 仮想関数版は実行時にStrategyを差し替えられる)
{
  const ClampFilter clamp{1.0};
  const SlewRateFilter slew{0.5};

  VirtualCommander commander{clamp};
  EXPECT_NEAR(commander.update(2.0), 1.0, 1e-9) << "clamp なので 1.0 に頭打ち";

  commander.set_filter(slew);
  // previous_ は 1.0 のまま。slew に替わったので 0.5 しか動けない。
  EXPECT_NEAR(commander.update(5.0), 1.5, 1e-9) << "Strategy が差し替わっていません";
}

TEST(StrategyTest, 仮想関数版はStrategyを所有せず参照で指している)
{
  const ClampFilter clamp{1.0};
  const SlewRateFilter slew{0.5};

  VirtualCommander commander{clamp};
  EXPECT_EQ(commander.filter(), &clamp)
    << "Strategy をコピーして持っています。ポインタで指してください（コピーするとスライシング）";

  commander.set_filter(slew);
  EXPECT_EQ(commander.filter(), &slew);
}

TEST(StrategyTest, function版はラムダを直接渡せる)
{
  int call_count = 0;

  // キャプチャありのラムダ。継承も override も要らない。
  FunctionCommander commander{
    [&call_count](double previous, double raw) {
      ++call_count;
      return (previous + raw) * 0.5;   // 単純な平均フィルタ
    }};

  EXPECT_NEAR(commander.update(2.0), 1.0, 1e-9);
  EXPECT_NEAR(commander.update(2.0), 1.5, 1e-9);
  EXPECT_EQ(call_count, 2);

  // 実行時に別のラムダへ差し替えられる。
  commander.set_filter([](double, double raw) { return raw; });
  EXPECT_NEAR(commander.update(9.0), 9.0, 1e-9);
  EXPECT_EQ(call_count, 2) << "差し替え後も古いラムダが呼ばれています";
}

TEST(StrategyTest, resetで前回値が戻る)
{
  const SlewRateFilter slew{0.5};
  VirtualCommander commander{slew};

  EXPECT_NEAR(commander.update(2.0), 0.5, 1e-9);
  EXPECT_NEAR(commander.output(), 0.5, 1e-9);

  commander.reset();
  EXPECT_NEAR(commander.output(), 0.0, 1e-9);
  EXPECT_NEAR(commander.update(2.0), 0.5, 1e-9) << "reset 後は前回値 0 から始まります";
}

// --- 型の性質（ここが C++ の Strategy の核心）--------------------------------

TEST(StrategyTest, テンプレート版は仮想関数を一切持たない)
{
  // 仮想関数版は多態。vtable がある。
  static_assert(std::is_polymorphic_v<ClampFilter>, "ClampFilter は多態のはず");
  static_assert(std::is_polymorphic_v<SlewRateFilter>, "SlewRateFilter は多態のはず");

  // ポリシー版は多態でない。vtable が無い。
  static_assert(!std::is_polymorphic_v<ClampPolicy>, "ClampPolicy に virtual を付けないでください");
  static_assert(!std::is_polymorphic_v<SlewRatePolicy>, "SlewRatePolicy に virtual を付けないでください");
  static_assert(
    !std::is_polymorphic_v<StaticCommander<ClampPolicy>>,
    "StaticCommander に virtual を付けないでください");

  // vtable ポインタが無いぶん、ポリシーの方が小さい。
  EXPECT_LT(sizeof(ClampPolicy), sizeof(ClampFilter))
    << "ClampPolicy=" << sizeof(ClampPolicy) << " ClampFilter=" << sizeof(ClampFilter);

  // 値は同じでなければならない。構造が違うだけで、アルゴリズムは同じもの。
  const ClampFilter filter{1.0};
  const ClampPolicy policy{1.0};
  EXPECT_NEAR(policy.apply(0.0, 2.0), 1.0, 1e-9);
  EXPECT_NEAR(policy.apply(0.0, -3.0), -1.0, 1e-9);
  EXPECT_NEAR(filter.apply(0.0, 2.0), policy.apply(0.0, 2.0), 1e-9);
  EXPECT_NEAR(filter.apply(0.0, -3.0), policy.apply(0.0, -3.0), 1e-9);
}
