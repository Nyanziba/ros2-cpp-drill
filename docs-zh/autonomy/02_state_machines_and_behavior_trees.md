# 2. 状态机与行为树

- 日语：https://nyanziba.github.io/ros2-cpp-drill/autonomy/02_状態機械とビヘイビアツリー.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/autonomy/02_state_machines_and_behavior_trees.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/autonomy/02_state_machines_and_behavior_trees.html

三分钟的比赛有一条可以回到已知位置的流程：开始、搜索、接近、放置、结束。这适合状态机。障碍回避这种每个周期都要问「现在什么最重要」的处理，适合行为树。开到区域是在某个状态里面 tick 一棵树。

## 转移

WaitStart 遇到 StartSignal 进入 Search。Search 遇到 MarkerSeen 进入 Approach，Approach 遇到 Aligned 进入 Align，Align 遇到 Placed 进入 Place，Place 遇到 Scored 进入 Finished。Approach、Align、Place 遇到 MarkerLost 都回到 Search。TimeUp 把 Finished 以外都变成 TimedOut。Finished 上的 TimeUp 仍是 Finished。TimedOut 和 Finished 忽略其他事件。

| 项目 | 规则 |
| --- | --- |
| 比赛阶段 | StateMachine |
| 障碍回避 | BehaviorTree |

## 怎么选

`choose_controller` 把 MatchPhase 映射到状态机，把 ObstacleAvoidance 映射到行为树，把 DriveToZone 映射到「状态机去调用行为树」。这一章的结论是不要把所有事都塞进其中一种。

```cpp
MatchState state = next_state(current, event);
Controller which = choose_controller(task);
```

## CI 不评分的部分

CI 不接裁判信号，也不接时钟。它一次传入一个事件，只检查下一个状态。

## 对应的练习

```bash
./drill run nav02
```
