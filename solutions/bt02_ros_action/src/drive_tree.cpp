#include "drill/drive_tree.hpp"

#include <algorithm>

#include "behaviortree_cpp/action_node.h"

namespace drill
{
namespace
{

class DriveToZone : public BT::StatefulActionNode
{
public:
  DriveToZone(const std::string & name, const BT::NodeConfig & config)
  : BT::StatefulActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {BT::InputPort<std::string>("zone")};
  }

  BT::NodeStatus onStart() override
  {
    const auto zone = getInput<std::string>("zone");
    double time_left = 0.0;
    DriveClientPtr client;
    if (!zone || !config().blackboard->get("time_left", time_left) ||
      !config().blackboard->get("drive_client", client) || !client)
    {
      return BT::NodeStatus::FAILURE;
    }
    const DriveGoal goal = make_drive_goal(zone.value(), time_left);
    if (goal.zone == "abort") {
      return BT::NodeStatus::FAILURE;
    }
    client->send(goal);
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    DriveClientPtr client;
    if (!config().blackboard->get("drive_client", client) || !client) {
      return BT::NodeStatus::FAILURE;
    }
    return status_from_action(client->status());
  }

  void onHalted() override
  {
    DriveClientPtr client;
    if (config().blackboard->get("drive_client", client) && client) {
      client->cancel();
    }
  }
};

}  // namespace

DriveGoal make_drive_goal(const std::string & zone, double time_left_sec)
{
  DriveGoal goal;
  if (time_left_sec < 0.5) {
    goal.zone = "abort";
    goal.timeout_sec = 0.0;
    return goal;
  }
  goal.zone = zone;
  goal.timeout_sec = std::min(3.0, time_left_sec);
  return goal;
}

BT::NodeStatus status_from_action(int8_t code)
{
  if (code == kActionExecuting || code == kActionCanceling) {
    return BT::NodeStatus::RUNNING;
  }
  if (code == kActionSucceeded) {
    return BT::NodeStatus::SUCCESS;
  }
  return BT::NodeStatus::FAILURE;
}

std::string drive_xml()
{
  return R"(<root BTCPP_format="4">
  <BehaviorTree ID="Drive">
    <DriveToZone zone="{zone}"/>
  </BehaviorTree>
</root>
)";
}

void register_drive_nodes(BT::BehaviorTreeFactory & factory)
{
  factory.registerNodeType<DriveToZone>("DriveToZone");
}

}  // namespace drill
