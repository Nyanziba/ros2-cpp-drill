# 3. 路径与恢复

- 日语：https://nyanziba.github.io/ros2-cpp-drill/autonomy/03_経路と復帰.html
- English：https://nyanziba.github.io/ros2-cpp-drill/en/autonomy/03_routes_and_recovery.html
- 中文：https://nyanziba.github.io/ros2-cpp-drill/zh/autonomy/03_routes_and_recovery.html

比剩余时间更长的路径，来不及拿到最后的得分。从地图到机器人的坐标也不能从 map 直接跳到 base_link，中间要有 odom。CI 会跳过启动 Nav2 和 slam_toolbox，改为给时间判断、TF 的连接、SLAM 的模式名和恢复 XML 评分。

## 时间和坐标系

地点至少一个、没有负的路段，并且路段总和不超过剩余时间（允许 1e-9 的余量）时，feasible 为真。不成立时也要填上名字和总和。TF 需要 map 到 odom、odom 到 base_link 两条直接的边。只有已经有地图并且不再追加时才是 localization，否则是 mapping。

| 项目 | 规则 |
| --- | --- |
| feasible | 非空、没有负路段、总和 <= 剩余时间 |
| SLAM | have_map 且 not keep_mapping 时才是 localization |

## 恢复

卡住时的 XML 必须和下方字符串完全一致。只有 with_spin 时才放入 Spin 那一行。缩进和末尾换行也要一致。

```xml
<RecoveryNode number_of_retries="1" name="FieldRecovery">
  <Sequence name="primary">
    <Wait wait_duration="0.5"/>
  </Sequence>
  <Sequence name="recovery">
    <BackUp backup_dist="0.30" backup_speed="0.15"/>
    <Spin spin_dist="1.57"/>
  </Sequence>
</RecoveryNode>
```

## CI 不评分的部分

CI 不启动地图，也不启动仿真。真正 tick Nav2 recovery 节点的测试是 `GTEST_SKIP`。

## 对应的练习

```bash
./drill run nav03
```
