# cv04 TensorRT 与帧预算 〔图像处理〕

实现精度、预算、校准集合和 trtexec 的 mean。

## 要做的事

`choose_precision` 只在 gpu_int8 且图像至少 32 张时返回 Int8。否则 gpu_fp16 时返回 Fp16，再否则 Fp32。`meets_frame_budget` 是 latency <= (1000/fps)*(1-margin)。fps <= 0 为 false。`speedup_ratio` 是 baseline/candidate，candidate <= 0 时为 0。`pick_calibration_set` 从下标 0 开始，贪心加入 8 格 L1 最远的样本，直到 count 个。距离相同取较小下标。`parse_trtexec_mean_ms` 读取 `mean = ` 后面的数。

## 运行

```bash
./drill run cv04
```

## 容易卡住的地方

用 `<` 比较预算时，正好落在上限上的延迟会和期望不一致。测试用的是 `<=`。

## 测试

```bash
./drill run cv04
```

| 测试 | 检查的内容 |
| --- | --- |
| `ChoosesInt8OnlyWithEnoughImages` | 何时允许 INT8 |
| `FrameBudgetLeavesMargin` | 帧预算 |
| `CalibrationSetSpreadsHistograms` | 校准集合 |
| `ParsesTrtexecMean` | 读取 mean |
| `TensorRtEngineBuildIsSkippedWithoutGpu` | 引擎构建被跳过 |

## 参考

- [4. TensorRT 与帧预算](../../docs-zh/vision/04_tensorrt_and_frame_budget.md)
