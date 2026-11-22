// I AM NOT DONE
//
// ±pi をまたぐ平均は向きが反転します。innovation を wrap してから混ぜてください。

#include "drill/yaw_fusion.hpp"

namespace drill
{

double wrap_to_pi(double angle)
{
  (void)angle;
  return 0.0;
}

double fuse_yaw(double previous, double odom_yaw, double gyro_rad_s, double dt)
{
  (void)previous;
  (void)odom_yaw;
  (void)gyro_rad_s;
  (void)dt;
  return 0.0;
}

bool odom_in_gate(double predicted, double odom_yaw, double gate_rad)
{
  (void)predicted;
  (void)odom_yaw;
  (void)gate_rad;
  return false;
}

double fuse_yaw_gated(
  double previous, double odom_yaw, double gyro_rad_s, double dt, double gate_rad)
{
  (void)previous;
  (void)odom_yaw;
  (void)gyro_rad_s;
  (void)dt;
  (void)gate_rad;
  return 0.0;
}

}  // namespace drill
