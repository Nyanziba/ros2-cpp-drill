// 解答例
//
// Pimpl。Bridge と構造は同じで、目的が違います（差し替えではなく隠蔽）。
//
// この .cpp だけが Impl の中身を知っています。
// <algorithm> も <vector> も、ヘッダ側には漏れていません。

#include "drill/link_stats.hpp"

#include <algorithm>
#include <vector>

struct LinkStats::Impl
{
  std::vector<double> samples;
};

LinkStats::LinkStats()
: impl_(std::make_unique<Impl>())
{
}

// この 3 つは必ず .cpp に置きます。
// ヘッダ側では Impl が不完全型なので、unique_ptr のデリータが
// sizeof(Impl) を要求してコンパイルエラーになります。
LinkStats::~LinkStats() = default;
LinkStats::LinkStats(LinkStats && other) noexcept = default;
LinkStats & LinkStats::operator=(LinkStats && other) noexcept = default;

void LinkStats::add_sample(double latency_ms)
{
  impl_->samples.push_back(latency_ms);
}

std::size_t LinkStats::count() const
{
  return impl_->samples.size();
}

double LinkStats::mean() const
{
  if (impl_->samples.empty()) {
    return 0.0;
  }
  double total = 0.0;
  for (const double sample : impl_->samples) {
    total += sample;
  }
  return total / static_cast<double>(impl_->samples.size());
}

double LinkStats::max() const
{
  if (impl_->samples.empty()) {
    return 0.0;
  }
  return *std::max_element(impl_->samples.begin(), impl_->samples.end());
}
