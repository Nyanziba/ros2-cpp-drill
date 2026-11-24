# 1. OpenCV and cv_bridge

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/vision/01_opencvとcv_bridge.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/01_opencv_and_cv_bridge.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/01_opencv_and_cv_bridge.html

A student Robocon field often has red and blue markers. The first job is to find one blob in a BGR camera image. A ROS 2 image is a `sensor_msgs/Image`, so this chapter also converts it to and from a `cv::Mat`.

## Pixel rule

Color shifts with the lights. A pixel must be bright enough and clearly stronger than the other channels. The centroid uses integer truncation. OpenCV's `at<cv::Vec3b>` is B, then G, then R.

| Item | Rule |
| --- | --- |
| Red | R >= 180 and R > G+50 and R > B+50 |
| Blue | B >= 180 and B > R+50 and B > G+50 |

## cv_bridge

Convert back to a Mat only when `encoding` is `bgr8`. Any other encoding, or a `data` buffer shorter than the header claims, is an empty Mat. `toCvCopy` returns a shared pointer: keep it alive, then `clone`.

```cpp
MarkerHit find_marker(const cv::Mat & bgr, int min_pixels);
sensor_msgs::msg::Image mat_to_ros_image(const cv::Mat & bgr);
cv::Mat ros_image_to_mat(const sensor_msgs::msg::Image & image);
```

## What CI does not grade

CI does not open a camera and does not undistort a lens. The tests use a synthetic 8UC3 image and a hand-built `sensor_msgs/Image`.

## Matching exercise

```bash
./drill run cv01
```
