// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <string>

#include "behaviortree_cpp/bt_factory.h"

namespace drill
{

enum class AvoidCommand
{
  Stop,
  DodgeLeft,
  DodgeRight,
  Go,
  Search,
};

// front < 150 は Stop（接触する前に止まる）。
// それ以外で front < 400 は、left >= right なら DodgeLeft、そうでなければ DodgeRight。
// それ以外で goal_visible なら Go。残りは Search。
AvoidCommand decide_avoidance(int front_mm, int left_mm, int right_mm, bool goal_visible);

// 黒板 front_mm, left_mm, right_mm, goal_visible を読み、
// command を stop / left / right / go / search にする。
// decide_avoidance と同じ結果になること。XML には Fallback と Sequence を含める。
std::string avoidance_xml();
void register_avoid_nodes(BT::BehaviorTreeFactory & factory);

}  // namespace drill
