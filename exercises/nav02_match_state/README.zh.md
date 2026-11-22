# nav02 状态机与行为树 〔自主控制〕

实现比赛的状态转移，以及每种工作使用的控制器。

## 要做的事

`next_state`：WaitStart+StartSignal 为 Search，Search+MarkerSeen 为 Approach，Approach+Aligned 为 Align，Align+Placed 为 Place，Place+Scored 为 Finished。Approach、Align、Place 的 MarkerLost 为 Search。TimeUp 把 Finished 以外变成 TimedOut，Finished 保持不变。TimedOut 和 Finished 忽略其他事件。没有写出的组合保持原状态。`choose_controller` 把 MatchPhase 映射为 StateMachine，ObstacleAvoidance 映射为 BehaviorTree，DriveToZone 映射为 StateMachineCallsBehaviorTree。

## 运行

```bash
./drill run nav02
```

## 容易卡住的地方

把 TimeUp 也用到 Finished 上，得分之后会掉进 TimedOut。

## 测试

```bash
./drill run nav02
```

| 测试 | 检查的内容 |
| --- | --- |
| `PlaysAPointAndStopsOnTime` | 一分的流程与时间到 |
| `PicksTheControllerForTheJob` | 如何选择控制器 |

## 参考

- [2. 状态机与行为树](../../docs-zh/autonomy/02_state_machines_and_behavior_trees.md)
