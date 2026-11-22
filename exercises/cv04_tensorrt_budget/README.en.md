# cv04 TensorRT and the frame budget [Vision]

Implement precision, the budget, the calibration set, and the trtexec mean.

## What to do

`choose_precision` returns Int8 only when gpu_int8 is set and there are at least 32 images. Otherwise Fp16 when gpu_fp16 is set, otherwise Fp32. `meets_frame_budget` is latency <= (1000/fps)*(1-margin). fps <= 0 is false. `speedup_ratio` is baseline/candidate, or 0 when candidate <= 0. `pick_calibration_set` starts at index 0 and greedily adds the farthest 8-bin L1 until it has count names. Ties take the smaller index. `parse_trtexec_mean_ms` reads the number after `mean = `.

## Run it

```bash
./drill run cv04
```

## Common pitfalls

Comparing the budget with `<` misses a latency that lands on the limit. The test uses `<=`.

## Tests

```bash
./drill run cv04
```

| Test | What it checks |
| --- | --- |
| `ChoosesInt8OnlyWithEnoughImages` | when INT8 is allowed |
| `FrameBudgetLeavesMargin` | the frame budget |
| `CalibrationSetSpreadsHistograms` | the calibration set |
| `ParsesTrtexecMean` | reading the mean |
| `TensorRtEngineBuildIsSkippedWithoutGpu` | the engine build is skipped |

## References

- [4. TensorRT and the frame budget](../../docs-en/vision/04_tensorrt_and_frame_budget.md)
