# nav01 航向角融合 〔自主控制〕

写下融合公式和 robot_localization 的二维配置。

## 要做的事

`wrap_to_pi` 折进 [-pi, pi]。`fuse_yaw` 是 predicted = wrap(previous + gyro*dt)，fused = wrap(predicted + 0.02*wrap(odom-predicted))。`odom_in_gate` 在折后差值的绝对值不超过 gate 时为真。`fuse_yaw_gated` 在门外返回 predicted。`config/ekf.yaml` 必须包含 frequency: 30.0、two_d_mode: true、publish_tf: true、world_frame: odom、odom_frame: odom、base_link_frame: base_link、odom0: /odom、imu0: /imu、imu0_remove_gravitational_acceleration: true。odom0_config 只有 x、y、yaw、vx、vyaw 为 true。imu0_config 只有 yaw、vyaw、ax 为 true。15 个布尔值写在一行。

## 运行

```bash
./drill run nav01
```

## 容易卡住的地方

不把跨越 ±pi 的差值 wrap 起来，3.1 和 -3.1 的测试会对不上。

## 测试

```bash
./drill run nav01
```

| 测试 | 检查的内容 |
| --- | --- |
| `WrapsAndMixesALittleOdometry` | wrap 与 0.02 |
| `RejectsWheelSlip` | 门限 |
| `EkfYamlIsTwoDimensional` | ekf.yaml 的子串 |
| `RobotLocalizationLaunchIsSkipped` | 启动节点被跳过 |

## 参考

- [1. 航向角融合](../../docs-zh/autonomy/01_yaw_fusion.md)
