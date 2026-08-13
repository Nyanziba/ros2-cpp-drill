// このファイルは編集しません（インタフェースの提示）。
//
// Adapter を 2 通りで用意します。**中身（src/motor_adapter.cpp）だけ書いてください。**
//
//   1. DelegatingMotorAdapter … 委譲版。生ドライバを「メンバとして持つ」
//   2. InheritingMotorAdapter … 継承版。生ドライバを「private 継承する」
//
// 結城本と同じく両方書いて、テストで**同じ振る舞いになること**を確かめます。
// そのうえで、記事の 2.4 で「なぜ実務では委譲版一択なのか」を判断してください。
#pragma once

#include <utility>

#include "drill/legacy_motor_driver.hpp"
#include "drill/motor_actuator.hpp"

/// 委譲版の Adapter。生ドライバを **メンバとして所有**する（has-a）。
class DelegatingMotorAdapter : public MotorActuator
{
public:
  explicit DelegatingMotorAdapter(LegacyMotorDriver driver)
  : driver_(std::move(driver))
  {
  }

  void set_velocity(double rad_per_sec) override;
  void stop() override;
  double position_rad() const override;

  /// テストから生ドライバの状態を覗くための窓。
  const LegacyMotorDriver & raw() const { return driver_; }
  LegacyMotorDriver & raw() { return driver_; }

private:
  LegacyMotorDriver driver_;
};

/// 継承版の Adapter。生ドライバを **private 継承**する
/// （"is-implemented-in-terms-of"。is-a ではない）。
///
/// 注意: public 継承にすると「MotorActuator でありながら setPulse() も生で呼べる」
/// 型になり、単位系を隠すという Adapter の目的が崩れます。だから private です。
class InheritingMotorAdapter : public MotorActuator, private LegacyMotorDriver
{
public:
  explicit InheritingMotorAdapter(LegacyMotorDriver driver)
  : LegacyMotorDriver(std::move(driver))
  {
  }

  void set_velocity(double rad_per_sec) override;
  void stop() override;
  double position_rad() const override;

  // private 継承したメンバのうち、テストから見たいものだけを選んで公開する。
  // これができるのは private 継承の数少ない利点。
  using LegacyMotorDriver::getPulse;
  using LegacyMotorDriver::injectEncoderRaw;
};
