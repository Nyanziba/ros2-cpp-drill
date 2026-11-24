# bt03 回避 〔行为树〕

根据距离决定 AvoidCommand，并在树里写出同样的结果。

## 要做的事

`decide_avoidance` 在 front < 150 时返回 Stop。否则当 front < 400 时，left >= right 返回 DodgeLeft，不然返回 DodgeRight。没有障碍时，goal_visible 则 Go，否则 Search。树读取黑板 front_mm、left_mm、right_mm、goal_visible，把 command 设为 stop、left、right、go 或 search。XML 要包含 Fallback 和 Sequence。

## 运行

```bash
./drill run bt03
```

## 容易卡住的地方

只改函数不改树，MatchesTheFunction 会失败。

## 测试

```bash
./drill run bt03
```

| 测试 | 检查的内容 |
| --- | --- |
| `StopsBeforeDodging` | 先停止 |
| `MatchesTheFunction` | 树与函数一致 |

## 参考

- [3. 回避](../../docs-zh/behavior-tree/03_avoidance.md)
