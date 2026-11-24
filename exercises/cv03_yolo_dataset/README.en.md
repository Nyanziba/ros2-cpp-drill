# cv03 A YOLO dataset [Vision]

Implement one label line, the split, the ONNX export arguments, and the class names.

## What to do

Four functions in `src/yolo_dataset.py`. `yolo_line` is `class cx cy w h` with six decimals. Outside the image, no area, or class_id < 0 raises ValueError. An edge that lands exactly on the border is allowed. `split_records` sends a row to val only when (i+1) is divisible by val_every. val_every <= 1 raises ValueError. `export_argv` is `yolo export model=... format=onnx opset=... simplify=True`. An empty path and opset < 12 are rejected. `robocon_class_names` is ball, goal_marker, opponent, start_pole.

## Run it

```bash
./drill run cv03
```

## Common pitfalls

Returning pixel centers before dividing by the image size fails the test. You do not need a training function.

## Tests

```bash
./drill run cv03
```

| Test | What it checks |
| --- | --- |
| `test_yolo_line_normalizes_box` | normalization and six decimals |
| `test_yolo_line_allows_far_edge_and_rejects_outside` | the edge and the outside |
| `test_split_is_deterministic` | train and val |
| `test_export_argv_pins_onnx` | the export arguments |
| `test_class_names_match_the_practice_field` | the four class names |
| `test_gpu_training_is_not_run_in_ci` | GPU training is skipped |

## References

- [3. A YOLO dataset](../../docs-en/vision/03_yolo_dataset.md)
