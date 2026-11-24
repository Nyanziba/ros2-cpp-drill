// I AM NOT DONE
//
// 画像を受けて "red cx cy" / "blue cx cy" / "none" / "stale" を出すノードです。

#include "drill/marker_detector.hpp"

namespace drill
{

std::string format_detection(
  const cv::Mat & bgr, int min_pixels, double elapsed_ms, double budget_ms)
{
  (void)bgr;
  (void)min_pixels;
  (void)elapsed_ms;
  (void)budget_ms;
  // TODO: 予算超過は stale。そうでなければ cv01 と同じ規則で red/blue/none。
  return "none";
}

MarkerDetector::MarkerDetector(const rclcpp::NodeOptions & options)
: rclcpp::Node("marker_detector", options)
{
  (void)options;
  // TODO: パラメータ、publisher("marker")、subscription("image")。
}

}  // namespace drill
