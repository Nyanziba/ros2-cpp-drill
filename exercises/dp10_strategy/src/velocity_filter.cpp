// I AM NOT DONE
//
// 結城本 第10章 Strategy を C++ で書きます。
// 同じアルゴリズムを 3 通りの手段で実装し、3 つとも同じ結果になることを確かめます。
//
//   1) 仮想関数版   ClampFilter / SlewRateFilter / VirtualCommander
//   2) std::function 版   make_clamp_fn / FunctionCommander
//   3) テンプレート版     ClampPolicy / SlewRatePolicy
//
// アルゴリズムの仕様（3 つとも同じにしてください）:
//   clamp     : 出力 = raw を [-max_abs, +max_abs] に収めた値。previous は使わない
//   slew rate : 出力 = raw を [previous - max_delta, previous + max_delta] に収めた値

#include "drill/velocity_filter.hpp"

#include <algorithm>

// ---------------------------------------------------------------------------
// 1) 仮想関数版
// ---------------------------------------------------------------------------

double ClampFilter::apply(double, double) const
{
  // TODO: raw を [-max_abs_, +max_abs_] に収めて返してください。
  //       previous は使いません。使わない引数は名前を書かないのが C++ の作法です
  //       （名前を書くと -Wunused-parameter で警告が出ます）。
  //       std::clamp が <algorithm> にあります。
  return std::clamp(0.0, -max_abs_, max_abs_);   // 仮の値（常に 0.0）
}

double SlewRateFilter::apply(double, double) const
{
  // TODO: raw を [previous - max_delta_, previous + max_delta_] に収めて返してください。
  return std::clamp(0.0, -max_delta_, max_delta_);   // 仮の値（常に 0.0）
}

VirtualCommander::VirtualCommander(const VelocityFilter &)
: filter_(nullptr)
{
  // TODO: 受け取った filter のアドレスを filter_ に入れてください。
  //
  //       コピーして持たないこと。コピーすると派生クラス部分が切り落とされます
  //       （スライシング）。所有もしません。呼び出し側が実体を生かし続けます。
}

void VirtualCommander::set_filter(const VelocityFilter &)
{
  // TODO: filter_ を差し替えてください。これが「実行時に切り替えられる」ということです。
}

const VelocityFilter * VirtualCommander::filter() const
{
  // TODO: いま指している Strategy を返してください。
  return nullptr;
}

double VirtualCommander::update(double)
{
  // TODO: filter_->apply(previous_, raw) を呼び、その結果を previous_ に入れて返してください。
  (void)filter_;   // 未実装のあいだ「使っていない」警告を出さないための行。消して構いません
  return 0.0;
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

FunctionCommander::FunctionCommander(FilterFn)
{
  // TODO: 受け取った filter を filter_ に入れてください（std::move を使います）。
}

void FunctionCommander::set_filter(FilterFn)
{
  // TODO: filter_ を差し替えてください。
}

bool FunctionCommander::has_filter() const
{
  return static_cast<bool>(filter_);
}

double FunctionCommander::update(double)
{
  // TODO: filter_(previous_, raw) を呼び、その結果を previous_ に入れて返してください。
  return 0.0;
}

double FunctionCommander::output() const
{
  return previous_;
}

void FunctionCommander::reset()
{
  previous_ = 0.0;
}

FunctionCommander::FilterFn make_clamp_fn(double)
{
  // TODO: ClampFilter::apply と同じ計算をするラムダを返してください。
  //
  //       max_abs をキャプチャします。キャプチャがあるので、このラムダは
  //       関数ポインタには変換できません。std::function が要る理由がこれです。
  return FunctionCommander::FilterFn{};
}

// ---------------------------------------------------------------------------
// 3) テンプレート（ポリシー）版
// ---------------------------------------------------------------------------

double ClampPolicy::apply(double, double) const
{
  // TODO: ClampFilter::apply と同じ計算を書いてください。
  //       違うのは「virtual が無い」ことだけです。
  return std::clamp(0.0, -max_abs_, max_abs_);   // 仮の値（常に 0.0）
}

double SlewRatePolicy::apply(double, double) const
{
  // TODO: SlewRateFilter::apply と同じ計算を書いてください。
  return std::clamp(0.0, -max_delta_, max_delta_);   // 仮の値（常に 0.0）
}
