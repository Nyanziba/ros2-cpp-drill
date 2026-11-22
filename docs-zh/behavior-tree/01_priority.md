# 1. 优先级

- 日语：https://nyanziba.github.io/ros2-cpp-drill/behavior-tree/01_優先順位.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/behavior-tree/01_priority.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/behavior-tree/01_priority.html

比赛中的判断如果全写成一串 if，事后就很难把急停插进去。行为树用 Fallback 表示优先级：第一个成功的子节点获胜。这一章使用 BehaviorTree.CPP v4。

## 从上到下

急停优先于时间到。时间到优先于搜索和得分，这时整棵树返回 FAILURE，trace 保持为空。这个分支要用 Inverter 包住 TimeUp。距离小于 400 mm 是回避，看得见标记是得分，其余是搜索。正好 400 mm 不是障碍。

| 项目 | 规则 |
| --- | --- |
| estop | trace 是 estop，即使同时 time_up |
| time_up | FAILURE，trace 为空，用 Inverter 包住 |

## 节点种类

Sequence 要到每个子节点都成功才前进。Fallback 有一个子节点成功就算成功。Condition 只返回 SUCCESS 或 FAILURE，不返回 RUNNING。Action 把标签追加到黑板的 `trace`。

```cpp
register_match_nodes(factory);
auto tree = factory.createTreeFromText(match_xml());
tree.tickOnce();
```

## CI 不评分的部分

CI 不驱动机器人。一次 `tickOnce` 之后，只看黑板上的字符串。

## 对应的练习

```bash
./drill run bt01
```
