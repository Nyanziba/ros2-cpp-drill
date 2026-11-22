// I AM NOT DONE
//
// 前方が近すぎるときは避けずに止まり、余裕がある側へかわしてください。

#include "drill/avoidance.hpp"

namespace drill
{

AvoidCommand decide_avoidance(int front_mm, int left_mm, int right_mm, bool goal_visible)
{
  (void)front_mm;
  (void)left_mm;
  (void)right_mm;
  (void)goal_visible;
  return AvoidCommand::Search;
}

std::string avoidance_xml()
{
  return
    "<root BTCPP_format=\"4\"><BehaviorTree ID=\"Avoid\">"
    "<AlwaysSuccess/></BehaviorTree></root>";
}

void register_avoid_nodes(BT::BehaviorTreeFactory & factory)
{
  (void)factory;
}

}  // namespace drill
