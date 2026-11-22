// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill/match_state.hpp"
#include "drill_i18n.hpp"

TEST(MatchState, PlaysAPointAndStopsOnTime)
{
  using drill::MatchEvent;
  using drill::MatchState;
  EXPECT_EQ(drill::next_state(MatchState::WaitStart, MatchEvent::StartSignal), MatchState::Search);
  EXPECT_EQ(drill::next_state(MatchState::Search, MatchEvent::MarkerSeen), MatchState::Approach);
  EXPECT_EQ(drill::next_state(MatchState::Approach, MatchEvent::MarkerLost), MatchState::Search);
  EXPECT_EQ(drill::next_state(MatchState::Approach, MatchEvent::Aligned), MatchState::Align);
  EXPECT_EQ(drill::next_state(MatchState::Align, MatchEvent::Placed), MatchState::Place);
  EXPECT_EQ(drill::next_state(MatchState::Place, MatchEvent::Scored), MatchState::Finished);
  EXPECT_EQ(drill::next_state(MatchState::Approach, MatchEvent::TimeUp), MatchState::TimedOut);
  EXPECT_EQ(drill::next_state(MatchState::Finished, MatchEvent::TimeUp), MatchState::Finished);
  EXPECT_EQ(drill::next_state(MatchState::TimedOut, MatchEvent::StartSignal), MatchState::TimedOut);
  EXPECT_EQ(drill::next_state(MatchState::Search, MatchEvent::Placed), MatchState::Search);
}

TEST(MatchState, PicksTheControllerForTheJob)
{
  EXPECT_EQ(drill::choose_controller(drill::TaskKind::MatchPhase), drill::Controller::StateMachine);
  EXPECT_EQ(
    drill::choose_controller(drill::TaskKind::ObstacleAvoidance), drill::Controller::BehaviorTree)
    << drill::localized(
      "回避は状態を増やさず、木の優先度で処理します。",
      "Avoidance stays in the tree, not in another match state.");
  EXPECT_EQ(
    drill::choose_controller(drill::TaskKind::DriveToZone),
    drill::Controller::StateMachineCallsBehaviorTree);
}
