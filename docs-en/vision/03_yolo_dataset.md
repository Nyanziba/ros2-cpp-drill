# 3. A YOLO dataset

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/vision/03_yoloデータセット.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/03_yolo_dataset.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/03_yolo_dataset.html

After flat markers, the field has objects you tell apart by shape: the ball, the goal, an opponent, and the start pole. The training script runs on a GPU machine. This chapter grades the contract in front of that: one label line, the train/val split, the ONNX export arguments, and the class names.

## One label line

One YOLO line is `class cx cy w h`, divided by the image width and height so the numbers sit in 0..1. Print six decimal places. A box outside the image, a box with no area, or a negative class raises `ValueError`. A far edge that lands exactly on the image edge is inside.

| Item | Rule |
| --- | --- |
| Class names | ball, goal_marker, opponent, start_pole |
| Validation rows | a row goes to val when (i+1) is divisible by val_every |

## Export

`export_argv` returns the argument list of the Ultralytics CLI. It does not train. An opset below 12 is rejected, and so is an empty weights path.

```python
yolo_line(class_id, x_min, y_min, x_max, y_max, image_w, image_h)
split_records(names, val_every)
export_argv(weights_path, opset)
robocon_class_names()
```

## What CI does not grade

GPU training is skipped in pytest. CI does not write a weights file. It only checks the strings and the split.

## Matching exercise

```bash
./drill run cv03
```
