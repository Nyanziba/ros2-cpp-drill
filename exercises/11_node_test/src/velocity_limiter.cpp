// このファイルは編集しません（正しい実装）。
#include "drill/velocity_limiter.hpp"

#include <algorithm>

double limit_velocity(double target, double previous, double max_speed, double max_delta)
{
  // 負が来たら 0 として扱う。
  const double safe_max_speed = std::max(0.0, max_speed);
  const double safe_max_delta = std::max(0.0, max_delta);

  // 1. 加速度制限: 変化量の絶対値を max_delta 以内に収める。
  const double delta = target - previous;
  const double clamped_delta = std::clamp(delta, -safe_max_delta, safe_max_delta);
  const double accel_limited = previous + clamped_delta;

  // 2. 速度制限: 結果の絶対値を max_speed 以内に収める（正負どちらの向きも）。
  return std::clamp(accel_limited, -safe_max_speed, safe_max_speed);
}
