# 2. 动作与黑板

- 日语：https://nyanziba.github.io/ros2-cpp-drill/behavior-tree/02_アクションと黒板.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/behavior-tree/02_actions_and_blackboard.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/behavior-tree/02_actions_and_blackboard.html

开到某个区域不是一次 tick 能结束的。BehaviorTree.CPP 的 `StatefulActionNode` 有 onStart、onRunning、onHalted，对应 ROS 2 动作的「已发送、还在跑、被中断」。这一章不启动真实的动作服务器，而是插入一个返回同样状态号的客户端。

## 目标

剩余时间小于 0.5 秒时，zone 是 `abort`，timeout 是 0，并且不要发送。其余时候 timeout 是 `min(3.0, time_left)`。动作状态号里，EXECUTING(2) 和 CANCELING(3) 是 RUNNING，SUCCEEDED(4) 是 SUCCESS，其他是 FAILURE。

| 项目 | 规则 |
| --- | --- |
| 黑板 | drive_client、time_left，以及端口 zone |
| onHalted | 在 RUNNING 时被 halt 就要 cancel |

## 真实的客户端

装到机器人上时，`rclcpp_action` 的客户端扮演 `DriveClient`。接口只有 send、cancel、status，所以测试不需要服务器，也不需要执行器。

```cpp
DriveGoal goal = make_drive_goal(zone, time_left_sec);
BT::NodeStatus status = status_from_action(code);
```

## CI 不评分的部分

CI 不启动动作服务器。FakeDrive 只返回状态号。

## 对应的练习

```bash
./drill run bt02
```
