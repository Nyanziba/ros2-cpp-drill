# 4. TensorRT and the frame budget

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/vision/04_tensorrtとフレーム予算.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/04_tensorrt_and_frame_budget.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/04_tensorrt_and_frame_budget.html

On a Jetson, if detection does not fit in the camera period, the robot drives on an old picture. This chapter makes the precision choice and the share of a 30 fps frame testable on a CI machine with no GPU. Building the engine itself is skipped.

## Precision

INT8 is allowed only when there are at least 32 calibration images and the GPU has INT8. Otherwise FP16 if the GPU has it, otherwise FP32. The frame budget is `(1000 / fps) * (1 - margin)` milliseconds. A fps of 0 or less is not a budget.

| Item | Rule |
| --- | --- |
| 30 fps, margin 0.30 | the limit is about 23.33 ms. 23 fits, 24 does not |
| Calibration | greedy farthest L1 on 8-bin histograms |

## trtexec

A measured speed is the number after `mean = ` in a trtexec line such as `mean = 1.80 ms`. No number means failure. Speedup is baseline / candidate, or 0 when candidate is 0 or less.

```cpp
choose_precision(gpu_fp16, gpu_int8, calibration_images)
meets_frame_budget(latency_ms, camera_fps, margin)
pick_calibration_set(samples, count)
```

## What CI does not grade

The TensorRT engine build is `GTEST_SKIP` unless `DRILL_HAS_TENSORRT` is set. CI grades the precision choice, the budget, the calibration set, and parsing the mean.

## Matching exercise

```bash
./drill run cv04
```
