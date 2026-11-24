// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>

#include "drill/yaw_fusion.hpp"
#include "drill_i18n.hpp"

#ifndef DRILL_EKF_YAML
#error "DRILL_EKF_YAML is not set"
#endif

TEST(YawFusion, WrapsAndMixesALittleOdometry)
{
  EXPECT_NEAR(drill::wrap_to_pi(0.0), 0.0, 1e-12);
  EXPECT_NEAR(drill::wrap_to_pi(3.14159265358979323846 + 0.1), -3.041592653589793, 1e-9);
  EXPECT_NEAR(drill::fuse_yaw(0.0, 0.0, 0.0, 0.1), 0.0, 1e-12);
  EXPECT_NEAR(drill::fuse_yaw(0.0, 0.0, 1.0, 0.1), 0.098, 1e-12);
  EXPECT_NEAR(drill::fuse_yaw(3.1, -3.1, 0.0, 0.1), 3.1016637061435916, 1e-9) << drill::localized(
    "±pi をまたぐ差を wrap してから 0.02 倍してください。",
    "Wrap the difference across ±pi before scaling it by 0.02.");
}

TEST(YawFusion, RejectsWheelSlip)
{
  const double predicted = drill::wrap_to_pi(0.2);
  EXPECT_TRUE(drill::odom_in_gate(predicted, 0.3, 0.2));
  EXPECT_FALSE(drill::odom_in_gate(predicted, 1.0, 0.2));
  EXPECT_NEAR(drill::fuse_yaw_gated(0.2, 1.0, 0.0, 0.1, 0.2), 0.2, 1e-12);
  EXPECT_NEAR(drill::fuse_yaw_gated(0.0, 0.0, 1.0, 0.1, 1.0), 0.098, 1e-12);
}

TEST(YawFusion, EkfYamlIsTwoDimensional)
{
  std::ifstream input(DRILL_EKF_YAML);
  ASSERT_TRUE(input) << DRILL_EKF_YAML;
  std::stringstream buffer;
  buffer << input.rdbuf();
  const std::string yaml = buffer.str();
  const char * required[] = {
    "frequency: 30.0",
    "two_d_mode: true",
    "publish_tf: true",
    "world_frame: odom",
    "odom_frame: odom",
    "base_link_frame: base_link",
    "odom0: /odom",
    "odom0_config: [true, true, false, false, false, true, true, false, false, false, false, true, false, false, false]",
    "imu0: /imu",
    "imu0_config: [false, false, false, false, false, true, false, false, false, false, false, true, true, false, false]",
    "imu0_remove_gravitational_acceleration: true",
  };
  for (const char * needle : required) {
    EXPECT_NE(yaml.find(needle), std::string::npos) << needle;
  }
}

TEST(YawFusion, RobotLocalizationLaunchIsSkipped)
{
  GTEST_SKIP() << drill::localized(
    "robot_localization のノード起動は TF と実センサが要るので CI ではスキップします。"
    " 融合式と ekf.yaml を採点します。",
    "Skipped: launching robot_localization needs TF and real sensors. "
    "CI grades the fusion formula and ekf.yaml.");
}
