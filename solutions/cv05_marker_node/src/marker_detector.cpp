#include "drill/marker_detector.hpp"

#include <chrono>
#include <string>

#include <cv_bridge/cv_bridge.hpp>

namespace drill
{
namespace
{

bool is_red(const cv::Vec3b & pixel)
{
  return pixel[2] >= 180 && pixel[2] > pixel[1] + 50 && pixel[2] > pixel[0] + 50;
}

bool is_blue(const cv::Vec3b & pixel)
{
  return pixel[0] >= 180 && pixel[0] > pixel[2] + 50 && pixel[0] > pixel[1] + 50;
}

}  // namespace

std::string format_detection(
  const cv::Mat & bgr, int min_pixels, double elapsed_ms, double budget_ms)
{
  if (elapsed_ms > budget_ms) {
    return "stale";
  }
  if (bgr.empty() || bgr.type() != CV_8UC3) {
    return "none";
  }

  int red_pixels = 0;
  int blue_pixels = 0;
  int red_x = 0;
  int red_y = 0;
  int blue_x = 0;
  int blue_y = 0;
  for (int y = 0; y < bgr.rows; ++y) {
    for (int x = 0; x < bgr.cols; ++x) {
      const cv::Vec3b pixel = bgr.at<cv::Vec3b>(y, x);
      if (is_red(pixel)) {
        ++red_pixels;
        red_x += x;
        red_y += y;
      } else if (is_blue(pixel)) {
        ++blue_pixels;
        blue_x += x;
        blue_y += y;
      }
    }
  }

  const bool red_wins = red_pixels >= blue_pixels;
  const int pixels = red_wins ? red_pixels : blue_pixels;
  if (pixels < min_pixels || pixels == 0) {
    return "none";
  }
  const int cx = (red_wins ? red_x : blue_x) / pixels;
  const int cy = (red_wins ? red_y : blue_y) / pixels;
  return std::string(red_wins ? "red " : "blue ") + std::to_string(cx) + " " + std::to_string(cy);
}

MarkerDetector::MarkerDetector(const rclcpp::NodeOptions & options)
: rclcpp::Node("marker_detector", options)
{
  min_pixels_ = declare_parameter<int>("min_pixels", 3);
  budget_ms_ = declare_parameter<double>("budget_ms", 40.0);
  publisher_ = create_publisher<std_msgs::msg::String>("marker", 10);
  subscription_ = create_subscription<sensor_msgs::msg::Image>(
    "image", 10,
    [this](sensor_msgs::msg::Image::ConstSharedPtr message) {
      const auto started = std::chrono::steady_clock::now();
      cv::Mat bgr;
      try {
        const cv_bridge::CvImagePtr converted = cv_bridge::toCvCopy(*message, "bgr8");
        bgr = converted->image;
      } catch (const cv_bridge::Exception &) {
        return;
      }
      const double elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started).count();
      std_msgs::msg::String out;
      out.data = format_detection(bgr, min_pixels_, elapsed, budget_ms_);
      publisher_->publish(out);
    });
}

}  // namespace drill
