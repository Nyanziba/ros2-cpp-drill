// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>

#include "drill/priority_tree.hpp"
#include "drill_i18n.hpp"

namespace
{

std::string trace_of(bool estop, bool time_up, int range_mm, bool see_marker, BT::NodeStatus & status)
{
  BT::BehaviorTreeFactory factory;
  drill::register_match_nodes(factory);
  auto tree = factory.createTreeFromText(drill::match_xml());
  tree.rootBlackboard()->set("estop", estop);
  tree.rootBlackboard()->set("time_up", time_up);
  tree.rootBlackboard()->set("range_mm", range_mm);
  tree.rootBlackboard()->set("see_marker", see_marker);
  tree.rootBlackboard()->set("trace", std::string(""));
  status = tree.tickOnce();
  std::string trace;
  (void)tree.rootBlackboard()->get("trace", trace);
  return trace;
}

}  // namespace

TEST(PriorityTree, XmlInvertsTimeUp)
{
  EXPECT_NE(drill::match_xml().find("Inverter"), std::string::npos);
  EXPECT_NE(drill::match_xml().find("Fallback"), std::string::npos);
}

TEST(PriorityTree, EstopOutranksEverything)
{
  BT::NodeStatus status = BT::NodeStatus::IDLE;
  EXPECT_EQ(trace_of(true, true, 100, true, status), "estop");
  EXPECT_EQ(status, BT::NodeStatus::SUCCESS);
}

TEST(PriorityTree, TimeUpStopsTheRest)
{
  BT::NodeStatus status = BT::NodeStatus::IDLE;
  EXPECT_EQ(trace_of(false, true, 100, true, status), "") << drill::localized(
    "時間切れのあとに avoid や score を動かさないでください。",
    "Do not run avoid or score after time is up.");
  EXPECT_EQ(status, BT::NodeStatus::FAILURE);
}

TEST(PriorityTree, AvoidBeatsScoreAndSearchIsLast)
{
  BT::NodeStatus status = BT::NodeStatus::IDLE;
  EXPECT_EQ(trace_of(false, false, 399, true, status), "avoid");
  EXPECT_EQ(trace_of(false, false, 400, true, status), "score");
  EXPECT_EQ(trace_of(false, false, 1000, false, status), "search");
  EXPECT_EQ(status, BT::NodeStatus::SUCCESS);
}
