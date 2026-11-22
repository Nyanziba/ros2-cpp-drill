// このファイルは編集しません（インタフェースの提示）。
#pragma once

namespace drill
{

// 角度を [-pi, pi] に畳み込みます。
double wrap_to_pi(double angle);

// predicted = wrap(previous + gyro_rad_s * dt)
// fused = wrap(predicted + 0.02 * wrap(odom_yaw - predicted))
// 係数 0.02 は (1 - 0.98)。ホイールのヨーを少しだけ混ぜます。
double fuse_yaw(double previous, double odom_yaw, double gyro_rad_s, double dt);

// abs(wrap(odom_yaw - predicted)) <= gate_rad なら true。
bool odom_in_gate(double predicted, double odom_yaw, double gate_rad);

// ゲートの外なら predicted を返し、オドメトリは使いません。中なら fuse_yaw です。
double fuse_yaw_gated(
  double previous, double odom_yaw, double gyro_rad_s, double dt, double gate_rad);

}  // namespace drill
