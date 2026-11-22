# nav01 ヨー角の融合 〔自律制御編〕

融合式と、robot_localization 用の 2D 設定を書きます。

## やること

`wrap_to_pi` は [-pi, pi]。`fuse_yaw` は predicted = wrap(previous + gyro*dt)、fused = wrap(predicted + 0.02*wrap(odom-predicted))。`odom_in_gate` は折った差の絶対値が gate 以下。`fuse_yaw_gated` は外なら predicted。`config/ekf.yaml` に次を含めます。frequency: 30.0、two_d_mode: true、publish_tf: true、world_frame: odom、odom_frame: odom、base_link_frame: base_link、odom0: /odom、imu0: /imu、imu0_remove_gravitational_acceleration: true。odom0_config は x,y,yaw,vx,vyaw だけ true。imu0_config は yaw, vyaw, ax だけ true。どちらも 15 個を 1 行で書きます。

## 動かしてみる

```bash
./drill run nav01
```

## つまずきポイント

±pi をまたぐ差を wrap しないと、3.1 と -3.1 のテストがずれます。

## テスト

```bash
./drill run nav01
```

| テスト | 見ているところ |
| --- | --- |
| `WrapsAndMixesALittleOdometry` | wrap と 0.02 |
| `RejectsWheelSlip` | ゲート |
| `EkfYamlIsTwoDimensional` | ekf.yaml の部分文字列 |
| `RobotLocalizationLaunchIsSkipped` | ノード起動は skip |

## 参考

- [1. ヨー角の融合](../../docs/autonomy/01_ヨー角の融合.md)
