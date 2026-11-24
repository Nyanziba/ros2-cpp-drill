# bt02 动作与黑板 〔行为树〕

通过 DriveClient 发送目标，并把状态号映射成 NodeStatus。

## 要做的事

`make_drive_goal` 在 time_left < 0.5 时使用 zone `abort` 和 timeout 0。否则 timeout 是 min(3.0, time_left)，zone 保持不变。`status_from_action` 把 2 和 3 映射为 RUNNING，4 映射为 SUCCESS，其他为 FAILURE。树使用黑板 `drive_client` 和 `time_left`，以及端口 zone。onStart 发送并返回 RUNNING。abort 时不发送并返回 FAILURE。onHalted 调用 cancel。

## 运行

```bash
./drill run bt02
```

## 容易卡住的地方

abort 时仍然 send，会导致 DoesNotSendWhenTimeIsAlmostUp 失败。

## 测试

```bash
./drill run bt02
```

| 测试 | 检查的内容 |
| --- | --- |
| `CapsTimeoutAndAbortsLate` | timeout 与 abort |
| `MapsActionStatus` | 状态号 |
| `SendsGoalThenSucceeds` | 发送然后成功 |
| `DoesNotSendWhenTimeIsAlmostUp` | 时间所剩无几时不发送 |
| `HaltCancels` | halt 时 cancel |

## 参考

- [2. 动作与黑板](../../docs-zh/behavior-tree/02_actions_and_blackboard.md)
