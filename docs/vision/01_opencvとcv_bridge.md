# 1. OpenCV と cv_bridge

- 日本語: https://nyanziba.github.io/ros2-cpp-drill/vision/01_opencvとcv_bridge.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/01_opencv_and_cv_bridge.html
- 中文: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/01_opencv_and_cv_bridge.html

学生ロボコンのフィールドには、赤と青のマーカーが置かれることが多いです。最初の仕事は、カメラの BGR 画像からその塊を 1 つ見つけることです。ROS 2 の画像は `sensor_msgs/Image` なので、OpenCV の `cv::Mat` との往復も同じ章で扱います。

## 画素の規則

色は照明で揺れます。しきい値は「十分明るく、他のチャンネルより明らかに大きい」の両方にします。平均は整数の切り捨てです。OpenCV の `at<cv::Vec3b>` は B、G、R の順です。

| 項目 | 決まり |
| --- | --- |
| 赤 | R >= 180 かつ R > G+50 かつ R > B+50 |
| 青 | B >= 180 かつ B > R+50 かつ B > G+50 |

## cv_bridge

`encoding` が `bgr8` のときだけ Mat に戻します。他の encoding や、宣言より短い `data` は空の Mat です。`toCvCopy` の戻り値は共有ポインタなので、使い終わるまで持ってから `clone` します。

```cpp
MarkerHit find_marker(const cv::Mat & bgr, int min_pixels);
sensor_msgs::msg::Image mat_to_ros_image(const cv::Mat & bgr);
cv::Mat ros_image_to_mat(const sensor_msgs::msg::Image & image);
```

## CI でやらないこと

実カメラもレンズの歪み補正も採点しません。テストは合成した 8UC3 の画像と、手で組んだ `sensor_msgs/Image` だけを見ます。

## 対応する課題

```bash
./drill run cv01
```
