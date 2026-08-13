// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <memory>

/// 通信リンクの遅延統計。**Pimpl（pointer to implementation）で書いてあります。**
///
/// Bridge と構造は同じです。「機能」がこの LinkStats、「実装」が Impl。
/// 違うのは目的です。
///   - Bridge : 実装を **差し替える**ため。だから実装側は多態（純粋仮想）
///   - Pimpl  : 実装を **隠す**ため。実装は 1 つだけで、差し替えません
///
/// 隠すと何が得か:
///   - このヘッダは <cstddef> と <memory> しか include していません。
///     実装が <vector> や <algorithm> を使っても、このヘッダを include する
///     翻訳単位はそれらを読みません。**再コンパイルが減ります。**
///   - Impl にメンバを足しても、このヘッダは 1 バイトも変わりません。
///     クラスのサイズも変わりません（常にポインタ 1 個）。
///
/// 【C++ 固有の落とし穴】
/// unique_ptr<Impl> を持つと、デストラクタを **ヘッダで定義できません**。
/// ヘッダの時点で Impl は不完全型で、~unique_ptr が sizeof(Impl) を要求するからです。
/// だから宣言だけをここに書き、定義は Impl が完全型になった .cpp に置きます。
/// ムーブコンストラクタ／ムーブ代入も同じ理由で .cpp 側です。
class LinkStats
{
public:
  LinkStats();
  ~LinkStats();

  /// ムーブ可。宣言だけ。定義は .cpp（Impl が完全型になってから）。
  LinkStats(LinkStats && other) noexcept;
  LinkStats & operator=(LinkStats && other) noexcept;

  /// コピーは禁止。unique_ptr を持っているので既定でも消えますが、明示します。
  LinkStats(const LinkStats &) = delete;
  LinkStats & operator=(const LinkStats &) = delete;

  /// 遅延サンプル [ms] を 1 つ足す。
  void add_sample(double latency_ms);

  /// これまでに足したサンプル数。
  std::size_t count() const;

  /// 平均 [ms]。サンプルが 0 個なら 0.0。
  double mean() const;

  /// 最大 [ms]。サンプルが 0 個なら 0.0。
  double max() const;

private:
  /// 前方宣言だけ。中身はこのヘッダを読む人には見えません。
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
