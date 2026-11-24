# 3. 回避

- 日语：https://nyanziba.github.io/ros2-cpp-drill/behavior-tree/03_回避.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/behavior-tree/03_avoidance.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/behavior-tree/03_avoidance.html

前方的障碍要在得分之前处理。但如果已经近到会碰上，就先停，再考虑躲开。这一章把这条规则写成普通函数和行为树两份，测试要求两边给出同一个命令。

## 规则

front 小于 150 mm 就是 Stop。否则若小于 400 mm，就朝更空的一侧躲开。左右相同则向左。没有障碍且看得到目标就是 Go，看不到就是 Search。

| 项目 | 规则 |
| --- | --- |
| 树写下的字符串 | stop, left, right, go, search |
| XML | 要包含 Fallback 和 Sequence |

## 函数和树

`decide_avoidance` 是基准。树从黑板读取 front_mm、left_mm、right_mm、goal_visible，把同样的结果写到 command。规则写在两处，不要只改一边。

```cpp
AvoidCommand command = decide_avoidance(front_mm, left_mm, right_mm, goal_visible);
// tree writes command: stop, left, right, go, search
```

## CI 不评分的部分

CI 不用激光雷达，也不用电机。距离由测试以整数传入。

## 对应的练习

```bash
./drill run bt03
```
