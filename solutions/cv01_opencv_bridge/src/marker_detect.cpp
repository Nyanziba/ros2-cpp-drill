#include "drill/marker_detect.hpp"

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

MarkerHit find_marker(const cv::Mat & bgr, int min_pixels)
{
  MarkerHit hit;
  if (bgr.empty() || bgr.type() != CV_8UC3) {
    return hit;
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
    return hit;
  }
  hit.klass = red_wins ? 1 : 2;
  hit.pixels = pixels;
  hit.cx = (red_wins ? red_x : blue_x) / pixels;
  hit.cy = (red_wins ? red_y : blue_y) / pixels;
  return hit;
}

sensor_msgs::msg::Image mat_to_ros_image(const cv::Mat & bgr)
{
  cv_bridge::CvImage image;
  image.encoding = "bgr8";
  image.image = bgr;
  return *image.toImageMsg();
}

cv::Mat ros_image_to_mat(const sensor_msgs::msg::Image & image)
{
  if (image.encoding != "bgr8") {
    return {};
  }
  try {
    const cv_bridge::CvImagePtr converted = cv_bridge::toCvCopy(image, "bgr8");
    return converted->image.clone();
  } catch (const cv_bridge::Exception &) {
    return {};
  }
}

}  // namespace drill
