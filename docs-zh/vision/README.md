# 图像处理

- 日语：https://nyanziba.github.io/ros2-cpp-drill/vision/index.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/vision/index.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/vision/index.html

图像处理是六章，讲的是如何在 Jetson 上、在比赛周期内处理学生机器人竞赛场地的颜色和形状。视线控制也放在这里，因为它的输入是图像检测。

## 章节

| 章 | 练习 | 内容 |
| --- | --- | --- |
| 1 | cv01 | BGR 标记与 cv_bridge |
| 2 | cv02 | 用随仓库的 ONNX 做颜色分类 |
| 3 | cv03 | YOLO 标签与划分 |
| 4 | cv04 | 精度与帧预算 |
| 5 | cv05 | 发布检测的节点 |
| 6 | cv06 | 平移与俯仰的视线 |

## 依赖包

图像需要 `cv_bridge` 和 OpenCV，行为树需要 `behaviortree_cpp`，ONNX 需要 `onnxruntime_vendor`。某个发行版没有 ONNX 包时，Docker 镜像会把官方 CPU 构建装到 /usr/local。默认镜像不含 Nav2 和 TensorRT。

```bash
sudo apt install ros-jazzy-cv-bridge ros-jazzy-behaviortree-cpp ros-jazzy-onnxruntime-vendor libopencv-dev
```

## 接着读

- [OpenCV and cv_bridge](01_opencv_and_cv_bridge.md)
- [Gaze control](06_gaze_control.md)
