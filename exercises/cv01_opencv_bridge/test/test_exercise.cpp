// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <cstdint>

#include "drill/marker_detect.hpp"
#include "drill_i18n.hpp"

namespace
{

cv::Mat field(int rows, int cols)
{
  return cv::Mat(rows, cols, CV_8UC3, cv::Scalar(20, 90, 20));
}

void paint(cv::Mat & image, int x0, int y0, int x1, int y1, const cv::Scalar & bgr)
{
  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      image.at<cv::Vec3b>(y, x) = cv::Vec3b(bgr[0], bgr[1], bgr[2]);
    }
  }
}

}  // namespace

TEST(MarkerDetect, FindsRedBlockCentroid)
{
  cv::Mat image = field(8, 8);
  paint(image, 1, 4, 3, 6, cv::Scalar(0, 0, 255));
  const drill::MarkerHit hit = drill::find_marker(image, 3);
  EXPECT_EQ(hit.klass, 1) << drill::localized(
    "3x3 の赤は klass 1 です。", "A 3x3 red block is klass 1.");
  EXPECT_EQ(hit.pixels, 9);
  EXPECT_EQ(hit.cx, 2) << drill::localized(
    "列 1,2,3 の整数平均は 2 です。", "The integer mean of columns 1,2,3 is 2.");
  EXPECT_EQ(hit.cy, 5);
}

TEST(MarkerDetect, FindsBlueBlock)
{
  cv::Mat image = field(8, 8);
  paint(image, 1, 4, 3, 6, cv::Scalar(255, 0, 0));
  const drill::MarkerHit hit = drill::find_marker(image, 3);
  EXPECT_EQ(hit.klass, 2);
  EXPECT_EQ(hit.cx, 2);
  EXPECT_EQ(hit.cy, 5);
}

TEST(MarkerDetect, TiePrefersRed)
{
  cv::Mat image = field(8, 8);
  paint(image, 0, 0, 1, 1, cv::Scalar(0, 0, 255));
  paint(image, 4, 4, 5, 5, cv::Scalar(255, 0, 0));
  const drill::MarkerHit hit = drill::find_marker(image, 1);
  EXPECT_EQ(hit.klass, 1) << drill::localized(
    "同数のときは赤を採用します。", "A tie selects red.");
  EXPECT_EQ(hit.pixels, 4);
}

TEST(MarkerDetect, RejectsSmallBlobAndWrongType)
{
  cv::Mat image = field(8, 8);
  paint(image, 1, 4, 3, 6, cv::Scalar(0, 0, 255));
  EXPECT_EQ(drill::find_marker(image, 10).klass, 0);
  cv::Mat gray(4, 4, CV_8UC1, cv::Scalar(255));
  EXPECT_EQ(drill::find_marker(gray, 1).klass, 0);
  EXPECT_EQ(drill::find_marker(cv::Mat(), 1).klass, 0);
}

TEST(MarkerDetect, RoundTripsBgr8)
{
  cv::Mat image = field(2, 3);
  image.at<cv::Vec3b>(1, 2) = cv::Vec3b(1, 2, 3);
  const sensor_msgs::msg::Image message = drill::mat_to_ros_image(image);
  EXPECT_EQ(message.encoding, "bgr8");
  EXPECT_EQ(message.height, 2u);
  EXPECT_EQ(message.width, 3u);
  EXPECT_EQ(message.step, 9u);
  EXPECT_EQ(message.is_bigendian, 0);
  const cv::Mat back = drill::ros_image_to_mat(message);
  ASSERT_FALSE(back.empty());
  EXPECT_EQ(back.at<cv::Vec3b>(1, 2), cv::Vec3b(1, 2, 3));
}

TEST(MarkerDetect, RejectsOtherEncodingsAndShortData)
{
  sensor_msgs::msg::Image message = drill::mat_to_ros_image(field(2, 2));
  message.encoding = "rgb8";
  EXPECT_TRUE(drill::ros_image_to_mat(message).empty()) << drill::localized(
    "rgb8 を bgr8 と取り違えないでください。", "Do not treat rgb8 as bgr8.");
  message.encoding = "bgr8";
  message.data.clear();
  EXPECT_TRUE(drill::ros_image_to_mat(message).empty());
}
