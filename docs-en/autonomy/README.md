# Autonomy

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/autonomy/index.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/autonomy/index.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/autonomy/index.html

The autonomy track is three chapters for a three-minute match: hold your heading, make the flow a state machine, and pick a route that fits the time left. CI does not launch the EKF node, SLAM, or Nav2.

## Chapters

| Chapter | Exercise | What it covers |
| --- | --- | --- |
| 1 | nav01 | Gyro and wheel yaw |
| 2 | nav02 | When to use a state machine or a tree |
| 3 | nav03 | Time left and the recovery XML |

## Packages

Images need `cv_bridge` and OpenCV, trees need `behaviortree_cpp`, and ONNX needs `onnxruntime_vendor`. When a distro has no ONNX package, the Docker image installs the official CPU build under /usr/local. Nav2 and TensorRT are not in the default image.

```bash
sudo apt install ros-jazzy-cv-bridge ros-jazzy-behaviortree-cpp ros-jazzy-onnxruntime-vendor libopencv-dev
```

## Read next

- [Yaw fusion](01_yaw_fusion.md)
- [Routes and recovery](03_routes_and_recovery.md)
