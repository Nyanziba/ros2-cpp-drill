# cv02 ONNX Runtime [Vision]

Return the color class of the committed model with CPU ONNX Runtime.

## What to do

Implement `classify_marker_onnx`. Feed the mean of B,G,R divided by 255 as `[1,3]` named `mean_bgr`, and return the argmax of `logits`. Classes are 0 none, 1 red, 2 blue. A tie takes the smaller index. An empty image and a nullptr return -1. The model is `model/color_marker.onnx`.

## Run it

```bash
./drill run cv02
```

## Common pitfalls

RGB order swaps red and blue. Creating the session inside the call is fine.

## Tests

```bash
./drill run cv02
```

| Test | What it checks |
| --- | --- |
| `RejectsEmpty` | empty and null |
| `ClassifiesSolidColors` | black, red, blue, and green |
| `HalfRedIsStillRed` | half red is still red |

## References

- [2. ONNX Runtime](../../docs-en/vision/02_onnx_runtime.md)
