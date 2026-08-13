// I AM NOT DONE
//
// 実務でよく見るほうの Builder（メソッドチェーンで引数を埋める）です。
// 結城本 第7章の Director + Builder とは別物なので、混ぜないでください。

#include "drill/motor_config.hpp"

#include <utility>

MotorConfigBuilder & MotorConfigBuilder::motor_id(std::uint8_t id)
{
  // TODO: config_.motor_id に id を入れ、has_motor_id_ を true にしてください。
  //
  // 戻り型が MotorConfigBuilder & であることが大事です。
  // MotorConfigBuilder（値）にすると、チェーンをつなぐたびに Builder ごとコピーされます。
  (void)id;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::name(std::string motor_name)
{
  // TODO: config_.name に motor_name を入れてください。
  //
  // 引数を値で受けているので、std::move で移してください。
  // ここでコピーすると、std::string の確保が 1 回余計に走ります。
  (void)motor_name;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::max_duty(double duty)
{
  // TODO: config_.max_duty に duty を入れてください。
  (void)duty;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::current_limit_ampere(double ampere)
{
  // TODO: config_.current_limit_ampere に ampere を入れてください。
  (void)ampere;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::encoder_counts_per_rev(std::uint32_t counts)
{
  // TODO: config_.encoder_counts_per_rev に counts を入れてください。
  (void)counts;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::invert_direction(bool inverted)
{
  // TODO: config_.invert_direction に inverted を入れてください。
  (void)inverted;
  return *this;
}

MotorConfigBuilder & MotorConfigBuilder::brake_on_stop(bool brake)
{
  // TODO: config_.brake_on_stop に brake を入れてください。
  (void)brake;
  return *this;
}

std::optional<MotorConfig> MotorConfigBuilder::build() const &
{
  // TODO: has_motor_id_ が false なら std::nullopt を返してください。
  //       そうでなければ config_ を**コピーして**返します。
  //
  // この版は左辺値（名前の付いた Builder）から呼ばれます。
  // Builder はこのあとも使われるかもしれないので、中身を奪ってはいけません。
  (void)has_motor_id_;
  return std::nullopt;
}

std::optional<MotorConfig> MotorConfigBuilder::build() &&
{
  // TODO: has_motor_id_ が false なら std::nullopt を返してください。
  //       そうでなければ config_ を **std::move して**返します。
  //
  // この版は右辺値（一時オブジェクト、または std::move された Builder）から呼ばれます。
  // もう誰も Builder を見ないので、std::string を丸ごと持っていって構いません。
  return std::nullopt;
}
