// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <string>

#include "behaviortree_cpp/bt_factory.h"

namespace drill
{

// 1 回の tickOnce で黒板 trace を次のどれかにする。
// estop が true なら "estop"（time_up より優先）。
// そうでなく time_up なら木は FAILURE で、trace は空のまま。
//   この分岐は Inverter で TimeUp を包む。
// そうでなく range_mm < 400 なら "avoid"。
// そうでなく see_marker なら "score"。
// それ以外は "search"。
// range_mm == 400 は障害物ではありません。
std::string match_xml();
void register_match_nodes(BT::BehaviorTreeFactory & factory);

}  // namespace drill
