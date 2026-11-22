// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>

#include "drill/drive_tree.hpp"
#include "drill_i18n.hpp"

namespace
{

class FakeDrive : public drill::DriveClient
{
public:
  void send(const drill::DriveGoal & goal) override
  {
    sent = true;
    this->goal = goal;
  }
  void cancel() override {++cancels;}
  int8_t status() const override {return code;}

  bool sent{false};
  int cancels{0};
  int8_t code{drill::kActionExecuting};
  drill::DriveGoal goal;
};

BT::Tree make_tree(const drill::DriveClientPtr & client, double time_left, const std::string & zone)
{
  BT::BehaviorTreeFactory factory;
  drill::register_drive_nodes(factory);
  auto tree = factory.createTreeFromText(drill::drive_xml());
  tree.rootBlackboard()->set("drive_client", client);
  tree.rootBlackboard()->set("time_left", time_left);
  tree.rootBlackboard()->set("zone", zone);
  return tree;
}

}  // namespace

TEST(DriveGoal, CapsTimeoutAndAbortsLate)
{
  const drill::DriveGoal full = drill::make_drive_goal("red", 10.0);
  EXPECT_EQ(full.zone, "red");
  EXPECT_NEAR(full.timeout_sec, 3.0, 1e-9);
  const drill::DriveGoal short_trip = drill::make_drive_goal("blue", 1.25);
  EXPECT_EQ(short_trip.zone, "blue");
  EXPECT_NEAR(short_trip.timeout_sec, 1.25, 1e-9);
  const drill::DriveGoal late = drill::make_drive_goal("red", 0.4);
  EXPECT_EQ(late.zone, "abort");
  EXPECT_NEAR(late.timeout_sec, 0.0, 1e-9);
}

TEST(DriveGoal, MapsActionStatus)
{
  EXPECT_EQ(drill::status_from_action(drill::kActionExecuting), BT::NodeStatus::RUNNING);
  EXPECT_EQ(drill::status_from_action(drill::kActionCanceling), BT::NodeStatus::RUNNING);
  EXPECT_EQ(drill::status_from_action(drill::kActionSucceeded), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(drill::status_from_action(drill::kActionAborted), BT::NodeStatus::FAILURE);
  EXPECT_EQ(drill::status_from_action(drill::kActionCanceled), BT::NodeStatus::FAILURE);
}

TEST(DriveTree, SendsGoalThenSucceeds)
{
  auto client = std::make_shared<FakeDrive>();
  auto tree = make_tree(client, 10.0, "blue");
  EXPECT_EQ(tree.tickOnce(), BT::NodeStatus::RUNNING);
  ASSERT_TRUE(client->sent);
  EXPECT_EQ(client->goal.zone, "blue");
  EXPECT_NEAR(client->goal.timeout_sec, 3.0, 1e-9);
  client->code = drill::kActionSucceeded;
  EXPECT_EQ(tree.tickOnce(), BT::NodeStatus::SUCCESS);
}

TEST(DriveTree, DoesNotSendWhenTimeIsAlmostUp)
{
  auto client = std::make_shared<FakeDrive>();
  auto tree = make_tree(client, 0.2, "red");
  EXPECT_EQ(tree.tickOnce(), BT::NodeStatus::FAILURE) << drill::localized(
    "残り 0.5 秒未満ではゴールを送らないでください。",
    "Do not send a goal when less than 0.5 seconds remain.");
  EXPECT_FALSE(client->sent);
}

TEST(DriveTree, HaltCancels)
{
  auto client = std::make_shared<FakeDrive>();
  auto tree = make_tree(client, 2.0, "red");
  ASSERT_EQ(tree.tickOnce(), BT::NodeStatus::RUNNING);
  tree.haltTree();
  EXPECT_EQ(client->cancels, 1);
}
