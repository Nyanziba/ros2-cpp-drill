# 6. Gaze control

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/vision/06_視線制御.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/06_gaze_control.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/06_gaze_control.html

When a person or a marker shows up in the image, pan and tilt the camera or the head toward it. This chapter sits in the vision series because the input is a detection. A behavior tree only picks the mode. A plain function computes the angles.

## Whom to look at

A smaller rank wins. 0 is a game object, 1 is a person, 2 is anything else. A strictly better rank preempts immediately, even while the current target is flickering out. A higher score at the same rank does not switch. A lost target is held until the 5th consecutive miss.

| Item | Rule |
| --- | --- |
| Pan | clamp kp * (cx - width/2) to ±max_rate |
| Tilt | image y points down, so negate kp * (cy - height/2) |

## Tree mode

If blackboard `estop` is true, the mode is `hold`. Otherwise if `target_visible`, it is `track`. Otherwise it is `search`. The XML contains a Fallback and a Sequence. Integrating and clamping the angles belongs to `update_gaze`.

```cpp
GazeMemory next = update_gaze(previous, detections, camera);
// gaze_mode: hold, track, or search
```

## What CI does not grade

CI does not move a servo or a pan-tilt unit. The tests use a 100x100 camera model and the blackboard flags.

## Matching exercise

```bash
./drill run cv06
```
