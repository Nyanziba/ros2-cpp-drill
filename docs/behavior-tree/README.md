# ビヘイビアツリー編

- 日本語: https://nyanziba.github.io/ros2-cpp-drill/behavior-tree/index.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/behavior-tree/index.html
- 中文: https://nyanziba.github.io/ros2-cpp-drill/zh/behavior-tree/index.html

ビヘイビアツリー編は、非常停止、制限時間、回避、得点の優先順位を BehaviorTree.CPP v4 で書く3章です。ROS 2 アクションのサーバは立てず、同じ状態番号のクライアントで採点します。

## 章

| 章 | 課題 | 内容 |
| --- | --- | --- |
| 1 | bt01 | Fallback で優先順位 |
| 2 | bt02 | StatefulAction とアクションの状態 |
| 3 | bt03 | 止まってから避ける |

## 依存パッケージ

画像は `cv_bridge` と OpenCV、木は `behaviortree_cpp`、ONNX は `onnxruntime_vendor` です。ディストリビューションに ONNX のパッケージが無いときは、Docker イメージが公式の CPU ビルドを /usr/local に置きます。Nav2 と TensorRT は既定のイメージに入れません。

```bash
sudo apt install ros-jazzy-cv-bridge ros-jazzy-behaviortree-cpp ros-jazzy-onnxruntime-vendor libopencv-dev
```

## 次に読む

- [優先順位](01_優先順位.md)
- [回避](03_回避.md)
