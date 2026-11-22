# 2. ONNX Runtime

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/vision/02_onnx_runtime.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/02_onnx_runtime.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/02_onnx_runtime.html

After a color threshold, the same three classes (red, blue, neither) can come from a small network. Training happens on another machine. The robot only carries ONNX. The model in this chapter is a one-layer Gemm committed in the repository. No GPU is required.

## Inputs and outputs

The input is not the image. It is three means, B, G, and R, scaled to 0..1. The class is the index of the largest of the three outputs. A tie takes the smaller index. An empty image or a null pointer returns -1.

| Item | Rule |
| --- | --- |
| Input mean_bgr | float [1, 3], in order B, G, R |
| Output logits | [1, 3]. 0 none, 1 red, 2 blue |

## Session

Pass the model path to `Ort::Session`. The names written into the model are `mean_bgr` and `logits`. A wrong tensor shape fails at run time.

```cpp
int klass = classify_marker_onnx(model_path, pixels, width, height);
// klass: -1 empty, 0 none, 1 red, 2 blue
```

## What CI does not grade

CI does not train and does not quantize. It only classifies with the committed model on CPU ONNX Runtime (`onnxruntime_vendor`).

## Matching exercise

```bash
./drill run cv02
```
