#include "drill/match_state.hpp"

namespace drill
{

MatchState next_state(MatchState current, MatchEvent event)
{
  if (event == MatchEvent::TimeUp) {
    return current == MatchState::Finished ? MatchState::Finished : MatchState::TimedOut;
  }
  if (current == MatchState::TimedOut || current == MatchState::Finished) {
    return current;
  }
  switch (current) {
    case MatchState::WaitStart:
      if (event == MatchEvent::StartSignal) {
        return MatchState::Search;
      }
      break;
    case MatchState::Search:
      if (event == MatchEvent::MarkerSeen) {
        return MatchState::Approach;
      }
      break;
    case MatchState::Approach:
      if (event == MatchEvent::MarkerLost) {
        return MatchState::Search;
      }
      if (event == MatchEvent::Aligned) {
        return MatchState::Align;
      }
      break;
    case MatchState::Align:
      if (event == MatchEvent::MarkerLost) {
        return MatchState::Search;
      }
      if (event == MatchEvent::Placed) {
        return MatchState::Place;
      }
      break;
    case MatchState::Place:
      if (event == MatchEvent::MarkerLost) {
        return MatchState::Search;
      }
      if (event == MatchEvent::Scored) {
        return MatchState::Finished;
      }
      break;
    case MatchState::Finished:
    case MatchState::TimedOut:
      break;
  }
  return current;
}

Controller choose_controller(TaskKind task)
{
  switch (task) {
    case TaskKind::MatchPhase:
      return Controller::StateMachine;
    case TaskKind::ObstacleAvoidance:
      return Controller::BehaviorTree;
    case TaskKind::DriveToZone:
      return Controller::StateMachineCallsBehaviorTree;
  }
  return Controller::StateMachine;
}

}  // namespace drill
