# cv04 TensorRT とフレーム予算 〔画像処理編〕

精度、予算、校正集合、trtexec の mean を実装します。

## やること

`choose_precision` は gpu_int8 かつ画像 32 枚以上のときだけ Int8。そうでなく gpu_fp16 なら Fp16、それ以外は Fp32。`meets_frame_budget` は latency <= (1000/fps)*(1-margin)。fps <= 0 は false。`speedup_ratio` は baseline/candidate、candidate <= 0 なら 0。`pick_calibration_set` は添字 0 を種に、8 ビンの L1 が最も遠いものを count 枚まで足します。同距離は小さい添字。`parse_trtexec_mean_ms` は `mean = ` の後ろの数です。

## 動かしてみる

```bash
./drill run cv04
```

## つまずきポイント

予算の比較を `<` にすると、上限ちょうどの 23 ms 付近で期待とずれます。テストは `<=` です。

## テスト

```bash
./drill run cv04
```

| テスト | 見ているところ |
| --- | --- |
| `ChoosesInt8OnlyWithEnoughImages` | INT8 の条件 |
| `FrameBudgetLeavesMargin` | フレーム予算 |
| `CalibrationSetSpreadsHistograms` | 校正集合 |
| `ParsesTrtexecMean` | mean の読取 |
| `TensorRtEngineBuildIsSkippedWithoutGpu` | エンジンビルドは skip |

## 参考

- [4. TensorRT とフレーム予算](../../docs/vision/04_tensorrtとフレーム予算.md)
