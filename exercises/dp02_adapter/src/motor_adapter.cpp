// I AM NOT DONE
//
// 委譲版と継承版、両方の Adapter を実装してください。
// 振る舞いは 2 つとも完全に同じにします（テストで突き合わせます）。
//
// 変換の規則:
//   パルス指令 = std::lround(rad_per_sec * PULSES_PER_RAD_PER_SEC)
//   角度 [rad]  = readEncoderRaw() / COUNTS_PER_RAD

#include "drill/motor_adapter.hpp"

#include <cmath>

// ---- 委譲版 -------------------------------------------------------------

void DelegatingMotorAdapter::set_velocity(double rad_per_sec)
{
  // TODO: rad_per_sec をパルスに換算して driver_.setPulse() を呼んでください。
  (void)rad_per_sec;
}

void DelegatingMotorAdapter::stop()
{
  // TODO: driver_.stopAll() を呼んでください。
}

double DelegatingMotorAdapter::position_rad() const
{
  // TODO: driver_.readEncoderRaw() を rad に換算して返してください。
  return 0.0;
}

// ---- 継承版 -------------------------------------------------------------
//
// private 継承なので、基底のメンバ関数は setPulse(...) のように
// **オブジェクトを介さず直接**呼べます（自分自身が LegacyMotorDriver だから）。

void InheritingMotorAdapter::set_velocity(double rad_per_sec)
{
  // TODO: setPulse() を呼んでください。
  (void)rad_per_sec;
}

void InheritingMotorAdapter::stop()
{
  // TODO: stopAll() を呼んでください。
}

double InheritingMotorAdapter::position_rad() const
{
  // TODO: readEncoderRaw() を rad に換算して返してください。
  return 0.0;
}
