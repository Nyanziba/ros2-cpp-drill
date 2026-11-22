# cv01 OpenCV と cv_bridge 〔画像処理編〕

BGR 画像から赤または青のマーカーを 1 つ見つけ、ROS の Image と往復します。

## やること

`src/marker_detect.cpp` を実装します。赤は R>=180 かつ R>G+50 かつ R>B+50、青は B>=180 かつ B>R+50 かつ B>G+50。多い色を採用し、同数は赤。画素数が min_pixels 未満、空、CV_8UC3 以外は klass 0。cx, cy は採用画素の整数平均です。`mat_to_ros_image` は encoding `bgr8`。`ros_image_to_mat` は bgr8 以外と短い data で空を返します。

## 動かしてみる

```bash
./drill run cv01
```

## つまずきポイント

R と B を逆にすると赤と青が入れ替わります。`toCvCopy` の一時オブジェクトが死ぬ前に clone してください。

## テスト

```bash
./drill run cv01
```

| テスト | 見ているところ |
| --- | --- |
| `FindsRedBlockCentroid` | 赤い塊の重心 |
| `FindsBlueBlock` | 青い塊 |
| `TiePrefersRed` | 同数は赤 |
| `RejectsSmallBlobAndWrongType` | 小さすぎる塊と型 |
| `RoundTripsBgr8` | bgr8 の往復 |
| `RejectsOtherEncodingsAndShortData` | 他の encoding と短い data |

## 参考

- [1. OpenCV と cv_bridge](../../docs/vision/01_opencvとcv_bridge.md)
