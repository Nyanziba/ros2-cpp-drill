# 3. Avoidance

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/behavior-tree/03_回避.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/behavior-tree/03_avoidance.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/behavior-tree/03_avoidance.html

An obstacle in front is handled before scoring. If it is already close enough to touch, stop before dodging. This chapter writes that rule both as a plain function and as a behavior tree, and the test requires the same command from both.

## The rule

If front is under 150 mm, Stop. Otherwise if it is under 400 mm, dodge toward the more open side. A tie goes left. If there is no obstacle and the goal is visible, Go. If the goal is not visible, Search.

| Item | Rule |
| --- | --- |
| Strings the tree writes | stop, left, right, go, search |
| XML | include a Fallback and a Sequence |

## Function and tree

`decide_avoidance` is the reference. The tree reads front_mm, left_mm, right_mm, and goal_visible from the blackboard and writes the same result to command. The rule is copied in two places, so do not fix only one of them.

```cpp
AvoidCommand command = decide_avoidance(front_mm, left_mm, right_mm, goal_visible);
// tree writes command: stop, left, right, go, search
```

## What CI does not grade

CI uses neither a LiDAR nor a motor. The test passes distances as integers.

## Matching exercise

```bash
./drill run bt03
```
