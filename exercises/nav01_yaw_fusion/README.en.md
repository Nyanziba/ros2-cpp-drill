# nav01 Yaw fusion [Autonomy]

Write the fusion formula and a 2D robot_localization config.

## What to do

`wrap_to_pi` folds into [-pi, pi]. `fuse_yaw` is predicted = wrap(previous + gyro*dt), fused = wrap(predicted + 0.02*wrap(odom-predicted)). `odom_in_gate` is true when the absolute wrapped difference is within the gate. `fuse_yaw_gated` returns predicted when outside. `config/ekf.yaml` must contain frequency: 30.0, two_d_mode: true, publish_tf: true, world_frame: odom, odom_frame: odom, base_link_frame: base_link, odom0: /odom, imu0: /imu, and imu0_remove_gravitational_acceleration: true. odom0_config is true only for x, y, yaw, vx, and vyaw. imu0_config is true only for yaw, vyaw, and ax. Write all fifteen booleans on one line.

## Run it

```bash
./drill run nav01
```

## Common pitfalls

If you do not wrap a difference that crosses ±pi, the 3.1 and -3.1 test misses.

## Tests

```bash
./drill run nav01
```

| Test | What it checks |
| --- | --- |
| `WrapsAndMixesALittleOdometry` | wrap and 0.02 |
| `RejectsWheelSlip` | the gate |
| `EkfYamlIsTwoDimensional` | substrings of ekf.yaml |
| `RobotLocalizationLaunchIsSkipped` | launching the node is skipped |

## References

- [1. Yaw fusion](../../docs-en/autonomy/01_yaw_fusion.md)
