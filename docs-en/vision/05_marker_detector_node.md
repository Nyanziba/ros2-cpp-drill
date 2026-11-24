# 5. The marker detector node

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/vision/05_マーカ検出ノード.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/05_marker_detector_node.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/05_marker_detector_node.html

Detection should not stay a bare function. Make it a node that subscribes to `image` and publishes `marker`. The rest of the match reads that one string. A slow detection is `stale`, not a color, so the robot does not score from an old picture.

## The string

The pixel rule is the same as cv01. When `elapsed_ms > budget_ms`, the result is `stale` even if a color is visible. Equal is not stale. Otherwise the string is `red cx cy`, `blue cx cy`, or `none`.

| Item | Rule |
| --- | --- |
| Topics | subscribe image, publish marker, KeepLast(10) reliable |
| Parameters | min_pixels defaults to 3, budget_ms defaults to 40 |

## A real camera

A real camera often uses `SensorDataQoS` (best effort). This exercise's test must not drop the one frame, so QoS stays reliable. Remember the difference and switch it back on the robot.

```cpp
format_detection(bgr, min_pixels, elapsed_ms, budget_ms)
// "stale" / "red cx cy" / "blue cx cy" / "none"
```

## What CI does not grade

CI does not start a camera driver. The test publishes one blue blob in-process and expects `blue 2 5`.

## Matching exercise

```bash
./drill run cv05
```
