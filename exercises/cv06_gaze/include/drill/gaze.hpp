// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <string>
#include <vector>

#include "behaviortree_cpp/bt_factory.h"

namespace drill
{

// rank は小さいほど優先。0 = 競技対象、1 = 人、2 = その他。
struct GazeDetection
{
  int id{0};
  int rank{0};
  double score{0.0};
  double cx{0.0};
  double cy{0.0};
  bool visible{false};
};

struct GazeMemory
{
  int target_id{-1};
  int target_rank{0};
  int lost_frames{0};
  double pan_deg{0.0};
  double tilt_deg{0.0};
};

// 画像の y は下向き。チルト正は上なので、誤差 y の符号を反転します。
struct CameraModel
{
  int width{0};
  int height{0};
  double kp{0.0};
  double max_rate{0.0};
  double pan_min{0.0};
  double pan_max{0.0};
  double tilt_min{0.0};
  double tilt_max{0.0};
  double dt{0.0};
};

// 見失ってからこの回数に達する更新でロックを外します（5 回目の欠測で解除）。
constexpr int kGazeLostLimit = 5;

// 見えているロックは維持します。より小さい rank が現れたときだけ即座に乗り換えます。
// 同じ rank の高スコアには乗り換えません。
// ロック対象がいない更新が続くあいだ角度は保持し、lost_frames を増やします。
// 速度は kp * 画素誤差を ±max_rate で挟み、角度は pan/tilt の範囲で挟みます。
GazeMemory update_gaze(
  const GazeMemory & previous,
  const std::vector<GazeDetection> & detections,
  const CameraModel & camera);

// 黒板 estop / target_visible を見て gaze_mode を hold / track / search にする木。
// estop が最優先、次に target_visible、どちらも無ければ search。
// XML には Fallback と Sequence を含めます。
std::string gaze_xml();
void register_gaze_nodes(BT::BehaviorTreeFactory & factory);

}  // namespace drill
