# 3. YOLO のデータセット

- 日本語: https://nyanziba.github.io/ros2-cpp-drill/vision/03_yoloデータセット.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/vision/03_yolo_dataset.html
- 中文: https://nyanziba.github.io/ros2-cpp-drill/zh/vision/03_yolo_dataset.html

マーカーの次は、ボール、ゴール、相手ロボット、スタートポールのように形で分ける対象です。学習スクリプトは GPU のマシンで回します。この章が採点するのは、その前の契約です。ラベルの1行、train と val の分け方、ONNX 書き出しの引数、クラス名です。

## ラベル1行

YOLO の1行は `class cx cy w h` で、画像幅と高さで割った 0 から 1 の数です。小数は 6 桁です。画像の外、面積がない箱、負のクラスは `ValueError` です。右端や下端が画像の端ちょうどなら、中に入っています。

| 項目 | 決まり |
| --- | --- |
| クラス名 | ball, goal_marker, opponent, start_pole |
| 検証の取り方 | (i+1) が val_every で割り切れる行だけ val |

## 書き出し

`export_argv` は Ultralytics の CLI と同じ並びのリストを返します。中で学習はしません。opset は 12 未満を拒否し、空の重みパスも拒否します。

```python
yolo_line(class_id, x_min, y_min, x_max, y_max, image_w, image_h)
split_records(names, val_every)
export_argv(weights_path, opset)
robocon_class_names()
```

## CI でやらないこと

GPU での学習は pytest で skip します。重みファイルも作りません。CI は文字列と分割だけを見ます。

## 対応する課題

```bash
./drill run cv03
```
