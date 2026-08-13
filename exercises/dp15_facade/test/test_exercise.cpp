// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

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

TEST(FacadeTest, 起動に成功すると4手順が正しい順序で走る)
{
  Log log;
  const RobotSession session{ok_config(), &log};

  EXPECT_TRUE(session.is_ready()) << "全部成功する設定なのに起動できていません";

  const Log expected = {"power_on", "sensor_init", "calibrate", "link_up"};
  EXPECT_EQ(log, expected)
    << "Facade を 1 回作るだけで、内部の 4 手順がこの順に走るはずです";
}

TEST(FacadeTest, スコープを抜けると後始末が逆順で走る)
{
  Log log;
  {
    const RobotSession session{ok_config(), &log};
    ASSERT_TRUE(session.is_ready());
    const Log during = {"power_on", "sensor_init", "calibrate", "link_up"};
    ASSERT_EQ(log, during) << "まだ後始末は走らないはずです";
  }

  const Log expected = {
    "power_on", "sensor_init", "calibrate", "link_up",
    "link_down", "calibration_clear", "sensor_deinit", "power_off"};
  EXPECT_EQ(log, expected)
    << "デストラクタで、初期化と逆順に後始末するはずです";
}

TEST(FacadeTest, キャリブレーションで失敗するとそれ以降は走らない)
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
    << "link_up が走ってはいけません。かつ、成功済みの 2 段だけが巻き戻るはずです";
}

TEST(FacadeTest, センサ初期化で失敗すると電源だけが巻き戻る)
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

TEST(FacadeTest, 電源投入で失敗すると後始末は何も走らない)
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
    << "成功した段が 0 個なのだから、power_off を呼んではいけません";
}

TEST(FacadeTest, 通信確立で失敗すると3段が巻き戻る)
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

TEST(FacadeTest, 起動できていなければdriveできない)
{
  StartupConfig config = ok_config();
  config.link_ok = false;

  Log log;
  RobotSession session{config, &log};
  ASSERT_FALSE(session.is_ready());
  EXPECT_EQ(session.failed_stage(), StartupStage::kLink);
  ASSERT_FALSE(log.empty()) << "起動を試みた記録が残っていません";
  const std::size_t before = log.size();

  EXPECT_FALSE(session.drive(50)) << "起動していないのに drive できています";
  EXPECT_EQ(log.size(), before) << "drive できないならログも残らないはずです";
}

TEST(FacadeTest, 起動していればdriveできる)
{
  Log log;
  RobotSession session{ok_config(), &log};
  ASSERT_TRUE(session.is_ready());

  EXPECT_TRUE(session.drive(50));
  ASSERT_FALSE(log.empty());
  EXPECT_EQ(log.back(), "drive:50");
}

TEST(FacadeTest, 自由関数版とRAIIクラス版のログが一致する)
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
      << "名前空間 + 自由関数版と RAII クラス版で、走る手順が違います";
    EXPECT_FALSE(from_free_function.empty()) << "start_once() が何もしていません";

    RobotSession probe{config, nullptr};
    EXPECT_EQ(result.ok, probe.is_ready());
    if (!result.ok) {
      EXPECT_EQ(result.failed_stage, probe.failed_stage());
    }
  }
}

TEST(FacadeTest, ムーブしても後始末は一度だけ走る)
{
  Log log;
  {
    RobotSession original{ok_config(), &log};
    ASSERT_TRUE(original.is_ready());

    const RobotSession moved{std::move(original)};
    EXPECT_TRUE(moved.is_ready()) << "ムーブ先が起動状態を引き継いでいません";
  }

  const Log expected = {
    "power_on", "sensor_init", "calibrate", "link_up",
    "link_down", "calibration_clear", "sensor_deinit", "power_off"};
  EXPECT_EQ(log, expected)
    << "後始末が 2 回走っています。ムーブ元を空にしましたか";
}

TEST(FacadeTest, セッションの型の性質)
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
