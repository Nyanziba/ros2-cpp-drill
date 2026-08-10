// 解答例。
//
// 結城本 第15章 Facade。C++ では 2 通りに分かれます。
//   1. 状態を持たない窓口 → 名前空間 + 自由関数（robot::start_once）。クラスにしない
//   2. 状態を持つ窓口     → RAII クラス（robot::RobotSession）。これが本命
//
// 内部のサブシステムは無名名前空間に閉じ込め、ヘッダには一切出していません。

#include "drill/robot_startup.hpp"

#include <string>
#include <utility>
#include <vector>

namespace robot
{

namespace
{

void append(std::vector<std::string> * log, const char * entry)
{
  if (log != nullptr) {
    log->push_back(entry);
  }
}

bool power_on(const StartupConfig & config, std::vector<std::string> * log)
{
  if (config.battery_mv < kMinBatteryMv) {
    append(log, "power_on_failed");
    return false;
  }
  append(log, "power_on");
  return true;
}

void power_off(std::vector<std::string> * log)
{
  append(log, "power_off");
}

bool sensor_init(const StartupConfig & config, std::vector<std::string> * log)
{
  if (!config.sensor_present) {
    append(log, "sensor_init_failed");
    return false;
  }
  append(log, "sensor_init");
  return true;
}

void sensor_deinit(std::vector<std::string> * log)
{
  append(log, "sensor_deinit");
}

bool calibrate(const StartupConfig & config, std::vector<std::string> * log)
{
  if (!config.calibration_ok) {
    append(log, "calibrate_failed");
    return false;
  }
  append(log, "calibrate");
  return true;
}

void calibration_clear(std::vector<std::string> * log)
{
  append(log, "calibration_clear");
}

bool link_up(const StartupConfig & config, std::vector<std::string> * log)
{
  if (!config.link_ok) {
    append(log, "link_up_failed");
    return false;
  }
  append(log, "link_up");
  return true;
}

void link_down(std::vector<std::string> * log)
{
  append(log, "link_down");
}

/// 完了した段の数だけ、初期化と逆順で後始末する。
/// 段を増やすときに直すのはここと、起動側の 1 箇所だけで済む。
void teardown(int completed_stages, std::vector<std::string> * log)
{
  if (completed_stages >= 4) {
    link_down(log);
  }
  if (completed_stages >= 3) {
    calibration_clear(log);
  }
  if (completed_stages >= 2) {
    sensor_deinit(log);
  }
  if (completed_stages >= 1) {
    power_off(log);
  }
}

}  // namespace

StartupResult start_once(const StartupConfig & config, std::vector<std::string> * log)
{
  if (!power_on(config, log)) {
    teardown(0, log);
    return StartupResult{false, StartupStage::kPower};
  }
  if (!sensor_init(config, log)) {
    teardown(1, log);
    return StartupResult{false, StartupStage::kSensor};
  }
  if (!calibrate(config, log)) {
    teardown(2, log);
    return StartupResult{false, StartupStage::kCalibration};
  }
  if (!link_up(config, log)) {
    teardown(3, log);
    return StartupResult{false, StartupStage::kLink};
  }

  teardown(4, log);
  return StartupResult{true, StartupStage::kPower};
}

RobotSession::RobotSession(StartupConfig config, std::vector<std::string> * log)
: config_(std::move(config)),
  log_(log),
  completed_stages_(0),
  ready_(false),
  failed_stage_(StartupStage::kPower)
{
  if (!power_on(config_, log_)) {
    failed_stage_ = StartupStage::kPower;
    return;
  }
  ++completed_stages_;

  if (!sensor_init(config_, log_)) {
    failed_stage_ = StartupStage::kSensor;
    return;
  }
  ++completed_stages_;

  if (!calibrate(config_, log_)) {
    failed_stage_ = StartupStage::kCalibration;
    return;
  }
  ++completed_stages_;

  if (!link_up(config_, log_)) {
    failed_stage_ = StartupStage::kLink;
    return;
  }
  ++completed_stages_;

  ready_ = true;
}

RobotSession::~RobotSession()
{
  teardown(completed_stages_, log_);
}

RobotSession::RobotSession(RobotSession && other) noexcept
: config_(std::move(other.config_)),
  log_(other.log_),
  completed_stages_(other.completed_stages_),
  ready_(other.ready_),
  failed_stage_(other.failed_stage_)
{
  // ムーブ元を「何も持っていない」状態にする。
  // これを忘れると、ムーブ元のデストラクタでも後始末が走る。
  other.log_ = nullptr;
  other.completed_stages_ = 0;
  other.ready_ = false;
}

bool RobotSession::drive(int duty)
{
  if (!ready_) {
    return false;
  }
  if (log_ != nullptr) {
    log_->push_back("drive:" + std::to_string(duty));
  }
  return true;
}

}  // namespace robot
