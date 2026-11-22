# nav03 路径与恢复 〔自主控制〕

实现路径是否放得进比赛时间、坐标系的连接，以及恢复 XML。

## 要做的事

`plan_field_route` 在地点非空、没有负的 leg_sec、并且总和不超过 match_time_left（余量 1e-9）时 feasible。不成立时也要填上名字和总和。`field_frames_connected` 需要 map 到 odom、odom 到 base_link 两条直接的边。`slam_mode_name` 只在 have_map 且不是 keep_mapping 时为 `localization`，否则为 `mapping`。`nav_recovery_xml` 必须与头文件注释完全一致，只有 with_spin 时才放入 Spin 那一行。

## 运行

```bash
./drill run nav03
```

## 容易卡住的地方

缩进或末尾换行差一个字符，RecoveryXmlMatches 就会失败。

## 测试

```bash
./drill run nav03
```

| 测试 | 检查的内容 |
| --- | --- |
| `RejectsARouteThatOverrunsTheMatch` | 剩余时间 |
| `NeedsMapToOdomToBase` | TF 与 SLAM 模式 |
| `RecoveryXmlMatches` | 恢复 XML 完全一致 |
| `Nav2AndSlamLaunchAreSkipped` | 启动被跳过 |

## 参考

- [3. 路径与恢复](../../docs-zh/autonomy/03_routes_and_recovery.md)
