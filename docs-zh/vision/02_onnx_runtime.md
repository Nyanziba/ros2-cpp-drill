# 2. ONNX Runtime

- 日语：https://nyanziba.github.io/ros2-cpp-drill/vision/02_onnx_runtime.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/vision/02_onnx_runtime.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/vision/02_onnx_runtime.html

颜色阈值之后，同样的三类（红、蓝、都不是）可以交给一个小网络。训练在另一台机器上完成，机器人只携带 ONNX。这一章的模型是仓库里提交的一层 Gemm。不需要 GPU。

## 输入和输出

输入不是整张图，而是把 B、G、R 归一化到 0 到 1 的三个平均值。三个输出里最大值的下标就是类别。并列时取较小的下标。空图像和空指针返回 -1。

| 项目 | 规则 |
| --- | --- |
| 输入 mean_bgr | float [1, 3]，顺序是 B, G, R |
| 输出 logits | [1, 3]。0 无，1 红，2 蓝 |

## 会话

把模型路径交给 `Ort::Session`。模型里的名字是 `mean_bgr` 和 `logits`。张量形状不对会在运行时失败。

```cpp
int klass = classify_marker_onnx(model_path, pixels, width, height);
// klass: -1 empty, 0 none, 1 red, 2 blue
```

## CI 不评分的部分

CI 不训练，也不做量化。它只用随仓库提交的模型，在 CPU 上的 ONNX Runtime（`onnxruntime_vendor`）里分类。

## 对应的练习

```bash
./drill run cv02
```
