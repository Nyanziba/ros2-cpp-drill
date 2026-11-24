# 自主控制

- 日语：https://nyanziba.github.io/ros2-cpp-drill/autonomy/index.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/autonomy/index.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/autonomy/index.html

自主控制是三章，对应三分钟的比赛：保持航向、把流程做成状态、选择能放进剩余时间的路径。CI 不启动 EKF 节点、SLAM 或 Nav2。

## 章节

| 章 | 练习 | 内容 |
| --- | --- | --- |
| 1 | nav01 | 陀螺与轮式航向 |
| 2 | nav02 | 何时用状态机或行为树 |
| 3 | nav03 | 剩余时间与恢复 XML |

## 依赖包

图像需要 `cv_bridge` 和 OpenCV，行为树需要 `behaviortree_cpp`，ONNX 需要 `onnxruntime_vendor`。某个发行版没有 ONNX 包时，Docker 镜像会把官方 CPU 构建装到 /usr/local。默认镜像不含 Nav2 和 TensorRT。

```bash
sudo apt install ros-jazzy-cv-bridge ros-jazzy-behaviortree-cpp ros-jazzy-onnxruntime-vendor libopencv-dev
```

## 接着读

- [Yaw fusion](01_yaw_fusion.md)
- [Routes and recovery](03_routes_and_recovery.md)
