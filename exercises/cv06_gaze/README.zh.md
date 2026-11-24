# cv06 视线控制 〔图像处理〕

把云台转向检测目标，急停时保持不动。

## 要做的事

`update_gaze` 优先选择更小的 rank（0 比赛目标、1 人、2 其他）。同一 rank 保持当前目标。更好的 rank 会立刻抢占。看不到时保持角度并增加 lost_frames，第 5 次才放开锁定。平移速度是 kp*(cx-width/2)，俯仰速度是 -kp*(cy-height/2)。速度夹在 ±max_rate，角度夹在 pan 和 tilt 的范围内。`gaze_xml` 在 estop 时是 hold，target_visible 时是 track，否则是 search。XML 要包含 Fallback 和 Sequence。

## 运行

```bash
./drill run cv06
```

## 容易卡住的地方

如果忘了图像的 y 向下，俯仰会转到相反的方向。

## 测试

```bash
./drill run cv06
```

| 测试 | 检查的内容 |
| --- | --- |
| `ServosTowardGameObject` | 转向比赛目标的角速度 |
| `HigherRankPreemptsAndSameRankDoesNot` | 按 rank 切换 |
| `HoldsThroughFlickerThenDrops` | 保持到第 5 次丢失 |
| `ClampsPan` | 平移限位 |
| `XmlUsesFallbackAndSequence` | XML 的形状 |
| `EstopBeatsTracking` | hold、track 与 search |

## 参考

- [6. 视线控制](../../docs-zh/vision/06_gaze_control.md)
