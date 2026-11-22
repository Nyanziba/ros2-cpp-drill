# Vision

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/vision/index.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/index.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/index.html

The vision track is six chapters on handling the colors and shapes of a student Robocon field on a Jetson, inside the match period. Gaze control is here because its input is an image detection.

## Chapters

| Chapter | Exercise | What it covers |
| --- | --- | --- |
| 1 | cv01 | BGR markers and cv_bridge |
| 2 | cv02 | Color classes from the committed ONNX |
| 3 | cv03 | YOLO labels and the split |
| 4 | cv04 | Precision and the frame budget |
| 5 | cv05 | A node that publishes the detection |
| 6 | cv06 | Pan and tilt gaze |

## Packages

Images need `cv_bridge` and OpenCV, trees need `behaviortree_cpp`, and ONNX needs `onnxruntime_vendor`. When a distro has no ONNX package, the Docker image installs the official CPU build under /usr/local. Nav2 and TensorRT are not in the default image.

```bash
sudo apt install ros-jazzy-cv-bridge ros-jazzy-behaviortree-cpp ros-jazzy-onnxruntime-vendor libopencv-dev
```

## Read next

- [OpenCV and cv_bridge](01_opencv_and_cv_bridge.md)
- [Gaze control](06_gaze_control.md)
