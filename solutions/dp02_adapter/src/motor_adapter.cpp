#include "drill/motor_adapter.hpp"

#include <cmath>

namespace
{

/// 角速度 [rad/s] を生ドライバのパルス指令に換算する。
int to_pulse(double rad_per_sec)
{
  return static_cast<int>(std::lround(rad_per_sec * PULSES_PER_RAD_PER_SEC));
}

/// エンコーダ生カウントを角度 [rad] に換算する。
double to_radian(std::int32_t encoder_raw)
{
  return static_cast<double>(encoder_raw) / COUNTS_PER_RAD;
}

}  // namespace

// ---- 委譲版 -------------------------------------------------------------

void DelegatingMotorAdapter::set_velocity(double rad_per_sec)
{
  driver_.setPulse(to_pulse(rad_per_sec));
}

void DelegatingMotorAdapter::stop()
{
  driver_.stopAll();
}

double DelegatingMotorAdapter::position_rad() const
{
  return to_radian(driver_.readEncoderRaw());
}

// ---- 継承版 -------------------------------------------------------------

void InheritingMotorAdapter::set_velocity(double rad_per_sec)
{
  setPulse(to_pulse(rad_per_sec));
}

void InheritingMotorAdapter::stop()
{
  stopAll();
}

double InheritingMotorAdapter::position_rad() const
{
  return to_radian(readEncoderRaw());
}
