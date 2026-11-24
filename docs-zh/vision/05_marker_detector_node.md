# 5. 标记检测节点

- 日语：https://nyanziba.github.io/ros2-cpp-drill/vision/05_マーカ検出ノード.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/vision/05_marker_detector_node.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/vision/05_marker_detector_node.html

检测不要停在一个函数上。把它做成订阅 `image`、发布 `marker` 的节点。比赛里的其他节点只看这一行字符串。太慢的检测是 `stale`，不是颜色，这样机器人就不会拿着旧画面去得分。

## 字符串

像素规则与 cv01 相同。`elapsed_ms > budget_ms` 时，即使看见颜色也是 `stale`。相等不算 stale。其余是 `red cx cy`、`blue cx cy` 或 `none`。

| 项目 | 规则 |
| --- | --- |
| 话题 | 订阅 image，发布 marker，KeepLast(10) 且 reliable |
| 参数 | min_pixels 默认 3，budget_ms 默认 40 |

## 和真实相机的差别

真实相机常常用 `SensorDataQoS`（best effort）。这个练习的测试不能丢掉那一帧，所以 QoS 固定为 reliable。记住这个差别，装到机器人上时再改回去。

```cpp
format_detection(bgr, min_pixels, elapsed_ms, budget_ms)
// "stale" / "red cx cy" / "blue cx cy" / "none"
```

## CI 不评分的部分

CI 不启动相机驱动。测试在同一进程里发布一块蓝色，并期望得到 `blue 2 5`。

## 对应的练习

```bash
./drill run cv05
```
