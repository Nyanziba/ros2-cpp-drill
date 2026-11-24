# 2. Actions and the blackboard

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/behavior-tree/02_アクションと黒板.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/behavior-tree/02_actions_and_blackboard.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/behavior-tree/02_actions_and_blackboard.html

Driving to a zone does not finish in one tick. BehaviorTree.CPP's `StatefulActionNode` has onStart, onRunning, and onHalted, which line up with a ROS 2 action: sent, still running, aborted. This chapter does not start a real action server. A stand-in client returns the same status codes.

## The goal

If less than 0.5 seconds remain, the zone is `abort`, the timeout is 0, and you must not send. Otherwise the timeout is `min(3.0, time_left)`. Status codes: EXECUTING (2) and CANCELING (3) are RUNNING, SUCCEEDED (4) is SUCCESS, and anything else is FAILURE.

| Item | Rule |
| --- | --- |
| Blackboard | drive_client, time_left, and the zone port |
| onHalted | cancel when halted while RUNNING |

## A real client

On the robot, an `rclcpp_action` client plays the role of `DriveClient`. The interface is only send, cancel, and status, so the test needs neither a server nor an executor.

```cpp
DriveGoal goal = make_drive_goal(zone, time_left_sec);
BT::NodeStatus status = status_from_action(code);
```

## What CI does not grade

CI does not start an action server. A FakeDrive only returns status codes.

## Matching exercise

```bash
./drill run bt02
```
