# cv05 标记检测节点 〔图像处理〕

订阅 image，在 marker 上发布一行。

## 要做的事

`format_detection` 使用与 cv01 相同的像素规则。elapsed_ms > budget_ms 时返回 `stale`。否则是 `red cx cy`、`blue cx cy` 或 `none`。`MarkerDetector` 使用话题 image 和 marker，参数 min_pixels（默认 3）和 budget_ms（默认 40），QoS 为 KeepLast(10) 且 reliable。

## 运行

```bash
./drill run cv05
```

## 容易卡住的地方

改成 SensorDataQoS 时，测试发出的那一帧可能到不了。这个练习请保持 reliable。

## 测试

```bash
./drill run cv05
```

| 测试 | 检查的内容 |
| --- | --- |
| `FormatsRedBlueNoneAndStale` | 四种字符串 |
| `PublishesBlueMarker` | 节点发布 blue 2 5 |

## 参考

- [5. 标记检测节点](../../docs-zh/vision/05_marker_detector_node.md)
