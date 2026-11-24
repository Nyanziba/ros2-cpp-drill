// I AM NOT DONE
//
// 残り時間に収まらない経路は feasible を false にしてください。名前は残します。

#include "drill/field_route.hpp"

namespace drill
{

RoutePlan plan_field_route(const std::vector<Waypoint> & points, double match_time_left_sec)
{
  (void)points;
  (void)match_time_left_sec;
  return {};
}

bool field_frames_connected(const std::vector<std::pair<std::string, std::string>> & edges)
{
  (void)edges;
  return false;
}

std::string slam_mode_name(bool have_map, bool keep_mapping)
{
  (void)have_map;
  (void)keep_mapping;
  return {};
}

std::string nav_recovery_xml(bool with_spin)
{
  (void)with_spin;
  return {};
}

}  // namespace drill
