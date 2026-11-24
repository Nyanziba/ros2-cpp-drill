// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "drill/field_route.hpp"
#include "drill_i18n.hpp"

TEST(FieldRoute, RejectsARouteThatOverrunsTheMatch)
{
  const std::vector<drill::Waypoint> points{
    {"ball", 1.0, 0.5, 20.0},
    {"goal", 2.0, 1.5, 25.0},
    {"home", 0.0, 0.0, 30.0},
  };
  const drill::RoutePlan over = drill::plan_field_route(points, 60.0);
  EXPECT_FALSE(over.feasible);
  EXPECT_NEAR(over.total_sec, 75.0, 1e-9);
  EXPECT_EQ(over.names, (std::vector<std::string>{"ball", "goal", "home"}));
  const drill::RoutePlan fit = drill::plan_field_route(points, 75.0);
  EXPECT_TRUE(fit.feasible);
  EXPECT_FALSE(drill::plan_field_route({}, 10.0).feasible);
  drill::Waypoint bad = points.front();
  bad.leg_sec = -1.0;
  EXPECT_FALSE(drill::plan_field_route({bad}, 10.0).feasible);
}

TEST(FieldRoute, NeedsMapToOdomToBase)
{
  EXPECT_TRUE(drill::field_frames_connected({{"map", "odom"}, {"odom", "base_link"}}));
  EXPECT_FALSE(drill::field_frames_connected({{"map", "base_link"}})) << drill::localized(
    "map から base_link へ直行してはいけません。odom を挟みます。",
    "Do not connect map directly to base_link. odom sits between them.");
  EXPECT_EQ(drill::slam_mode_name(true, false), "localization");
  EXPECT_EQ(drill::slam_mode_name(true, true), "mapping");
  EXPECT_EQ(drill::slam_mode_name(false, false), "mapping");
}

TEST(FieldRoute, RecoveryXmlMatches)
{
  const std::string with_spin = drill::nav_recovery_xml(true);
  const std::string expected =
    "<RecoveryNode number_of_retries=\"1\" name=\"FieldRecovery\">\n"
    "  <Sequence name=\"primary\">\n"
    "    <Wait wait_duration=\"0.5\"/>\n"
    "  </Sequence>\n"
    "  <Sequence name=\"recovery\">\n"
    "    <BackUp backup_dist=\"0.30\" backup_speed=\"0.15\"/>\n"
    "    <Spin spin_dist=\"1.57\"/>\n"
    "  </Sequence>\n"
    "</RecoveryNode>\n";
  EXPECT_EQ(with_spin, expected);
  EXPECT_EQ(drill::nav_recovery_xml(false).find("Spin"), std::string::npos);
}

TEST(FieldRoute, Nav2AndSlamLaunchAreSkipped)
{
  GTEST_SKIP() << drill::localized(
    "slam_toolbox と Nav2 の起動は地図とシミュレーションが要るので CI ではスキップします。"
    " 経路の時間、TF の並び、SLAM のモード、復帰 XML を採点します。",
    "Skipped: launching slam_toolbox and Nav2 needs a map and a simulation. "
    "CI grades the route time, the TF chain, the SLAM mode, and the recovery XML.");
}
