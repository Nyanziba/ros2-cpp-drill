// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "behaviortree_cpp/bt_factory.h"

namespace drill
{

// action_msgs/msg/GoalStatus と同じ番号です。
constexpr int8_t kActionExecuting = 2;
constexpr int8_t kActionCanceling = 3;
constexpr int8_t kActionSucceeded = 4;
constexpr int8_t kActionCanceled = 5;
constexpr int8_t kActionAborted = 6;

struct DriveGoal
{
  std::string zone;
  double timeout_sec{0.0};
};

// time_left_sec < 0.5 なら zone は "abort"、timeout_sec は 0。送ってはいけません。
// それ以外の timeout_sec は min(3.0, time_left_sec)。zone はそのまま。
DriveGoal make_drive_goal(const std::string & zone, double time_left_sec);

// EXECUTING と CANCELING は RUNNING、SUCCEEDED は SUCCESS、それ以外は FAILURE。
BT::NodeStatus status_from_action(int8_t code);

// テストが差し込むアクションクライアント。実機では rclcpp_action のクライアントです。
class DriveClient
{
public:
  virtual ~DriveClient() = default;
  virtual void send(const DriveGoal & goal) = 0;
  virtual void cancel() = 0;
  virtual int8_t status() const = 0;
};

using DriveClientPtr = std::shared_ptr<DriveClient>;

// 黒板 drive_client（DriveClientPtr）、time_left（double）、ポート zone。
// onStart でゴールを送り RUNNING。abort なら送らず FAILURE。
// onRunning は status_from_action。onHalted は cancel。
std::string drive_xml();
void register_drive_nodes(BT::BehaviorTreeFactory & factory);

}  // namespace drill
