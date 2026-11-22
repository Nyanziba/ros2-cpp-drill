// I AM NOT DONE
//
// Fallback の並びが優先度です。非常停止、時間切れ、回避、得点、探索の順にしてください。

#include "drill/priority_tree.hpp"

namespace drill
{

std::string match_xml()
{
  return
    "<root BTCPP_format=\"4\"><BehaviorTree ID=\"Match\">"
    "<AlwaysSuccess/></BehaviorTree></root>";
}

void register_match_nodes(BT::BehaviorTreeFactory & factory)
{
  (void)factory;
}

}  // namespace drill
