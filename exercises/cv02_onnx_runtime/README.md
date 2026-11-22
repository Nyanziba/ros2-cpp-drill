# cv02 ONNX Runtime 〔画像処理編〕

CPU の ONNX Runtime で、同梱モデルの色クラスを返します。

## やること

`classify_marker_onnx` を実装します。画素の B,G,R を 255 で割った平均を `[1,3]` で `mean_bgr` に入れ、`logits` の argmax を返します。クラスは 0 なし、1 赤、2 青。同点は小さい添字。空画像と nullptr は -1。モデルは `model/color_marker.onnx` です。

## 動かしてみる

```bash
./drill run cv02
```

## つまずきポイント

チャンネル順を RGB にすると赤と青が入れ替わります。セッションは呼び出しの中で作って構いません。

## テスト

```bash
./drill run cv02
```

| テスト | 見ているところ |
| --- | --- |
| `RejectsEmpty` | 空とヌル |
| `ClassifiesSolidColors` | 黒・赤・青・緑 |
| `HalfRedIsStillRed` | 半分の赤 |

## 参考

- [2. ONNX Runtime](../../docs/vision/02_onnx_runtime.md)
