// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>
#include <string>

#include <opencv2/core.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <std_msgs/msg/string.hpp>

namespace drill
{

// cv01 と同じ画素規則。min_pixels 未満は "none"。
// elapsed_ms > budget_ms なら、見えていても "stale"。
// それ以外は "red cx cy" または "blue cx cy"（整数、空白区切り）。
std::string format_detection(
  const cv::Mat & bgr, int min_pixels, double elapsed_ms, double budget_ms);

// トピック "image" を購読し、"marker" に format_detection の結果を出します。
// パラメータ min_pixels（既定 3）と budget_ms（既定 40.0）。
// QoS は KeepLast(10) の reliable です。実カメラは SensorDataQoS にしますが、
// この課題のテストは 1 枚を落とさず届けるため reliable に固定しています。
class MarkerDetector : public rclcpp::Node
{
public:
  explicit MarkerDetector(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  int min_pixels_{3};
  double budget_ms_{40.0};
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr subscription_;
};

}  // namespace drill
