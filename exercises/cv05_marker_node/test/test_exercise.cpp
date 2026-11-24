// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <string>

#include <cv_bridge/cv_bridge.hpp>
#include <std_msgs/msg/string.hpp>

#include "drill/marker_detector.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;

namespace
{

cv::Mat block(const cv::Scalar & bgr)
{
  cv::Mat image(8, 8, CV_8UC3, cv::Scalar(20, 90, 20));
  for (int y = 4; y <= 6; ++y) {
    for (int x = 1; x <= 3; ++x) {
      image.at<cv::Vec3b>(y, x) = cv::Vec3b(bgr[0], bgr[1], bgr[2]);
    }
  }
  return image;
}

}  // namespace

TEST(MarkerNode, FormatsRedBlueNoneAndStale)
{
  EXPECT_EQ(drill::format_detection(block(cv::Scalar(0, 0, 255)), 3, 1.0, 40.0), "red 2 5");
  EXPECT_EQ(drill::format_detection(block(cv::Scalar(255, 0, 0)), 3, 40.0, 40.0), "blue 2 5")
    << drill::localized(
      "elapsed と budget が同じときは stale にしません。",
      "Do not mark stale when elapsed equals the budget.");
  EXPECT_EQ(
    drill::format_detection(block(cv::Scalar(0, 0, 255)), 3, 40.1, 40.0), "stale");
  cv::Mat green(8, 8, CV_8UC3, cv::Scalar(20, 90, 20));
  EXPECT_EQ(drill::format_detection(green, 3, 1.0, 40.0), "none");
}

TEST_F(DrillTest, PublishesBlueMarker)
{
  auto detector = std::make_shared<drill::MarkerDetector>();
  auto probe = std::make_shared<rclcpp::Node>(drill::unique_name("probe"));
  std::vector<std::string> received;
  auto subscription = probe->create_subscription<std_msgs::msg::String>(
    "marker", 10,
    [&received](std_msgs::msg::String::ConstSharedPtr message) {
      received.push_back(message->data);
    });
  auto publisher = probe->create_publisher<sensor_msgs::msg::Image>("image", 10);
  cv_bridge::CvImage image;
  image.encoding = "bgr8";
  image.image = block(cv::Scalar(255, 0, 0));
  const auto message = image.toImageMsg();

  ASSERT_TRUE(drill::spin_until(
    {detector, probe},
    [&received]() {return !received.empty();},
    std::chrono::seconds(8),
    [&publisher, &message]() {publisher->publish(*message);}))
    << drill::localized(
      "marker に 8 秒待っても届きません。image を購読して marker を出していますか？",
      "Nothing arrived on marker within 8 seconds. Do you subscribe to image and publish marker?");
  EXPECT_EQ(received.front(), "blue 2 5") << drill::localized(
    "実際の値: ", "Actual value: ") << received.front();
}
