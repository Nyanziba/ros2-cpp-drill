// I AM NOT DONE
//
// 競技フィールドの赤・青マーカーを BGR で数えて重心を出し、
// cv_bridge で sensor_msgs/Image と往復してください。

#include "drill/marker_detect.hpp"

namespace drill
{

MarkerHit find_marker(const cv::Mat & bgr, int min_pixels)
{
  (void)bgr;
  (void)min_pixels;
  // TODO: 赤と青を数える。多い方の整数平均を返す。同数は赤。
  return {};
}

sensor_msgs::msg::Image mat_to_ros_image(const cv::Mat & bgr)
{
  (void)bgr;
  // TODO: cv_bridge で encoding "bgr8" の Image にする。
  return sensor_msgs::msg::Image();
}

cv::Mat ros_image_to_mat(const sensor_msgs::msg::Image & image)
{
  (void)image;
  // TODO: "bgr8" 以外と壊れたデータは空 Mat。それ以外は cv_bridge で戻す。
  return {};
}

}  // namespace drill
