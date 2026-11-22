// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace drill
{

struct Waypoint
{
  std::string name;
  double x{0.0};
  double y{0.0};
  double leg_sec{0.0};
};

struct RoutePlan
{
  bool feasible{false};
  double total_sec{0.0};
  std::vector<std::string> names;
};

// leg_sec の合計が match_time_left_sec 以下で、地点が 1 つ以上なら feasible。
// 負の leg_sec があれば feasible は false。合計と名前は、不成立でも埋めます。
RoutePlan plan_field_route(const std::vector<Waypoint> & points, double match_time_left_sec);

// map -> odom と odom -> base_link が両方、直接の辺としてあること。
bool field_frames_connected(const std::vector<std::pair<std::string, std::string>> & edges);

// 地図が既にあり、追記しないなら "localization"。それ以外は "mapping"。
std::string slam_mode_name(bool have_map, bool keep_mapping);

// 次の文字列と完全一致（末尾改行を含む）。with_spin のときだけ Spin の行を入れる。
// <RecoveryNode number_of_retries="1" name="FieldRecovery">
//   <Sequence name="primary">
//     <Wait wait_duration="0.5"/>
//   </Sequence>
//   <Sequence name="recovery">
//     <BackUp backup_dist="0.30" backup_speed="0.15"/>
//     <Spin spin_dist="1.57"/>   ← with_spin のときだけ、この行
//   </Sequence>
// </RecoveryNode>
std::string nav_recovery_xml(bool with_spin);

}  // namespace drill
