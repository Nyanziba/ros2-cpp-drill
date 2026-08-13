// I AM NOT DONE
//
// Pimpl を書きます。Bridge と構造は同じ、目的が違います。
//
// この .cpp だけが Impl の中身を知っています。
// ヘッダを include している他の翻訳単位は、ここに何を書いても再コンパイルされません。

#include "drill/link_stats.hpp"

#include <algorithm>
#include <vector>

/// 実装の中身。**ヘッダには 1 行も出ません。**
struct LinkStats::Impl
{
  // TODO: 遅延サンプルを保持するメンバを持たせてください。
  //       例: std::vector<double> samples_;
  //       count() / mean() / max() が答えられれば形は問いません。
};

LinkStats::LinkStats()
{
  // TODO: impl_ を std::make_unique<Impl>() で作ってください。
  //       ここを書かないと impl_ は nullptr のままです。
}

// ここから 3 つは **必ず .cpp に置きます。**
// ヘッダに書くと Impl が不完全型なので
//   error: invalid application of 'sizeof' to an incomplete type 'LinkStats::Impl'
// で落ちます。記事の 9.4 でわざと出しています。
LinkStats::~LinkStats() = default;
LinkStats::LinkStats(LinkStats && other) noexcept = default;
LinkStats & LinkStats::operator=(LinkStats && other) noexcept = default;

void LinkStats::add_sample(double latency_ms)
{
  // TODO: impl_ の中にサンプルを 1 つ追加してください。
  static_cast<void>(latency_ms);
}

std::size_t LinkStats::count() const
{
  // TODO: サンプル数を返してください。
  return 0;
}

double LinkStats::mean() const
{
  // TODO: 平均を返してください。サンプルが 0 個なら 0.0。
  return 0.0;
}

double LinkStats::max() const
{
  // TODO: 最大を返してください。サンプルが 0 個なら 0.0。
  //       std::max_element が使えます。
  return 0.0;
}
