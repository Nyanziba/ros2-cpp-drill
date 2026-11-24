// I AM NOT DONE
//
// SyncActionNode で結果を待つと、そのあいだ非常停止が tick されません。
// StatefulActionNode でゴールを送り、RUNNING のまま木に戻してください。

#include "drill/drive_tree.hpp"

namespace drill
{

DriveGoal make_drive_goal(const std::string & zone, double time_left_sec)
{
  (void)zone;
  (void)time_left_sec;
  return {};
}

BT::NodeStatus status_from_action(int8_t code)
{
  (void)code;
  return BT::NodeStatus::FAILURE;
}

std::string drive_xml()
{
  return
    "<root BTCPP_format=\"4\"><BehaviorTree ID=\"Drive\">"
    "<AlwaysSuccess/></BehaviorTree></root>";
}

void register_drive_nodes(BT::BehaviorTreeFactory & factory)
{
  (void)factory;
}

}  // namespace drill
