// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>

#include "drill/gaze.hpp"
#include "drill_i18n.hpp"

namespace
{

drill::CameraModel camera()
{
  drill::CameraModel model;
  model.width = 100;
  model.height = 100;
  model.kp = 1.0;
  model.max_rate = 4.0;
  model.pan_min = -90.0;
  model.pan_max = 90.0;
  model.tilt_min = -30.0;
  model.tilt_max = 40.0;
  model.dt = 0.5;
  return model;
}

std::string mode_after(bool estop, bool visible)
{
  BT::BehaviorTreeFactory factory;
  drill::register_gaze_nodes(factory);
  auto tree = factory.createTreeFromText(drill::gaze_xml());
  tree.rootBlackboard()->set("estop", estop);
  tree.rootBlackboard()->set("target_visible", visible);
  tree.rootBlackboard()->set("gaze_mode", std::string("unset"));
  tree.tickOnce();
  std::string mode = "unset";
  (void)tree.rootBlackboard()->get("gaze_mode", mode);
  return mode;
}

}  // namespace

TEST(Gaze, ServosTowardGameObject)
{
  drill::GazeDetection game;
  game.id = 7;
  game.rank = 0;
  game.score = 1.0;
  game.cx = 60.0;
  game.cy = 40.0;
  game.visible = true;
  const drill::GazeMemory moved = drill::update_gaze({}, {game}, camera());
  EXPECT_EQ(moved.target_id, 7);
  EXPECT_EQ(moved.target_rank, 0);
  EXPECT_EQ(moved.lost_frames, 0);
  EXPECT_NEAR(moved.pan_deg, 2.0, 1e-9);
  EXPECT_NEAR(moved.tilt_deg, 2.0, 1e-9) << drill::localized(
    "画像の上がチルト正です。cy 40 は中心より上です。",
    "Positive tilt looks up. cy 40 is above the center.");
}

TEST(Gaze, HigherRankPreemptsAndSameRankDoesNot)
{
  drill::GazeMemory locked;
  locked.target_id = 2;
  locked.target_rank = 1;
  locked.pan_deg = 10.0;
  locked.tilt_deg = 1.0;
  drill::GazeDetection person;
  person.id = 2;
  person.rank = 1;
  person.score = 0.4;
  person.cx = 50.0;
  person.cy = 50.0;
  person.visible = true;
  drill::GazeDetection game;
  game.id = 3;
  game.rank = 0;
  game.score = 0.2;
  game.cx = 60.0;
  game.cy = 50.0;
  game.visible = true;
  const drill::GazeMemory preempted = drill::update_gaze(locked, {person, game}, camera());
  EXPECT_EQ(preempted.target_id, 3);
  EXPECT_NEAR(preempted.pan_deg, 12.0, 1e-9);
  EXPECT_NEAR(preempted.tilt_deg, 1.0, 1e-9);

  drill::GazeMemory on_game;
  on_game.target_id = 5;
  on_game.target_rank = 0;
  on_game.pan_deg = 4.0;
  drill::GazeDetection current = game;
  current.id = 5;
  current.score = 0.5;
  current.cx = 50.0;
  drill::GazeDetection louder = game;
  louder.id = 4;
  louder.score = 0.9;
  louder.cx = 80.0;
  const drill::GazeMemory kept = drill::update_gaze(on_game, {current, louder}, camera());
  EXPECT_EQ(kept.target_id, 5) << drill::localized(
    "同じ rank の高スコアには乗り換えません。",
    "Do not switch to a higher score of the same rank.");
}

TEST(Gaze, HoldsThroughFlickerThenDrops)
{
  drill::GazeMemory locked;
  locked.target_id = 1;
  locked.target_rank = 0;
  locked.pan_deg = 8.0;
  locked.tilt_deg = -2.0;
  drill::GazeDetection person;
  person.id = 9;
  person.rank = 1;
  person.visible = true;
  person.cx = 90.0;
  person.cy = 90.0;
  const drill::GazeMemory held = drill::update_gaze(locked, {person}, camera());
  EXPECT_EQ(held.target_id, 1);
  EXPECT_EQ(held.lost_frames, 1);
  EXPECT_NEAR(held.pan_deg, 8.0, 1e-9);

  locked.lost_frames = 4;
  const drill::GazeMemory dropped = drill::update_gaze(locked, {}, camera());
  EXPECT_EQ(dropped.target_id, -1);
  EXPECT_EQ(dropped.lost_frames, 0);
  EXPECT_NEAR(dropped.pan_deg, 8.0, 1e-9);
}

TEST(Gaze, ClampsPan)
{
  drill::GazeMemory locked;
  locked.pan_deg = 89.0;
  drill::GazeDetection game;
  game.id = 1;
  game.rank = 0;
  game.visible = true;
  game.cx = 1000.0;
  game.cy = 50.0;
  const drill::GazeMemory moved = drill::update_gaze(locked, {game}, camera());
  EXPECT_NEAR(moved.pan_deg, 90.0, 1e-9);
}

TEST(GazeTree, XmlUsesFallbackAndSequence)
{
  const std::string xml = drill::gaze_xml();
  EXPECT_NE(xml.find("Fallback"), std::string::npos);
  EXPECT_NE(xml.find("Sequence"), std::string::npos);
}

TEST(GazeTree, EstopBeatsTracking)
{
  EXPECT_EQ(mode_after(true, true), "hold");
  EXPECT_EQ(mode_after(false, true), "track");
  EXPECT_EQ(mode_after(false, false), "search");
}
