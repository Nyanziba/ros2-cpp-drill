# cv03 YOLO のデータセット 〔画像処理編〕

ラベル 1 行、分割、ONNX 書き出しの引数、クラス名を実装します。

## やること

`src/yolo_dataset.py` の 4 関数です。`yolo_line` は `class cx cy w h`（6 桁）。画像の外、面積なし、class_id < 0 は ValueError。端ちょうどは許可。`split_records` は (i+1) が val_every で割り切れるものだけ val。val_every <= 1 は ValueError。`export_argv` は `yolo export model=... format=onnx opset=... simplify=True`。空パスと opset < 12 は拒否。`robocon_class_names` は ball, goal_marker, opponent, start_pole。

## 動かしてみる

```bash
./drill run cv03
```

## つまずきポイント

cx を幅で割る前に画素のまま返すとテストに通りません。学習関数は書かなくて構いません。

## テスト

```bash
./drill run cv03
```

| テスト | 見ているところ |
| --- | --- |
| `test_yolo_line_normalizes_box` | 正規化と 6 桁 |
| `test_yolo_line_allows_far_edge_and_rejects_outside` | 端と外側 |
| `test_split_is_deterministic` | train と val |
| `test_export_argv_pins_onnx` | export の引数 |
| `test_class_names_match_the_practice_field` | 4 つのクラス名 |
| `test_gpu_training_is_not_run_in_ci` | GPU 学習は skip |

## 参考

- [3. YOLO のデータセット](../../docs/vision/03_yoloデータセット.md)
