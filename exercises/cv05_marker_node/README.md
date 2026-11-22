# cv05 マーカ検出ノード 〔画像処理編〕

image を購読し、marker に 1 行を出します。

## やること

`format_detection` は cv01 と同じ画素規則です。elapsed_ms > budget_ms なら `stale`。そうでなければ `red cx cy` / `blue cx cy` / `none`。`MarkerDetector` はトピック image と marker、パラメータ min_pixels（既定 3）と budget_ms（既定 40）、QoS は KeepLast(10) の reliable です。

## 動かしてみる

```bash
./drill run cv05
```

## つまずきポイント

SensorDataQoS にすると、テストの 1 枚が届かないことがあります。この課題は reliable のままにしてください。

## テスト

```bash
./drill run cv05
```

| テスト | 見ているところ |
| --- | --- |
| `FormatsRedBlueNoneAndStale` | 文字列の 4 通り |
| `PublishesBlueMarker` | ノードが blue 2 5 を出す |

## 参考

- [5. マーカ検出ノード](../../docs/vision/05_マーカ検出ノード.md)
