# 1. Priority

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/behavior-tree/01_優先順位.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/behavior-tree/01_priority.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/behavior-tree/01_priority.html

Match decisions written as one long chain of if statements make it hard to insert an emergency stop later. A behavior tree expresses priority with a Fallback: the first child that succeeds wins. This chapter uses BehaviorTree.CPP v4.

## Top to bottom

Emergency stop outranks the time limit. The time limit outranks search and scoring, and in that case the whole tree returns FAILURE and the trace stays empty. Wrap TimeUp in an Inverter for that branch. A range under 400 mm is avoidance, a visible marker is scoring, and anything else is search. Exactly 400 mm is not an obstacle.

| Item | Rule |
| --- | --- |
| estop | trace is estop, even when time_up is also set |
| time_up | FAILURE, empty trace, wrapped in an Inverter |

## Node kinds

A Sequence advances until every child succeeds. A Fallback succeeds when one child succeeds. A Condition returns only SUCCESS or FAILURE, never RUNNING. An Action appends its label to the blackboard `trace`.

```cpp
register_match_nodes(factory);
auto tree = factory.createTreeFromText(match_xml());
tree.tickOnce();
```

## What CI does not grade

CI does not move a robot. After one `tickOnce`, it only reads the string on the blackboard.

## Matching exercise

```bash
./drill run bt01
```
