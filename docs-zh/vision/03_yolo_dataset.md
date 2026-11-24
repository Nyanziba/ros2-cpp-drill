# 3. YOLO 数据集

- 日语：https://nyanziba.github.io/ros2-cpp-drill/vision/03_yoloデータセット.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/vision/03_yolo_dataset.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/vision/03_yolo_dataset.html

平面标记之后，场上还有要靠形状区分的物体：球、球门、对方机器人、出发柱。训练脚本在有 GPU 的机器上跑。这一章评分的是那之前的约定：一行标签、train 和 val 的划分、导出 ONNX 的参数，以及类名。

## 一行标签

YOLO 的一行是 `class cx cy w h`，除以图像宽和高，数值在 0 到 1。小数保留 6 位。框超出图像、没有面积，或类别为负，都抛出 `ValueError`。右缘或下缘正好落在图像边上，算在里面。

| 项目 | 规则 |
| --- | --- |
| 类名 | ball, goal_marker, opponent, start_pole |
| 验证集 | (i+1) 能被 val_every 整除的行进入 val |

## 导出

`export_argv` 返回与 Ultralytics 命令行相同顺序的列表。它本身不训练。opset 小于 12 会被拒绝，空的权重路径也会被拒绝。

```python
yolo_line(class_id, x_min, y_min, x_max, y_max, image_w, image_h)
split_records(names, val_every)
export_argv(weights_path, opset)
robocon_class_names()
```

## CI 不评分的部分

GPU 训练在 pytest 里 skip。CI 不写出权重文件，只检查字符串和划分。

## 对应的练习

```bash
./drill run cv03
```
