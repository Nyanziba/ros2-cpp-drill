# cv02 ONNX Runtime 〔图像处理〕

用 CPU 上的 ONNX Runtime 返回随仓库模型的颜色类别。

## 要做的事

实现 `classify_marker_onnx`。把 B、G、R 除以 255 的平均值作为名为 `mean_bgr` 的 `[1,3]` 输入，返回 `logits` 的 argmax。类别是 0 无、1 红、2 蓝。并列取较小下标。空图像和 nullptr 返回 -1。模型是 `model/color_marker.onnx`。

## 运行

```bash
./drill run cv02
```

## 容易卡住的地方

用 RGB 顺序会把红蓝对调。在调用内部创建会话也可以。

## 测试

```bash
./drill run cv02
```

| 测试 | 检查的内容 |
| --- | --- |
| `RejectsEmpty` | 空和空指针 |
| `ClassifiesSolidColors` | 黑、红、蓝、绿 |
| `HalfRedIsStillRed` | 一半的红仍是红 |

## 参考

- [2. ONNX Runtime](../../docs-zh/vision/02_onnx_runtime.md)
