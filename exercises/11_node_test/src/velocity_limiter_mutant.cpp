// このファイルは編集しません（採点用にわざとバグを入れた実装）。
//
// 受講者には中身を見せない前提のファイルです。README にはバグの種類の
// ヒント（観点）だけを書いてあり、答えそのものはここにしかありません。
#include "drill/velocity_limiter.hpp"

#include <cmath>

double limit_velocity(double target, double previous, double max_speed, double max_delta)
{
  // バグ1: 負の値を「0として扱う」はずが、max_delta だけ絶対値を取ってしまっている。
  // 正の max_delta ではこのバグは表に出ない（abs(x) == max(0,x) が x>=0 で成立するため）。
  const double safe_max_delta = std::abs(max_delta);
  const double safe_max_speed = std::max(0.0, max_speed);

  const double delta = target - previous;
  double accel_limited;
  if (delta > safe_max_delta) {
    accel_limited = previous + safe_max_delta;
  } else if (delta < -safe_max_delta) {
    accel_limited = previous - safe_max_delta;
  } else {
    accel_limited = target;
  }

  // バグ2: 速度制限を正の側にしか適用していない。
  // 負方向に大きくはみ出した値はそのまま素通りしてしまう。
  if (accel_limited > safe_max_speed) {
    return safe_max_speed;
  }
  return accel_limited;
}
