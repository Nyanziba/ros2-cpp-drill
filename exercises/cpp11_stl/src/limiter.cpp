// I AM NOT DONE
//
// 標準ライブラリを使ってください。

#include "drill/limiter.hpp"
#include <algorithm>

double clamp_velocity(double value, double min_val, double max_val)
{
  // TODO: std::clamp を使って value を [min_val, max_val] の範囲に限定してください。
  return value;
}

std::optional<int> find_user_id(const std::map<std::string, int> & users, const std::string & name)
{
  // TODO: map から name に対応する id を探してください。
  // 見つかれば std::optional で id を返し、見つからなければ std::nullopt を返してください。
  return std::nullopt;
}
