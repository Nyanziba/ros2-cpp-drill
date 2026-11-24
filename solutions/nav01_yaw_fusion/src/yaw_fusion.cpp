#include "drill/yaw_fusion.hpp"

#include <cmath>

namespace drill
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

}  // namespace

double wrap_to_pi(double angle)
{
  while (angle > kPi) {
    angle -= 2.0 * kPi;
  }
  while (angle < -kPi) {
    angle += 2.0 * kPi;
  }
  return angle;
}

double fuse_yaw(double previous, double odom_yaw, double gyro_rad_s, double dt)
{
  const double predicted = wrap_to_pi(previous + gyro_rad_s * dt);
  const double innovation = wrap_to_pi(odom_yaw - predicted);
  return wrap_to_pi(predicted + 0.02 * innovation);
}

bool odom_in_gate(double predicted, double odom_yaw, double gate_rad)
{
  return std::abs(wrap_to_pi(odom_yaw - predicted)) <= gate_rad;
}

double fuse_yaw_gated(
  double previous, double odom_yaw, double gyro_rad_s, double dt, double gate_rad)
{
  const double predicted = wrap_to_pi(previous + gyro_rad_s * dt);
  if (!odom_in_gate(predicted, odom_yaw, gate_rad)) {
    return predicted;
  }
  return fuse_yaw(previous, odom_yaw, gyro_rad_s, dt);
}

}  // namespace drill
