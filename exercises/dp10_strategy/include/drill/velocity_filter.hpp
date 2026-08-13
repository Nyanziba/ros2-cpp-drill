// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <functional>
#include <utility>

/// 題材: 速度指令のフィルタ。
///
/// 上位から降ってくる生の速度指令 raw を、そのままモータに渡さずに
/// 「前回出した値 previous」と突き合わせて丸めてから出します。
/// 丸めかた（＝アルゴリズム）が Strategy です。
///
/// この課題では、同じ Strategy を C++ の 3 通りの手段で実装して比べます。
///   1) 仮想関数版        … Java 版と同じ形。実行時に差し替えられる
///   2) std::function 版  … ラムダをそのまま渡せる。確保が走りうる
///   3) テンプレート版    … コンパイル時に決まる。仮想関数もヒープもゼロ
///
/// 【重要】3 つとも「同じ入力に同じ出力」でなければいけません。
/// 手段が違うだけで、アルゴリズムは同じものです。

// ---------------------------------------------------------------------------
// 1) 仮想関数版
// ---------------------------------------------------------------------------

/// 速度フィルタ（Strategy）。結城本の Strategy インタフェースに対応します。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須。
///   - apply() は状態を持たないので const メンバ関数にします。
///     状態（前回値）は Context 側が持ちます。これがマイコンで効きます
///     （Strategy が状態を持たなければ static なインスタンスを共有できる）。
class VelocityFilter
{
public:
  virtual ~VelocityFilter() = default;

  /// previous: 前回このフィルタが出した値 / raw: 今回の生指令。
  virtual double apply(double previous, double raw) const = 0;
};

/// 絶対値で頭打ちにする。previous は見ません。
class ClampFilter : public VelocityFilter
{
public:
  explicit ClampFilter(double max_abs)
  : max_abs_(max_abs)
  {
  }

  double apply(double previous, double raw) const override;

private:
  double max_abs_;
};

/// 前回値からの変化量を制限する（スルーレート制限）。
class SlewRateFilter : public VelocityFilter
{
public:
  explicit SlewRateFilter(double max_delta)
  : max_delta_(max_delta)
  {
  }

  double apply(double previous, double raw) const override;

private:
  double max_delta_;
};

/// Context（仮想関数版）。
///
/// 【所有権】このクラスは Strategy を**所有しません**。ポインタで指すだけです。
/// つまり VelocityFilter の実体は VirtualCommander より長生きしなければいけません。
/// unique_ptr で所有する設計もありえますが、そうするとヒープが要ります。
/// マイコンでは「static な実体を置いてポインタを差し替える」この形を使います。
class VirtualCommander
{
public:
  explicit VirtualCommander(const VelocityFilter & filter);

  /// 実行時に Strategy を差し替える。
  void set_filter(const VelocityFilter & filter);

  /// いま指している Strategy。所有していないのでポインタを返します。
  const VelocityFilter * filter() const;

  /// 生指令を 1 つ処理して、出力値を返す。内部の前回値も更新する。
  double update(double raw);

  /// 直近の出力値。
  double output() const;

  /// 前回値を 0 に戻す。
  void reset();

private:
  const VelocityFilter * filter_;   // 所有しない
  double previous_ = 0.0;
};

// ---------------------------------------------------------------------------
// 2) std::function 版
// ---------------------------------------------------------------------------

/// Context（std::function 版）。
///
/// 継承も override も要りません。ラムダをそのまま渡せます。
/// 代わりに、std::function は中身を型消去して持つので**ヒープ確保が走りうる**。
/// 小さいオブジェクトなら確保されないことが多い（SBO）が、**規格上の保証はありません**。
class FunctionCommander
{
public:
  using FilterFn = std::function<double(double previous, double raw)>;

  explicit FunctionCommander(FilterFn filter);

  void set_filter(FilterFn filter);

  /// Strategy が入っているか。初期状態では空でも呼べてしまうので確認用。
  bool has_filter() const;

  double update(double raw);
  double output() const;
  void reset();

private:
  FilterFn filter_;
  double previous_ = 0.0;
};

/// ClampFilter と同じ計算をするラムダを返す。
/// 「Strategy をクラスにしなくても関数オブジェクトで足りる」ことの確認です。
FunctionCommander::FilterFn make_clamp_fn(double max_abs);

// ---------------------------------------------------------------------------
// 3) テンプレート（ポリシー）版
// ---------------------------------------------------------------------------

/// ポリシー。基底クラスも virtual もありません。
/// 求められるのは「apply(previous, raw) が呼べること」だけです（暗黙の要件）。
class ClampPolicy
{
public:
  explicit ClampPolicy(double max_abs)
  : max_abs_(max_abs)
  {
  }

  double apply(double previous, double raw) const;

private:
  double max_abs_;
};

class SlewRatePolicy
{
public:
  explicit SlewRatePolicy(double max_delta)
  : max_delta_(max_delta)
  {
  }

  double apply(double previous, double raw) const;

private:
  double max_delta_;
};

/// Context（テンプレート版）。
///
/// Strategy は型パラメータです。差し替えは**コンパイル時**にしかできません。
/// 代わりに、仮想関数呼び出しもヒープ確保もゼロで、apply() はインライン展開されます。
/// このクラスはテンプレートなので定義がヘッダにあります（実装する箇所ではありません）。
template <typename FilterPolicy>
class StaticCommander
{
public:
  explicit StaticCommander(FilterPolicy policy)
  : policy_(std::move(policy))
  {
  }

  double update(double raw)
  {
    previous_ = policy_.apply(previous_, raw);
    return previous_;
  }

  double output() const { return previous_; }

  void reset() { previous_ = 0.0; }

private:
  FilterPolicy policy_;
  double previous_ = 0.0;
};
