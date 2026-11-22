# 4. TensorRT 与帧预算

- 日语：https://nyanziba.github.io/ros2-cpp-drill/vision/04_tensorrtとフレーム予算.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/vision/04_tensorrt_and_frame_budget.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/vision/04_tensorrt_and_frame_budget.html

在 Jetson 上，如果检测塞不进相机周期，机器人就会看着过期的画面行动。这一章让精度选择，以及 30 fps 里可以分给检测多少时间，在没有 GPU 的 CI 上也能判断。引擎构建本身会跳过。

## 精度

只有校准图像至少 32 张，并且 GPU 支持 INT8 时才选 INT8。否则在 GPU 支持时用 FP16，再不然用 FP32。帧预算是 `(1000 / fps) * (1 - margin)` 毫秒。fps 小于等于 0 则不成立。

| 项目 | 规则 |
| --- | --- |
| 30 fps，余量 0.30 | 上限约 23.33 ms。23 可以，24 不行 |
| 校准集 | 在 8 格直方图上贪心选取 L1 距离最远的样本 |

## trtexec

实测速度从 trtexec 里 `mean = 1.80 ms` 这种行的 `mean = ` 后面读取。没有数字就是失败。加速比是 baseline / candidate，candidate 小于等于 0 时为 0。

```cpp
choose_precision(gpu_fp16, gpu_int8, calibration_images)
meets_frame_budget(latency_ms, camera_fps, margin)
pick_calibration_set(samples, count)
```

## CI 不评分的部分

没有 `DRILL_HAS_TENSORRT` 时，TensorRT 引擎构建是 `GTEST_SKIP`。评分的是精度选择、预算、校准集合，以及 mean 的读取。

## 对应的练习

```bash
./drill run cv04
```
