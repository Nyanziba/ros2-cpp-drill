# bt01 优先级 〔行为树〕

一次 tick 把黑板 trace 设为 estop、avoid、score 或 search。

## 要做的事

实现 `match_xml` 和 `register_match_nodes`。若 estop，trace 是 `estop` 且状态为 SUCCESS。否则若 time_up，则为 FAILURE 且 trace 为空（用 Inverter 包住 TimeUp）。否则若 range_mm < 400，为 `avoid`。否则若 see_marker，为 `score`。其余为 `search`。XML 里还要有 Fallback。

## 运行

```bash
./drill run bt01
```

## 容易卡住的地方

在 time_up 的分支上执行 Trace，会在必须保持为空的 trace 里留下文字。

## 测试

```bash
./drill run bt01
```

| 测试 | 检查的内容 |
| --- | --- |
| `XmlInvertsTimeUp` | Inverter 与 Fallback |
| `EstopOutranksEverything` | estop 最先 |
| `TimeUpStopsTheRest` | 时间到时 trace 为空 |
| `AvoidBeatsScoreAndSearchIsLast` | 回避、得分、然后搜索 |

## 参考

- [1. 优先级](../../docs-zh/behavior-tree/01_priority.md)
