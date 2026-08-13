// 解答例
//
// 結城本 第10章 Strategy。同じアルゴリズムを 3 通りの手段で実装したもの。
//
//   1) 仮想関数版        実行時に差し替えられる。vtable のぶんだけ太る
//   2) std::function 版  ラムダをそのまま渡せる。確保が走りうる
//   3) テンプレート版    コンパイル時に固定。仮想関数もヒープもゼロ

#include "drill/velocity_filter.hpp"

#include <algorithm>
#include <utility>

// ---------------------------------------------------------------------------
// 1) 仮想関数版
// ---------------------------------------------------------------------------

// previous は使いません。使わない引数は名前を書きません。
double ClampFilter::apply(double, double raw) const
{
  return std::clamp(raw, -max_abs_, max_abs_);
}

double SlewRateFilter::apply(double previous, double raw) const
{
  return std::clamp(raw, previous - max_delta_, previous + max_delta_);
}

VirtualCommander::VirtualCommander(const VelocityFilter & filter)
: filter_(&filter)
{
  // コピーではなくアドレスを持ちます。
  // コピーすると VelocityFilter 部分だけが残り、派生の apply() が消えます（スライシング）。
  // 所有もしません。filter の実体は、この Commander より長生きしなければいけません。
}

void VirtualCommander::set_filter(const VelocityFilter & filter)
{
  filter_ = &filter;
}

const VelocityFilter * VirtualCommander::filter() const
{
  return filter_;
}

double VirtualCommander::update(double raw)
{
  previous_ = filter_->apply(previous_, raw);
  return previous_;
}

double VirtualCommander::output() const
{
  return previous_;
}

void VirtualCommander::reset()
{
  previous_ = 0.0;
}

// ---------------------------------------------------------------------------
// 2) std::function 版
// ---------------------------------------------------------------------------

FunctionCommander::FunctionCommander(FilterFn filter)
: filter_(std::move(filter))
{
}

void FunctionCommander::set_filter(FilterFn filter)
{
  filter_ = std::move(filter);
}

bool FunctionCommander::has_filter() const
{
  return static_cast<bool>(filter_);
}

double FunctionCommander::update(double raw)
{
  previous_ = filter_(previous_, raw);
  return previous_;
}

double FunctionCommander::output() const
{
  return previous_;
}

void FunctionCommander::reset()
{
  previous_ = 0.0;
}

FunctionCommander::FilterFn make_clamp_fn(double max_abs)
{
  // max_abs をキャプチャしているので、このラムダは関数ポインタに変換できません。
  // キャプチャを外せば double(*)(double, double) に変換できます。
  return [max_abs](double, double raw) { return std::clamp(raw, -max_abs, max_abs); };
}

// ---------------------------------------------------------------------------
// 3) テンプレート（ポリシー）版
// ---------------------------------------------------------------------------

// 中身は仮想関数版と 1 文字も違いません。違うのは virtual が無いことだけです。
double ClampPolicy::apply(double, double raw) const
{
  return std::clamp(raw, -max_abs_, max_abs_);
}

double SlewRatePolicy::apply(double previous, double raw) const
{
  return std::clamp(raw, previous - max_delta_, previous + max_delta_);
}
