#include "drill/field_route.hpp"

namespace drill
{

RoutePlan plan_field_route(const std::vector<Waypoint> & points, double match_time_left_sec)
{
  RoutePlan plan;
  bool negative = false;
  for (const auto & point : points) {
    if (point.leg_sec < 0.0) {
      negative = true;
    }
    plan.total_sec += point.leg_sec;
    plan.names.push_back(point.name);
  }
  plan.feasible = !points.empty() && !negative && plan.total_sec <= match_time_left_sec + 1e-9;
  return plan;
}

bool field_frames_connected(const std::vector<std::pair<std::string, std::string>> & edges)
{
  bool map_odom = false;
  bool odom_base = false;
  for (const auto & edge : edges) {
    if (edge.first == "map" && edge.second == "odom") {
      map_odom = true;
    }
    if (edge.first == "odom" && edge.second == "base_link") {
      odom_base = true;
    }
  }
  return map_odom && odom_base;
}

std::string slam_mode_name(bool have_map, bool keep_mapping)
{
  if (have_map && !keep_mapping) {
    return "localization";
  }
  return "mapping";
}

std::string nav_recovery_xml(bool with_spin)
{
  std::string xml =
    "<RecoveryNode number_of_retries=\"1\" name=\"FieldRecovery\">\n"
    "  <Sequence name=\"primary\">\n"
    "    <Wait wait_duration=\"0.5\"/>\n"
    "  </Sequence>\n"
    "  <Sequence name=\"recovery\">\n"
    "    <BackUp backup_dist=\"0.30\" backup_speed=\"0.15\"/>\n";
  if (with_spin) {
    xml += "    <Spin spin_dist=\"1.57\"/>\n";
  }
  xml +=
    "  </Sequence>\n"
    "</RecoveryNode>\n";
  return xml;
}

}  // namespace drill
