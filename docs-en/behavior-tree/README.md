# Behavior trees

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/behavior-tree/index.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/behavior-tree/index.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/behavior-tree/index.html

The behavior-tree track is three chapters that write the priority of emergency stop, time limit, avoidance, and scoring in BehaviorTree.CPP v4. CI does not start a ROS 2 action server. A client that returns the same status codes stands in.

## Chapters

| Chapter | Exercise | What it covers |
| --- | --- | --- |
| 1 | bt01 | Priority with a Fallback |
| 2 | bt02 | StatefulAction and action status |
| 3 | bt03 | Stop before dodging |

## Packages

Images need `cv_bridge` and OpenCV, trees need `behaviortree_cpp`, and ONNX needs `onnxruntime_vendor`. When a distro has no ONNX package, the Docker image installs the official CPU build under /usr/local. Nav2 and TensorRT are not in the default image.

```bash
sudo apt install ros-jazzy-cv-bridge ros-jazzy-behaviortree-cpp ros-jazzy-onnxruntime-vendor libopencv-dev
```

## Read next

- [Priority](01_priority.md)
- [Avoidance](03_avoidance.md)
