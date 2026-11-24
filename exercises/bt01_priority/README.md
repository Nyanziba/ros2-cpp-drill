# bt01 優先順位 〔ビヘイビアツリー編〕

1 回の tick で黒板 trace を estop / avoid / score / search にします。

## やること

`match_xml` と `register_match_nodes` を実装します。estop なら trace は `estop` で SUCCESS。そうでなく time_up なら FAILURE かつ trace は空（TimeUp を Inverter で包む）。そうでなく range_mm < 400 なら `avoid`。そうでなく see_marker なら `score`。それ以外は `search`。XML には Fallback も含めます。

## 動かしてみる

```bash
./drill run bt01
```

## つまずきポイント

time_up の枝で Trace を実行すると、空であるべき trace に文字が残ります。

## テスト

```bash
./drill run bt01
```

| テスト | 見ているところ |
| --- | --- |
| `XmlInvertsTimeUp` | Inverter と Fallback |
| `EstopOutranksEverything` | estop が先 |
| `TimeUpStopsTheRest` | 時間切れは空の trace |
| `AvoidBeatsScoreAndSearchIsLast` | 回避、得点、探索 |

## 参考

- [1. 優先順位](../../docs/behavior-tree/01_優先順位.md)
