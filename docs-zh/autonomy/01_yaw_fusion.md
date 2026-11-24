# 1. 航向角融合

- 日语：https://nyanziba.github.io/ros2-cpp-drill/autonomy/01_ヨー角の融合.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/autonomy/01_yaw_fusion.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/autonomy/01_yaw_fusion.html

轮式航向会在打滑时跳变，陀螺航向会慢慢漂移。比赛中的朝向以陀螺为主，只掺一点轮式航向。两者差得太多时丢掉轮式测量。CI 不启动 robot_localization 的 EKF，而是给同样思路的公式和一份二维 ekf.yaml 评分。

## 公式

相减之前先把角度折进 [-pi, pi]。预测是 `wrap(previous + gyro * dt)`，融合是 `wrap(predicted + 0.02 * wrap(odom - predicted))`。0.02 就是 1-0.98。门限之外直接返回预测。

| 项目 | 规则 |
| --- | --- |
| odom0_config | 只有 x, y, yaw, vx, vyaw 为 true |
| imu0_config | 只有 yaw, vyaw, ax 为 true，并且去掉重力 |

## yaml

要包含 `frequency: 30.0`、`two_d_mode: true`、`world_frame: odom`、`odom0: /odom`、`imu0: /imu`。每个 config 的 15 个布尔值写在一行里，因为测试按子串查找。

```cpp
double fused = fuse_yaw_gated(previous, odom_yaw, gyro_rad_s, dt, gate_rad);
```

## CI 不评分的部分

启动 ekf_node 是 `GTEST_SKIP`，因为它需要 TF 和真实传感器。评分的是公式和 yaml。

## 对应的练习

```bash
./drill run nav01
```
