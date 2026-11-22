# 4. TensorRT とフレーム予算

- 日本語: https://nyanziba.github.io/ros2-cpp-drill/vision/04_tensorrtとフレーム予算.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/04_tensorrt_and_frame_budget.html
- 中文: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/04_tensorrt_and_frame_budget.html

Jetson では、検出がカメラの周期に収まらないと、ロボットは古い絵を見て動きます。精度の選び方と、30 fps のうちいくらを検出に使ってよいかを、GPU が無い CI でも判断できるようにします。エンジンのビルド自体はスキップします。

## 精度

INT8 はキャリブレーション画像が 32 枚以上あり、GPU が INT8 を持っているときだけです。足りなければ FP16、それも無ければ FP32 です。フレーム予算は `(1000 / fps) * (1 - margin)` ミリ秒以下です。fps が 0 以下なら不成立です。

| 項目 | 決まり |
| --- | --- |
| 30 fps、余裕 0.30 | 上限は約 23.33 ms。23 は可、24 は不可 |
| キャリブレーション | 8 ビンのヒストグラムで、L1 が最も遠いものを貪欲に選ぶ |

## trtexec

速度の実測は `trtexec` の `mean = 1.80 ms` のような行から平均を読みます。数が無ければ失敗です。speedup は baseline / candidate で、candidate が 0 以下なら 0 です。

```cpp
choose_precision(gpu_fp16, gpu_int8, calibration_images)
meets_frame_budget(latency_ms, camera_fps, margin)
pick_calibration_set(samples, count)
```

## CI でやらないこと

TensorRT のエンジンビルドは `DRILL_HAS_TENSORRT` が無いとき `GTEST_SKIP` です。採点されるのは精度の選択、予算、校正集合、mean の読取です。

## 対応する課題

```bash
./drill run cv04
```
