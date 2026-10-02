// 模範解答: 課題11「テストを書く」
//
// 正しい実装 (velocity_limiter) では全て通り、
// バグ入り実装 (velocity_limiter_mutant) ではいずれかが落ちることを狙っている。
#include <gtest/gtest.h>

#include "drill/velocity_limiter.hpp"

TEST(VelocityLimiterTest, ReturnsTargetUnchangedWhenNoLimitApplies)
{
  const double result = limit_velocity(/*target=*/1.0, /*previous=*/0.0,
    /*max_speed=*/10.0, /*max_delta=*/10.0);
  EXPECT_DOUBLE_EQ(result, 1.0);
}

TEST(VelocityLimiterTest, AccelerationLimitAppliesInBothDirections)
{
  // target - previous = 5.0 だが max_delta = 1.0 なので +1.0 までしか動けない。
  EXPECT_DOUBLE_EQ(limit_velocity(5.0, 0.0, 100.0, 1.0), 1.0);
  // 逆方向も同様に -1.0 までしか動けない。
  EXPECT_DOUBLE_EQ(limit_velocity(-5.0, 0.0, 100.0, 1.0), -1.0);
}

TEST(VelocityLimiterTest, SpeedLimitAppliesInNegativeDirection)
{
  // previous を先に -8 まで動かしておき、target も -8 のままにすることで
  // 加速度制限にはかからないようにする（delta = 0）。
  // その状態で max_speed = 5 を指定すれば、速度制限だけが -5 に抑えるはず。
  EXPECT_DOUBLE_EQ(limit_velocity(-8.0, -8.0, 5.0, 100.0), -5.0);
}

TEST(VelocityLimiterTest, SpeedLimitAppliesInPositiveDirection)
{
  EXPECT_DOUBLE_EQ(limit_velocity(8.0, 8.0, 5.0, 100.0), 5.0);
}

TEST(VelocityLimiterTest, AccelerationLimitIsAppliedBeforeSpeedLimit)
{
  // previous=0, target=100, max_delta=3 でまず 3 まで制限され、
  // その後 max_speed=2 でさらに制限されて最終的に 2 になるはず。
  EXPECT_DOUBLE_EQ(limit_velocity(100.0, 0.0, 2.0, 3.0), 2.0);
}

TEST(VelocityLimiterTest, NegativeMaxDeltaIsTreatedAsZero)
{
  // 変化量がどれだけあっても previous からまったく動けないはず。
  EXPECT_DOUBLE_EQ(limit_velocity(10.0, 0.0, 100.0, -2.0), 0.0);
}

TEST(VelocityLimiterTest, NegativeMaxSpeedIsTreatedAsZero)
{
  EXPECT_DOUBLE_EQ(limit_velocity(1.0, 0.0, -5.0, 100.0), 0.0);
}

TEST(VelocityLimiterTest, DeltaExactlyMaxDeltaPassesThrough)
{
  EXPECT_DOUBLE_EQ(limit_velocity(3.0, 0.0, 100.0, 3.0), 3.0);
}

TEST(VelocityLimiterTest, ResultExactlyMaxSpeedPassesThrough)
{
  EXPECT_DOUBLE_EQ(limit_velocity(5.0, 5.0, 5.0, 100.0), 5.0);
}
