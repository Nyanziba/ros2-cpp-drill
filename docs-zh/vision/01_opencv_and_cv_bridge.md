# 1. OpenCV 与 cv_bridge

- 日语：https://nyanziba.github.io/ros2-cpp-drill/vision/01_opencvとcv_bridge.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/vision/01_opencv_and_cv_bridge.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/vision/01_opencv_and_cv_bridge.html

学生机器人竞赛的场地经常放置红色和蓝色标记。第一步是在相机的 BGR 图像里找出一块。ROS 2 的图像是 `sensor_msgs/Image`，所以这一章同时处理它和 `cv::Mat` 的来回转换。

## 像素规则

颜色会随灯光变化。阈值要同时要求「足够亮」和「明显大于其他通道」。中心用整数截断。OpenCV 的 `at<cv::Vec3b>` 顺序是 B、G、R。

| 项目 | 规则 |
| --- | --- |
| 红 | R >= 180 且 R > G+50 且 R > B+50 |
| 蓝 | B >= 180 且 B > R+50 且 B > G+50 |

## cv_bridge

只有 `encoding` 为 `bgr8` 时才转回 Mat。其他编码，或 `data` 短于头部声明的长度，都返回空 Mat。`toCvCopy` 返回共享指针，用完之前要持有它，再 `clone`。

```cpp
MarkerHit find_marker(const cv::Mat & bgr, int min_pixels);
sensor_msgs::msg::Image mat_to_ros_image(const cv::Mat & bgr);
cv::Mat ros_image_to_mat(const sensor_msgs::msg::Image & image);
```

## CI 不评分的部分

CI 不打开真实相机，也不做镜头畸变校正。测试只用合成的 8UC3 图像和手工填好的 `sensor_msgs/Image`。

## 对应的练习

```bash
./drill run cv01
```
