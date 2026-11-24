# 2. ONNX Runtime

- 日本語: https://nyanziba.github.io/ros2-cpp-drill/vision/02_onnx_runtime.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/02_onnx_runtime.html
- 中文: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/02_onnx_runtime.html

色のしきい値の次は、同じ「赤・青・どちらでもない」を小さなネットワークに任せます。学習は別の機械で行い、ロボットには ONNX だけを載せます。この章のモデルはリポジトリに同梱した 1 層の Gemm です。GPU は要りません。

## 入出力

入力は画像そのものではなく、B、G、R を 0 から 1 に正規化した平均 3 個です。出力 3 個の最大値の添字がクラスです。同点なら小さい添字です。空画像とヌルポインタは -1 です。

| 項目 | 決まり |
| --- | --- |
| 入力 mean_bgr | [1, 3] の float。順は B, G, R |
| 出力 logits | [1, 3]。0 なし、1 赤、2 青 |

## セッション

`Ort::Session` にモデルのパスを渡します。入力名と出力名はモデルに書いた `mean_bgr` と `logits` です。テンソルの形を間違えると実行時に失敗します。

```cpp
int klass = classify_marker_onnx(model_path, pixels, width, height);
// klass: -1 empty, 0 none, 1 red, 2 blue
```

## CI でやらないこと

学習も量子化も採点しません。CPU の ONNX Runtime（`onnxruntime_vendor`）で、同梱モデルの分類だけを見ます。

## 対応する課題

```bash
./drill run cv02
```
