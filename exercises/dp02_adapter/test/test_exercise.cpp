// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <cstddef>
#include <iterator>
#include <memory>
#include <vector>

#include "drill/motor_adapter.hpp"

TEST(AdapterTest, 委譲版が角速度をパルスに変換する)
{
  DelegatingMotorAdapter adapter{LegacyMotorDriver{}};

  adapter.set_velocity(1.0);
  EXPECT_EQ(adapter.raw().getPulse(), 100);

  adapter.set_velocity(-2.5);
  EXPECT_EQ(adapter.raw().getPulse(), -250);
}

TEST(AdapterTest, 委譲版がドライバの上限で丸められる)
{
  DelegatingMotorAdapter adapter{LegacyMotorDriver{}};

  adapter.set_velocity(50.0);  // 5000 パルス相当。ドライバ側で 1000 に丸まる
  EXPECT_EQ(adapter.raw().getPulse(), LegacyMotorDriver::MAX_PULSE);
}

TEST(AdapterTest, 委譲版のstopがパルスをゼロにする)
{
  DelegatingMotorAdapter adapter{LegacyMotorDriver{}};

  adapter.set_velocity(3.0);
  ASSERT_EQ(adapter.raw().getPulse(), 300);  // 先に動いていること
  adapter.stop();
  EXPECT_EQ(adapter.raw().getPulse(), 0);
}

TEST(AdapterTest, 委譲版がエンコーダ生値をradに変換する)
{
  DelegatingMotorAdapter adapter{LegacyMotorDriver{}};

  adapter.raw().injectEncoderRaw(400);
  EXPECT_DOUBLE_EQ(adapter.position_rad(), 2.0);

  adapter.raw().injectEncoderRaw(-100);
  EXPECT_DOUBLE_EQ(adapter.position_rad(), -0.5);
}

TEST(AdapterTest, 継承版が委譲版とまったく同じ結果になる)
{
  DelegatingMotorAdapter delegating{LegacyMotorDriver{}};
  InheritingMotorAdapter inheriting{LegacyMotorDriver{}};

  const double velocities[] = {0.0, 1.0, -2.5, 7.25, 50.0};
  const int expected_pulses[] = {0, 100, -250, 725, LegacyMotorDriver::MAX_PULSE};
  for (std::size_t i = 0; i < std::size(velocities); ++i) {
    delegating.set_velocity(velocities[i]);
    inheriting.set_velocity(velocities[i]);
    EXPECT_EQ(inheriting.getPulse(), expected_pulses[i]);
    EXPECT_EQ(delegating.raw().getPulse(), inheriting.getPulse());
  }

  delegating.stop();
  inheriting.stop();
  EXPECT_EQ(delegating.raw().getPulse(), 0);
  EXPECT_EQ(inheriting.getPulse(), 0);

  delegating.raw().injectEncoderRaw(300);
  inheriting.injectEncoderRaw(300);
  EXPECT_DOUBLE_EQ(delegating.position_rad(), inheriting.position_rad());
}

TEST(AdapterTest, 基底ポインタ越しに同じように扱える)
{
  LegacyMotorDriver driver;
  driver.injectEncoderRaw(400);

  std::vector<std::unique_ptr<MotorActuator>> motors;
  motors.push_back(std::make_unique<DelegatingMotorAdapter>(driver));
  motors.push_back(std::make_unique<InheritingMotorAdapter>(driver));

  for (const std::unique_ptr<MotorActuator> & motor : motors) {
    motor->set_velocity(1.5);
    EXPECT_DOUBLE_EQ(motor->position_rad(), 2.0);
    motor->stop();
  }

  // 上位コードは LegacyMotorDriver の存在を一切知らない。
  // ここで motors が破棄される。MotorActuator の仮想デストラクタが効いている。
}
