// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>

#include "drill/avoidance.hpp"
#include "drill_i18n.hpp"

namespace
{

std::string command_of(drill::AvoidCommand command)
{
  switch (command) {
    case drill::AvoidCommand::Stop:
      return "stop";
    case drill::AvoidCommand::DodgeLeft:
      return "left";
    case drill::AvoidCommand::DodgeRight:
      return "right";
    case drill::AvoidCommand::Go:
      return "go";
    case drill::AvoidCommand::Search:
      return "search";
  }
  return "";
}

std::string tree_command(int front, int left, int right, bool goal)
{
  BT::BehaviorTreeFactory factory;
  drill::register_avoid_nodes(factory);
  auto tree = factory.createTreeFromText(drill::avoidance_xml());
  tree.rootBlackboard()->set("front_mm", front);
  tree.rootBlackboard()->set("left_mm", left);
  tree.rootBlackboard()->set("right_mm", right);
  tree.rootBlackboard()->set("goal_visible", goal);
  tree.rootBlackboard()->set("command", std::string("unset"));
  tree.tickOnce();
  std::string command = "unset";
  (void)tree.rootBlackboard()->get("command", command);
  return command;
}

}  // namespace

TEST(Avoidance, StopsBeforeDodging)
{
  EXPECT_EQ(drill::decide_avoidance(100, 800, 100, true), drill::AvoidCommand::Stop);
  EXPECT_EQ(drill::decide_avoidance(149, 10, 800, false), drill::AvoidCommand::Stop);
  EXPECT_EQ(drill::decide_avoidance(150, 500, 100, false), drill::AvoidCommand::DodgeLeft);
  EXPECT_EQ(drill::decide_avoidance(200, 100, 100, true), drill::AvoidCommand::DodgeLeft)
    << drill::localized("左右が同じときは左です。", "A tie dodges left.");
  EXPECT_EQ(drill::decide_avoidance(200, 100, 400, false), drill::AvoidCommand::DodgeRight);
  EXPECT_EQ(drill::decide_avoidance(400, 0, 0, true), drill::AvoidCommand::Go);
  EXPECT_EQ(drill::decide_avoidance(800, 0, 0, false), drill::AvoidCommand::Search);
}

TEST(AvoidanceTree, MatchesTheFunction)
{
  const std::string xml = drill::avoidance_xml();
  EXPECT_NE(xml.find("Fallback"), std::string::npos);
  EXPECT_NE(xml.find("Sequence"), std::string::npos);
  const int fronts[] = {100, 200, 200, 400, 900};
  const int lefts[] = {10, 500, 100, 0, 0};
  const int rights[] = {10, 100, 400, 0, 0};
  const bool goals[] = {true, false, true, true, false};
  for (int i = 0; i < 5; ++i) {
    const auto decided = drill::decide_avoidance(fronts[i], lefts[i], rights[i], goals[i]);
    EXPECT_EQ(tree_command(fronts[i], lefts[i], rights[i], goals[i]), command_of(decided));
  }
}
