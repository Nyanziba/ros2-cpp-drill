# 6. 视线控制

- 日语：https://nyanziba.github.io/ros2-cpp-drill/vision/06_視線制御.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/vision/06_gaze_control.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/vision/06_gaze_control.html

图像里出现人或标记时，把相机或头部的平移和俯仰转向那边。这一章放在图像处理系列里，是因为输入是检测结果。行为树只负责切换模式，角度计算放在普通函数里。

## 看谁

rank 越小越优先。0 是比赛目标，1 是人，2 是其他。更小的 rank 会立刻抢占，即使当前目标正在丢失。同一 rank 只是分数更高，不会切换。丢失的目标一直保持到连续第 5 次看不到。

| 项目 | 规则 |
| --- | --- |
| 平移 | 把 kp * (cx - width/2) 夹在 ±max_rate 里 |
| 俯仰 | 图像的 y 向下，所以要把 kp * (cy - height/2) 的符号反过来 |

## 树的模式

黑板 `estop` 为真则是 `hold`，否则 `target_visible` 为真则是 `track`，都不是则是 `search`。XML 里要有 Fallback 和 Sequence。角度的积分和限位属于 `update_gaze`。

```cpp
GazeMemory next = update_gaze(previous, detections, camera);
// gaze_mode: hold, track, or search
```

## CI 不评分的部分

CI 不驱动舵机或云台。测试只用 100x100 的相机模型和黑板上的标志。

## 对应的练习

```bash
./drill run cv06
```
