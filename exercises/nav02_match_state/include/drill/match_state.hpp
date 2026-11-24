// このファイルは編集しません（インタフェースの提示）。
#pragma once

namespace drill
{

enum class MatchState
{
  WaitStart,
  Search,
  Approach,
  Align,
  Place,
  Finished,
  TimedOut,
};

enum class MatchEvent
{
  StartSignal,
  MarkerSeen,
  MarkerLost,
  Aligned,
  Placed,
  Scored,
  TimeUp,
};

enum class TaskKind
{
  MatchPhase,
  ObstacleAvoidance,
  DriveToZone,
};

enum class Controller
{
  StateMachine,
  BehaviorTree,
  StateMachineCallsBehaviorTree,
};

// TimeUp は Finished 以外を TimedOut にする。Finished の TimeUp は Finished のまま。
// TimedOut と Finished は他のイベントでは動かない。
// WaitStart + StartSignal -> Search
// Search + MarkerSeen -> Approach
// Approach + MarkerLost -> Search、Approach + Aligned -> Align
// Align + MarkerLost -> Search、Align + Placed -> Place
// Place + MarkerLost -> Search、Place + Scored -> Finished
// それ以外のイベントは状態を変えない。
MatchState next_state(MatchState current, MatchEvent event);

// 試合の相は状態機械。障害物回避はビヘイビアツリー。
// ゾーンまでの走行は、状態の中からビヘイビアツリーを tick する。
Controller choose_controller(TaskKind task);

}  // namespace drill
