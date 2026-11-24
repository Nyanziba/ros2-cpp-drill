#include "drill/priority_tree.hpp"

#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/condition_node.h"

namespace drill
{
namespace
{

class BoolCondition : public BT::ConditionNode
{
public:
  BoolCondition(const std::string & name, const BT::NodeConfig & config, std::string key)
  : BT::ConditionNode(name, config), key_(std::move(key)) {}

  static BT::PortsList providedPorts() {return {};}

  BT::NodeStatus tick() override
  {
    bool value = false;
    (void)config().blackboard->get(key_, value);
    return value ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
  }

private:
  std::string key_;
};

class EstopPressed : public BoolCondition
{
public:
  EstopPressed(const std::string & name, const BT::NodeConfig & config)
  : BoolCondition(name, config, "estop") {}
};

class TimeUp : public BoolCondition
{
public:
  TimeUp(const std::string & name, const BT::NodeConfig & config)
  : BoolCondition(name, config, "time_up") {}
};

class SeeMarker : public BoolCondition
{
public:
  SeeMarker(const std::string & name, const BT::NodeConfig & config)
  : BoolCondition(name, config, "see_marker") {}
};

class IsObstacleNear : public BT::ConditionNode
{
public:
  IsObstacleNear(const std::string & name, const BT::NodeConfig & config)
  : BT::ConditionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {BT::InputPort<int>("range_mm")};
  }

  BT::NodeStatus tick() override
  {
    const auto range = getInput<int>("range_mm");
    if (!range || range.value() >= 400) {
      return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
  }
};

class Trace : public BT::SyncActionNode
{
public:
  Trace(const std::string & name, const BT::NodeConfig & config)
  : BT::SyncActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {BT::InputPort<std::string>("label")};
  }

  BT::NodeStatus tick() override
  {
    const auto label = getInput<std::string>("label");
    if (!label) {
      return BT::NodeStatus::FAILURE;
    }
    std::string trace;
    (void)config().blackboard->get("trace", trace);
    if (!trace.empty()) {
      trace += ",";
    }
    trace += label.value();
    config().blackboard->set("trace", trace);
    return BT::NodeStatus::SUCCESS;
  }
};

}  // namespace

std::string match_xml()
{
  return R"(<root BTCPP_format="4">
  <BehaviorTree ID="Match">
    <Fallback>
      <Sequence>
        <EstopPressed/>
        <Trace label="estop"/>
      </Sequence>
      <Sequence>
        <Inverter>
          <TimeUp/>
        </Inverter>
        <Fallback>
          <Sequence>
            <IsObstacleNear range_mm="{range_mm}"/>
            <Trace label="avoid"/>
          </Sequence>
          <Sequence>
            <SeeMarker/>
            <Trace label="score"/>
          </Sequence>
          <Trace label="search"/>
        </Fallback>
      </Sequence>
    </Fallback>
  </BehaviorTree>
</root>
)";
}

void register_match_nodes(BT::BehaviorTreeFactory & factory)
{
  factory.registerNodeType<EstopPressed>("EstopPressed");
  factory.registerNodeType<TimeUp>("TimeUp");
  factory.registerNodeType<SeeMarker>("SeeMarker");
  factory.registerNodeType<IsObstacleNear>("IsObstacleNear");
  factory.registerNodeType<Trace>("Trace");
}

}  // namespace drill
