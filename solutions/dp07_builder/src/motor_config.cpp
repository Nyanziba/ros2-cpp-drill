// 解答例
//
// 実務でよく見るほうの Builder（メソッドチェーンで引数を埋める）。
// 結城本 第7章の Director + Builder とは別物です。

#include "drill/motor_config.hpp"

#include <utility>

MotorConfigBuilder & MotorConfigBuilder::motor_id(std::uint8_t id)
{
  config_.motor_id = id;
  has_motor_id_ = true;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::name(std::string motor_name)
{
  // 値で受けて move で移す。呼び出し側が一時オブジェクトを渡したときに確保が 1 回で済みます。
  config_.name = std::move(motor_name);
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::max_duty(double duty)
{
  config_.max_duty = duty;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::current_limit_ampere(double ampere)
{
  config_.current_limit_ampere = ampere;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::encoder_counts_per_rev(std::uint32_t counts)
{
  config_.encoder_counts_per_rev = counts;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::invert_direction(bool inverted)
{
  config_.invert_direction = inverted;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::brake_on_stop(bool brake)
{
  config_.brake_on_stop = brake;
  return *this;
}

std::optional<MotorConfig> MotorConfigBuilder::build() const &
{
  if (!has_motor_id_) {
    // 例外を投げません。マイコンでは -fno-exceptions が普通だからです。
    return std::nullopt;
  }
  // 左辺値から呼ばれています。Builder はこのあとも使えなければいけないのでコピー。
  return config_;
}

std::optional<MotorConfig> MotorConfigBuilder::build() &&
{
  if (!has_motor_id_) {
    return std::nullopt;
  }
  // 右辺値から呼ばれています。もう誰も見ないので中身を持っていきます。
  return std::move(config_);
}
