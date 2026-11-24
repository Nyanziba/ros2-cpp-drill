// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <opencv2/core.hpp>
#include <sensor_msgs/msg/image.hpp>

namespace drill
{

// klass: 0 = なし, 1 = 赤, 2 = 青。
// 赤と青のどちらでもない画素は無視します。
struct MarkerHit
{
  int klass{0};
  int cx{0};
  int cy{0};
  int pixels{0};
};

// BGR 画像からフィールド上の色マーカーを 1 つ返します。
// 赤画素: BGR の R >= 180 かつ R > G+50 かつ R > B+50。
// 青画素: B >= 180 かつ B > R+50 かつ B > G+50。
// 数が多い色を採用します。同数なら赤です。
// 採用した画素数が min_pixels 未満、空画像、CV_8UC3 以外なら klass は 0 です。
// cx, cy は採用した画素の列・行の整数平均（切り捨て）です。
MarkerHit find_marker(const cv::Mat & bgr, int min_pixels);

// encoding は "bgr8"。step は width * 3、is_bigendian は 0。
sensor_msgs::msg::Image mat_to_ros_image(const cv::Mat & bgr);

// encoding が "bgr8" でない、またはデータが足りなければ空の Mat を返します。
cv::Mat ros_image_to_mat(const sensor_msgs::msg::Image & image);

}  // namespace drill
