// 解答例
//
// Template Method を NVI (Non-Virtual Interface) で書いています。
// 骨格 read_once() は非仮想。差し替えたい部分だけが仮想関数です。

#include "drill/sensor_reader.hpp"

#include <cmath>
#include <utility>

// ---------------------------------------------------------------------------
// SensorReader — 手順の骨格
// ---------------------------------------------------------------------------

void SensorReader::record(const char * step_name)
{
  call_log_.emplace_back(step_name);
}

bool SensorReader::validate(double converted_value) const
{
  return std::isfinite(converted_value);
}

std::optional<double> SensorReader::read_once()
{
  if (!is_initialized_) {
    record("initialize");
    initialize();
    is_initialized_ = true;
  }

  record("fetch_raw");
  const int raw_value = fetch_raw();

  record("convert");
  const double converted_value = convert(raw_value);

  record("validate");
  if (!validate(converted_value)) {
    return std::nullopt;
  }
  return converted_value;
}

// ---------------------------------------------------------------------------
// EncoderReader — カウント値 → 角度[deg]
// ---------------------------------------------------------------------------

EncoderReader::EncoderReader(std::vector<int> samples, double counts_per_revolution)
: samples_(std::move(samples)), counts_per_revolution_(counts_per_revolution)
{
}

void EncoderReader::initialize()
{
  next_index_ = 0;
}

int EncoderReader::fetch_raw()
{
  if (next_index_ >= samples_.size()) {
    return 0;
  }
  const int raw_value = samples_[next_index_];
  ++next_index_;
  return raw_value;
}

double EncoderReader::convert(int raw_value) const
{
  constexpr double kDegreesPerRevolution = 360.0;
  return static_cast<double>(raw_value) / counts_per_revolution_ * kDegreesPerRevolution;
}

// ---------------------------------------------------------------------------
// ThermistorReader — AD 値 → 温度[degC]
// ---------------------------------------------------------------------------

ThermistorReader::ThermistorReader(std::vector<int> samples)
: samples_(std::move(samples))
{
}

void ThermistorReader::initialize()
{
  next_index_ = 0;
}

int ThermistorReader::fetch_raw()
{
  if (next_index_ >= samples_.size()) {
    return 0;
  }
  const int raw_value = samples_[next_index_];
  ++next_index_;
  return raw_value;
}

double ThermistorReader::convert(int raw_value) const
{
  constexpr double kCelsiusPerCount = 0.1;
  constexpr double kCelsiusOffset = -20.0;
  return static_cast<double>(raw_value) * kCelsiusPerCount + kCelsiusOffset;
}

bool ThermistorReader::validate(double converted_value) const
{
  if (!SensorReader::validate(converted_value)) {
    return false;
  }
  return converted_value >= kMinValidCelsius && converted_value <= kMaxValidCelsius;
}
