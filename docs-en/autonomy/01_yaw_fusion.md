# 1. Yaw fusion

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/autonomy/01_ヨー角の融合.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/autonomy/01_yaw_fusion.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/autonomy/01_yaw_fusion.html

Wheel yaw jumps when the wheels slip. Gyro yaw drifts slowly. During a match, trust the gyro and mix in a little wheel yaw. Throw the wheel measurement away when the two disagree by a lot. Instead of launching the robot_localization EKF, CI grades the same idea as a formula and a 2D ekf.yaml.

## The formula

Wrap every angle into [-pi, pi] before subtracting. The prediction is `wrap(previous + gyro * dt)`. The fusion is `wrap(predicted + 0.02 * wrap(odom - predicted))`. The 0.02 is 1-0.98. Outside the gate, return the prediction unchanged.

| Item | Rule |
| --- | --- |
| odom0_config | only x, y, yaw, vx, and vyaw are true |
| imu0_config | only yaw, vyaw, and ax are true, and gravity is removed |

## The yaml

Include `frequency: 30.0`, `two_d_mode: true`, `world_frame: odom`, `odom0: /odom`, and `imu0: /imu`. The fifteen booleans of each config stay on one line, because the test searches for that substring.

```cpp
double fused = fuse_yaw_gated(previous, odom_yaw, gyro_rad_s, dt, gate_rad);
```

## What CI does not grade

Launching ekf_node is `GTEST_SKIP`, because it needs TF and real sensors. CI grades the formula and the yaml.

## Matching exercise

```bash
./drill run nav01
```
