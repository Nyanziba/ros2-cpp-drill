# 行为树

- 日语：https://nyanziba.github.io/ros2-cpp-drill/behavior-tree/index.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/behavior-tree/index.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/behavior-tree/index.html

行为树是三章，用 BehaviorTree.CPP v4 写下急停、时间到、回避和得分的优先级。CI 不启动 ROS 2 动作服务器，而用一个返回同样状态号的客户端来评分。

## 章节

| 章 | 练习 | 内容 |
| --- | --- | --- |
| 1 | bt01 | 用 Fallback 表示优先级 |
| 2 | bt02 | StatefulAction 与动作状态 |
| 3 | bt03 | 先停再躲开 |

## 依赖包

图像需要 `cv_bridge` 和 OpenCV，行为树需要 `behaviortree_cpp`，ONNX 需要 `onnxruntime_vendor`。某个发行版没有 ONNX 包时，Docker 镜像会把官方 CPU 构建装到 /usr/local。默认镜像不含 Nav2 和 TensorRT。

```bash
sudo apt install ros-jazzy-cv-bridge ros-jazzy-behaviortree-cpp ros-jazzy-onnxruntime-vendor libopencv-dev
```

## 接着读

- [Priority](01_priority.md)
- [Avoidance](03_avoidance.md)
