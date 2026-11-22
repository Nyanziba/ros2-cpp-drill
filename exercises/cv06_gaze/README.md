# cv06 視線制御 〔画像処理編〕

画像上の検出へ雲台を向け、非常停止では保持します。

## やること

`update_gaze` は rank が小さい対象を優先します（0 競技、1 人、2 その他）。同じ rank では今の対象を維持します。より良い rank は即時に乗り換えます。見えない更新は角度を保持して lost_frames を増やし、5 回目でロックを外します。パン速度は kp*(cx-width/2)、チルト速度は -kp*(cy-height/2)。速度は ±max_rate、角度は pan/tilt の範囲です。`gaze_xml` は estop なら hold、target_visible なら track、それ以外は search。Fallback と Sequence を含めます。

## 動かしてみる

```bash
./drill run cv06
```

## つまずきポイント

画像の y を上向きだと思ったまま符号を反転しないと、チルトが逆に動きます。

## テスト

```bash
./drill run cv06
```

| テスト | 見ているところ |
| --- | --- |
| `ServosTowardGameObject` | 競技対象への角速度 |
| `HigherRankPreemptsAndSameRankDoesNot` | rank の乗り換え |
| `HoldsThroughFlickerThenDrops` | 5 回まで保持 |
| `ClampsPan` | パンの制限 |
| `XmlUsesFallbackAndSequence` | XML の形 |
| `EstopBeatsTracking` | hold / track / search |

## 参考

- [6. 視線制御](../../docs/vision/06_視線制御.md)
