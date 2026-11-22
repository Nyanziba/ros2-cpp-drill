#include "drill/avoidance.hpp"

#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/condition_node.h"

namespace drill
{
namespace
{

class RangeCondition : public BT::ConditionNode
{
public:
  RangeCondition(const std::string & name, const BT::NodeConfig & config, int limit)
  : BT::ConditionNode(name, config), limit_(limit) {}

  static BT::PortsList providedPorts()
  {
    return {BT::InputPort<int>("front_mm")};
  }

  BT::NodeStatus tick() override
  {
    const auto front = getInput<int>("front_mm");
    if (!front || front.value() >= limit_) {
      return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
  }

private:
  int limit_;
};

class TooClose : public RangeCondition
{
public:
  TooClose(const std::string & name, const BT::NodeConfig & config)
  : RangeCondition(name, config, 150) {}
};

class FrontBlocked : public RangeCondition
{
public:
  FrontBlocked(const std::string & name, const BT::NodeConfig & config)
  : RangeCondition(name, config, 400) {}
};

class GoalVisible : public BT::ConditionNode
{
public:
  GoalVisible(const std::string & name, const BT::NodeConfig & config)
  : BT::ConditionNode(name, config) {}

  static BT::PortsList providedPorts() {return {};}

  BT::NodeStatus tick() override
  {
    bool visible = false;
    (void)config().blackboard->get("goal_visible", visible);
    return visible ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
  }
};

class SetCommand : public BT::SyncActionNode
{
public:
  SetCommand(const std::string & name, const BT::NodeConfig & config)
  : BT::SyncActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {BT::InputPort<std::string>("command")};
  }

  BT::NodeStatus tick() override
  {
    const auto command = getInput<std::string>("command");
    if (!command) {
      return BT::NodeStatus::FAILURE;
    }
    config().blackboard->set("command", command.value());
    return BT::NodeStatus::SUCCESS;
  }
};

class SetDodge : public BT::SyncActionNode
{
public:
  SetDodge(const std::string & name, const BT::NodeConfig & config)
  : BT::SyncActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {BT::InputPort<int>("left_mm"), BT::InputPort<int>("right_mm")};
  }

  BT::NodeStatus tick() override
  {
    const auto left = getInput<int>("left_mm");
    const auto right = getInput<int>("right_mm");
    if (!left || !right) {
      return BT::NodeStatus::FAILURE;
    }
    config().blackboard->set(
      "command", std::string(left.value() >= right.value() ? "left" : "right"));
    return BT::NodeStatus::SUCCESS;
  }
};

}  // namespace

AvoidCommand decide_avoidance(int front_mm, int left_mm, int right_mm, bool goal_visible)
{
  if (front_mm < 150) {
    return AvoidCommand::Stop;
  }
  if (front_mm < 400) {
    return left_mm >= right_mm ? AvoidCommand::DodgeLeft : AvoidCommand::DodgeRight;
  }
  if (goal_visible) {
    return AvoidCommand::Go;
  }
  return AvoidCommand::Search;
}

std::string avoidance_xml()
{
  return R"(<root BTCPP_format="4">
  <BehaviorTree ID="Avoid">
    <Fallback>
      <Sequence>
        <TooClose front_mm="{front_mm}"/>
        <SetCommand command="stop"/>
      </Sequence>
      <Sequence>
        <FrontBlocked front_mm="{front_mm}"/>
        <SetDodge left_mm="{left_mm}" right_mm="{right_mm}"/>
      </Sequence>
      <Sequence>
        <GoalVisible/>
        <SetCommand command="go"/>
      </Sequence>
      <SetCommand command="search"/>
    </Fallback>
  </BehaviorTree>
</root>
)";
}

void register_avoid_nodes(BT::BehaviorTreeFactory & factory)
{
  factory.registerNodeType<TooClose>("TooClose");
  factory.registerNodeType<FrontBlocked>("FrontBlocked");
  factory.registerNodeType<GoalVisible>("GoalVisible");
  factory.registerNodeType<SetCommand>("SetCommand");
  factory.registerNodeType<SetDodge>("SetDodge");
}

}  // namespace drill
