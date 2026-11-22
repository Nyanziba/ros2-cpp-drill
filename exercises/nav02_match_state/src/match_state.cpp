// I AM NOT DONE
//
// 3 分の試合の相は状態で覚えてください。回避のような反射は木に残します。

#include "drill/match_state.hpp"

namespace drill
{

MatchState next_state(MatchState current, MatchEvent event)
{
  (void)event;
  return current;
}

Controller choose_controller(TaskKind task)
{
  (void)task;
  return Controller::StateMachine;
}

}  // namespace drill
