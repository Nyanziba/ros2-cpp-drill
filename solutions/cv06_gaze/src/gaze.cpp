#include "drill/gaze.hpp"

#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/condition_node.h"

namespace drill
{
namespace
{

double clamp(double value, double low, double high)
{
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}

bool better(const GazeDetection & left, const GazeDetection & right)
{
  if (left.rank != right.rank) {
    return left.rank < right.rank;
  }
  if (left.score != right.score) {
    return left.score > right.score;
  }
  return left.id < right.id;
}

void servo(GazeMemory & memory, const GazeDetection & target, const CameraModel & camera)
{
  const double error_x = target.cx - static_cast<double>(camera.width) / 2.0;
  const double error_y = target.cy - static_cast<double>(camera.height) / 2.0;
  const double pan_rate = clamp(camera.kp * error_x, -camera.max_rate, camera.max_rate);
  const double tilt_rate = clamp(-camera.kp * error_y, -camera.max_rate, camera.max_rate);
  memory.target_id = target.id;
  memory.target_rank = target.rank;
  memory.lost_frames = 0;
  memory.pan_deg = clamp(memory.pan_deg + pan_rate * camera.dt, camera.pan_min, camera.pan_max);
  memory.tilt_deg = clamp(memory.tilt_deg + tilt_rate * camera.dt, camera.tilt_min, camera.tilt_max);
}

class FlagCondition : public BT::ConditionNode
{
public:
  FlagCondition(const std::string & name, const BT::NodeConfig & config, std::string key)
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

class EstopOn : public FlagCondition
{
public:
  EstopOn(const std::string & name, const BT::NodeConfig & config)
  : FlagCondition(name, config, "estop") {}
};

class TargetOn : public FlagCondition
{
public:
  TargetOn(const std::string & name, const BT::NodeConfig & config)
  : FlagCondition(name, config, "target_visible") {}
};

class SetMode : public BT::SyncActionNode
{
public:
  SetMode(const std::string & name, const BT::NodeConfig & config)
  : BT::SyncActionNode(name, config) {}

  static BT::PortsList providedPorts()
  {
    return {BT::InputPort<std::string>("mode")};
  }

  BT::NodeStatus tick() override
  {
    const auto mode = getInput<std::string>("mode");
    if (!mode) {
      return BT::NodeStatus::FAILURE;
    }
    config().blackboard->set("gaze_mode", mode.value());
    return BT::NodeStatus::SUCCESS;
  }
};

}  // namespace

GazeMemory update_gaze(
  const GazeMemory & previous,
  const std::vector<GazeDetection> & detections,
  const CameraModel & camera)
{
  GazeMemory next = previous;
  const GazeDetection * best = nullptr;
  const GazeDetection * current = nullptr;
  for (const auto & detection : detections) {
    if (!detection.visible) {
      continue;
    }
    if (best == nullptr || better(detection, *best)) {
      best = &detection;
    }
    if (previous.target_id != -1 && detection.id == previous.target_id) {
      current = &detection;
    }
  }

  if (current != nullptr) {
    const GazeDetection * chosen = current;
    if (best != nullptr && best->rank < current->rank) {
      chosen = best;
    }
    servo(next, *chosen, camera);
    return next;
  }
  if (previous.target_id != -1 && best != nullptr && best->rank < previous.target_rank) {
    servo(next, *best, camera);
    return next;
  }
  if (previous.target_id != -1 && previous.lost_frames + 1 < kGazeLostLimit) {
    next.lost_frames = previous.lost_frames + 1;
    return next;
  }
  if (best != nullptr) {
    servo(next, *best, camera);
    return next;
  }
  next.target_id = -1;
  next.lost_frames = 0;
  return next;
}

std::string gaze_xml()
{
  return R"(<root BTCPP_format="4">
  <BehaviorTree ID="Gaze">
    <Fallback>
      <Sequence>
        <EstopOn/>
        <SetMode mode="hold"/>
      </Sequence>
      <Sequence>
        <TargetOn/>
        <SetMode mode="track"/>
      </Sequence>
      <SetMode mode="search"/>
    </Fallback>
  </BehaviorTree>
</root>
)";
}

void register_gaze_nodes(BT::BehaviorTreeFactory & factory)
{
  factory.registerNodeType<EstopOn>("EstopOn");
  factory.registerNodeType<TargetOn>("TargetOn");
  factory.registerNodeType<SetMode>("SetMode");
}

}  // namespace drill
