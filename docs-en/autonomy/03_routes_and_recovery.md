# 3. Routes and recovery

- Japanese: https://nyanziba.github.io/ros2-cpp-drill/autonomy/03_経路と復帰.html
- English: https://nyanziba.github.io/ros2-cpp-drill/en/autonomy/03_routes_and_recovery.html
- Chinese: https://nyanziba.github.io/ros2-cpp-drill/zh/autonomy/03_routes_and_recovery.html

A route longer than the time left cannot reach the last score. The transform from the map to the robot also must not jump from map straight to base_link; odom sits between them. CI skips launching Nav2 and slam_toolbox, and grades the time check, the TF chain, the SLAM mode name, and the recovery XML.

## Time and frames

A route is feasible when it is not empty, no leg is negative, and the total is at most the time left (with 1e-9 of slack). Names and the total are filled in even when it is not feasible. TF needs both direct edges, map to odom and odom to base_link. The mode is localization only when a map already exists and you are not adding to it. Otherwise it is mapping.

| Item | Rule |
| --- | --- |
| feasible | non-empty, no negative leg, total <= time left |
| SLAM | localization only when have_map and not keep_mapping |

## Recovery

The XML for a stuck robot matches the block below exactly. Include the Spin line only when with_spin is set. Match the indentation and the trailing newline too.

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

## What CI does not grade

CI starts neither a map nor a simulator. The test that would tick a Nav2 recovery node is `GTEST_SKIP`.

## Matching exercise

```bash
./drill run nav03
```
