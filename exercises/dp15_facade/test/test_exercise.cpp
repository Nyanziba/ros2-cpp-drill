// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill_i18n.hpp"

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "drill/robot_startup.hpp"

namespace
{

using robot::RobotSession;
using robot::StartupConfig;
using robot::StartupStage;

using Log = std::vector<std::string>;

/// 全部成功する設定。
StartupConfig ok_config()
{
  return StartupConfig{};
}

/// RobotSession をスコープに入れて出すだけ。ログ全体を返します。
Log run_session(const StartupConfig & config)
{
  Log log;
  {
    const RobotSession session{config, &log};
    (void)session;
  }
  return log;
}

}  // namespace

TEST(FacadeTest, SuccessfulStartupRunsFourStepsInOrder)
{
  Log log;
  const RobotSession session{ok_config(), &log};

  EXPECT_TRUE(session.is_ready()) << drill::localized("全部成功する設定なのに起動できていません", "Every step is set to succeed, but startup failed");

  const Log expected = {"power_on", "sensor_init", "calibrate", "link_up"};
  EXPECT_EQ(log, expected)
    << drill::localized("Facade を 1 回作るだけで、内部の 4 手順がこの順に走るはずです", "Creating the Facade once should run the 4 internal steps in this order");
}

TEST(FacadeTest, LeavingScopeRunsCleanupInReverseOrder)
{
  Log log;
  {
    const RobotSession session{ok_config(), &log};
    ASSERT_TRUE(session.is_ready());
    const Log during = {"power_on", "sensor_init", "calibrate", "link_up"};
    ASSERT_EQ(log, during) << drill::localized("まだ後始末は走らないはずです", "Cleanup should not run yet");
  }

  const Log expected = {
    "power_on", "sensor_init", "calibrate", "link_up",
    "link_down", "calibration_clear", "sensor_deinit", "power_off"};
  EXPECT_EQ(log, expected)
    << drill::localized("デストラクタで、初期化と逆順に後始末するはずです", "The destructor should clean up in the reverse order of initialization");
}

TEST(FacadeTest, CalibrationFailureSkipsLaterSteps)
{
  StartupConfig config = ok_config();
  config.calibration_ok = false;

  Log log;
  {
    const RobotSession session{config, &log};
    EXPECT_FALSE(session.is_ready());
    EXPECT_EQ(session.failed_stage(), StartupStage::kCalibration);
  }

  const Log expected = {
    "power_on", "sensor_init", "calibrate_failed",
    "sensor_deinit", "power_off"};
  EXPECT_EQ(log, expected)
    << drill::localized("link_up が走ってはいけません。かつ、成功済みの 2 段だけが巻き戻るはずです", "link_up must not run, and only the 2 steps that succeeded should be rolled back");
}

TEST(FacadeTest, SensorInitFailureRollsBackOnlyPower)
{
  StartupConfig config = ok_config();
  config.sensor_present = false;

  Log log;
  {
    const RobotSession session{config, &log};
    EXPECT_FALSE(session.is_ready());
    EXPECT_EQ(session.failed_stage(), StartupStage::kSensor);
  }

  const Log expected = {"power_on", "sensor_init_failed", "power_off"};
  EXPECT_EQ(log, expected);
}

TEST(FacadeTest, PowerOnFailureRunsNoCleanup)
{
  StartupConfig config = ok_config();
  config.battery_mv = robot::kMinBatteryMv - 1;

  Log log;
  {
    const RobotSession session{config, &log};
    EXPECT_FALSE(session.is_ready());
    EXPECT_EQ(session.failed_stage(), StartupStage::kPower);
  }

  const Log expected = {"power_on_failed"};
  EXPECT_EQ(log, expected)
    << drill::localized("成功した段が 0 個なのだから、power_off を呼んではいけません", "No step succeeded, so power_off must not be called");
}

TEST(FacadeTest, LinkUpFailureRollsBackThreeSteps)
{
  StartupConfig config = ok_config();
  config.link_ok = false;

  Log log;
  {
    const RobotSession session{config, &log};
    EXPECT_FALSE(session.is_ready());
    EXPECT_EQ(session.failed_stage(), StartupStage::kLink);
  }

  const Log expected = {
    "power_on", "sensor_init", "calibrate", "link_up_failed",
    "calibration_clear", "sensor_deinit", "power_off"};
  EXPECT_EQ(log, expected);
}

TEST(FacadeTest, DriveFailsWhenNotStarted)
{
  StartupConfig config = ok_config();
  config.link_ok = false;

  Log log;
  RobotSession session{config, &log};
  ASSERT_FALSE(session.is_ready());
  EXPECT_EQ(session.failed_stage(), StartupStage::kLink);
  ASSERT_FALSE(log.empty()) << drill::localized("起動を試みた記録が残っていません", "There is no record of the startup attempt");
  const std::size_t before = log.size();

  EXPECT_FALSE(session.drive(50)) << drill::localized("起動していないのに drive できています", "drive succeeded although the session is not started");
  EXPECT_EQ(log.size(), before) << drill::localized("drive できないならログも残らないはずです", "If drive fails, nothing should be logged");
}

TEST(FacadeTest, DriveSucceedsWhenStarted)
{
  Log log;
  RobotSession session{ok_config(), &log};
  ASSERT_TRUE(session.is_ready());

  EXPECT_TRUE(session.drive(50));
  ASSERT_FALSE(log.empty());
  EXPECT_EQ(log.back(), "drive:50");
}

TEST(FacadeTest, FreeFunctionAndRaiiClassLogsMatch)
{
  StartupConfig fail_at_calibration = ok_config();
  fail_at_calibration.calibration_ok = false;

  StartupConfig fail_at_power = ok_config();
  fail_at_power.battery_mv = robot::kMinBatteryMv - 1;

  const StartupConfig configs[] = {ok_config(), fail_at_calibration, fail_at_power};

  for (const StartupConfig & config : configs) {
    Log from_free_function;
    const robot::StartupResult result = robot::start_once(config, &from_free_function);
    const Log from_session = run_session(config);

    EXPECT_EQ(from_free_function, from_session)
      << drill::localized("名前空間 + 自由関数版と RAII クラス版で、走る手順が違います", "The free-function version and the RAII class version run different steps");
    EXPECT_FALSE(from_free_function.empty()) << drill::localized("start_once() が何もしていません", "start_once() does nothing");

    RobotSession probe{config, nullptr};
    EXPECT_EQ(result.ok, probe.is_ready());
    if (!result.ok) {
      EXPECT_EQ(result.failed_stage, probe.failed_stage());
    }
  }
}

TEST(FacadeTest, CleanupRunsOnlyOnceAfterMove)
{
  Log log;
  {
    RobotSession original{ok_config(), &log};
    ASSERT_TRUE(original.is_ready());

    const RobotSession moved{std::move(original)};
    EXPECT_TRUE(moved.is_ready()) << drill::localized("ムーブ先が起動状態を引き継いでいません", "The moved-to object did not take over the started state");
  }

  const Log expected = {
    "power_on", "sensor_init", "calibrate", "link_up",
    "link_down", "calibration_clear", "sensor_deinit", "power_off"};
  EXPECT_EQ(log, expected)
    << drill::localized("後始末が 2 回走っています。ムーブ元を空にしましたか", "Cleanup ran twice. Did you empty the moved-from object?");
}

TEST(FacadeTest, SessionTypeProperties)
{
  static_assert(
    !std::is_copy_constructible<RobotSession>::value,
    "起動済みのハードウェア 1 台を表す型はコピーできてはいけません");
  static_assert(
    !std::is_copy_assignable<RobotSession>::value,
    "コピー代入も禁止です");
  static_assert(
    std::is_move_constructible<RobotSession>::value,
    "関数から返せるようにムーブ構築は許します");
  static_assert(
    !std::is_convertible<StartupConfig, RobotSession>::value,
    "コンストラクタは explicit です。StartupConfig から暗黙変換されてはいけません");
  static_assert(
    !std::is_move_assignable<RobotSession>::value,
    "ムーブ代入は禁止です");

  // 型の性質だけでは実装の有無が分からないので、1 つだけ実挙動も見ておく。
  Log log;
  const RobotSession session{ok_config(), &log};
  EXPECT_TRUE(session.is_ready());
}
