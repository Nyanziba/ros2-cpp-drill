# 自律制御編

- 日本語: https://nyanziba.github.io/ros2-cpp-drill/autonomy/index.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/autonomy/index.html
- 中文: https://nyanziba.github.io/ros2-cpp-drill/zh/autonomy/index.html

自律制御編は、3 分の試合で「向きを保つ」「流れを状態にする」「残り時間に収まる経路を選ぶ」の3章です。EKF ノード、SLAM、Nav2 の起動は採点しません。

## 章

| 章 | 課題 | 内容 |
| --- | --- | --- |
| 1 | nav01 | ジャイロとホイールのヨー |
| 2 | nav02 | 状態機械と木の使い分け |
| 3 | nav03 | 残り時間と復帰 XML |

## 依存パッケージ

画像は `cv_bridge` と OpenCV、木は `behaviortree_cpp`、ONNX は `onnxruntime_vendor` です。ディストリビューションに ONNX のパッケージが無いときは、Docker イメージが公式の CPU ビルドを /usr/local に置きます。Nav2 と TensorRT は既定のイメージに入れません。

```bash
sudo apt install ros-jazzy-cv-bridge ros-jazzy-behaviortree-cpp ros-jazzy-onnxruntime-vendor libopencv-dev
```

## 次に読む

- [ヨー角の融合](01_ヨー角の融合.md)
- [経路と復帰](03_経路と復帰.md)
