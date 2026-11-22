# nav03 経路と復帰 〔自律制御編〕

経路が試合時間に収まるか、座標系の並び、復帰 XML を実装します。

## やること

`plan_field_route` は地点が空でなく、負の leg_sec が無く、合計が match_time_left 以下（1e-9 の余裕）なら feasible。名前と合計は不成立でも埋めます。`field_frames_connected` は map→odom と odom→base_link の直接の辺が両方必要です。`slam_mode_name` は have_map かつ keep_mapping でないときだけ `localization`、それ以外は `mapping`。`nav_recovery_xml` はヘッダのコメントと完全一致させ、Spin の行は with_spin のときだけ入れます。

## 動かしてみる

```bash
./drill run nav03
```

## つまずきポイント

インデントか末尾改行が 1 文字でも違うと RecoveryXmlMatches が失敗します。

## テスト

```bash
./drill run nav03
```

| テスト | 見ているところ |
| --- | --- |
| `RejectsARouteThatOverrunsTheMatch` | 残り時間 |
| `NeedsMapToOdomToBase` | TF と SLAM のモード |
| `RecoveryXmlMatches` | 復帰 XML の完全一致 |
| `Nav2AndSlamLaunchAreSkipped` | 起動は skip |

## 参考

- [3. 経路と復帰](../../docs/autonomy/03_経路と復帰.md)
