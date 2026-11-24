# cv03 YOLO 数据集 〔图像处理〕

实现一行标签、划分、导出 ONNX 的参数和类名。

## 要做的事

`src/yolo_dataset.py` 里的四个函数。`yolo_line` 是 `class cx cy w h`（6 位小数）。超出图像、没有面积或 class_id < 0 抛出 ValueError。正好落在边上是允许的。`split_records` 只把 (i+1) 能被 val_every 整除的行放进 val。val_every <= 1 抛出 ValueError。`export_argv` 是 `yolo export model=... format=onnx opset=... simplify=True`。空路径和 opset < 12 要拒绝。`robocon_class_names` 是 ball、goal_marker、opponent、start_pole。

## 运行

```bash
./drill run cv03
```

## 容易卡住的地方

在除以图像尺寸之前就返回像素中心，测试不会通过。不必写训练函数。

## 测试

```bash
./drill run cv03
```

| 测试 | 检查的内容 |
| --- | --- |
| `test_yolo_line_normalizes_box` | 归一化与 6 位小数 |
| `test_yolo_line_allows_far_edge_and_rejects_outside` | 边缘与外侧 |
| `test_split_is_deterministic` | train 与 val |
| `test_export_argv_pins_onnx` | 导出参数 |
| `test_class_names_match_the_practice_field` | 四个类名 |
| `test_gpu_training_is_not_run_in_ci` | GPU 训练被跳过 |

## 参考

- [3. YOLO 数据集](../../docs-zh/vision/03_yolo_dataset.md)
