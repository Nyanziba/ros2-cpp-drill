# 画像処理編

- 日本語: https://nyanziba.github.io/ros2-cpp-drill/vision/index.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/index.html
- 中文: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/index.html

画像処理編は、学生ロボコンのフィールドにある色と形を、Jetson の上で試合の周期に収めて扱うための6章です。視線制御も、入力が画像の検出なのでここに置いています。

## 章

| 章 | 課題 | 内容 |
| --- | --- | --- |
| 1 | cv01 | BGR のマーカーと cv_bridge |
| 2 | cv02 | 同梱 ONNX の色分類 |
| 3 | cv03 | YOLO のラベルと分割 |
| 4 | cv04 | 精度とフレーム予算 |
| 5 | cv05 | 検出を出すノード |
| 6 | cv06 | パン・チルトの視線 |

## 依存パッケージ

画像は `cv_bridge` と OpenCV、木は `behaviortree_cpp`、ONNX は `onnxruntime_vendor` です。ディストリビューションに ONNX のパッケージが無いときは、Docker イメージが公式の CPU ビルドを /usr/local に置きます。Nav2 と TensorRT は既定のイメージに入れません。

```bash
sudo apt install ros-jazzy-cv-bridge ros-jazzy-behaviortree-cpp ros-jazzy-onnxruntime-vendor libopencv-dev
```

## 次に読む

- [OpenCV と cv_bridge](01_opencvとcv_bridge.md)
- [視線制御](06_視線制御.md)
