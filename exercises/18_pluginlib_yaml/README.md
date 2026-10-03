# 課題 18: YAML のパラメータでプラグインの列を組む（pluginlib）〔上級〕

課題 17 は「パラメータで 1 つ選ぶ」でした。この課題では **YAML に名前の列を書き、その順にプラグインを
並べて通す** ノード `FilterPipeline` を書きます。nav2 が controller / planner / costmap の層を
YAML の名前の列で組むのと同じ形です。3 つのフィルタ（課題 16 で作ったもの）は完成した形で渡してあります。

```yaml
filter_host:
  ros__parameters:
    filters: ["deadband", "clamp", "rate_limit"]   # この順に通す
    deadband:
      plugin: "drill/DeadbandFilter"               # <name>.plugin が pluginlib の名前
      linear_threshold: 0.05                       # <name>.<項目> は、プラグイン自身が宣言して読む
    clamp:
      plugin: "drill/ClampFilter"
      max_linear: 0.5
    rate_limit:
      plugin: "drill/RateLimitFilter"
      max_linear_acceleration: 1.0
```

## やること

`include/drill/filter_pipeline.hpp` と `src/filter_pipeline.cpp` の TODO を埋めてください。

| 項目 | 値 |
| --- | --- |
| クラス名 | `FilterPipeline`（`rclcpp::Node`。名前は `filter_host`。YAML の最上位のキーと同じ） |
| 順番 | パラメータ `filters`（文字列の配列。既定は空） |
| 各段の plugin 名 | `<name>.plugin`（文字列） |
| プラグインに渡す名前 | 段の名前 `name`。プラグインが `<name>.<項目>` を宣言する |
| ClassLoader | `pluginlib::ClassLoader<drill::VelocityFilter>("drill_18_pluginlib_yaml", "drill::VelocityFilter")` |
| 購読 / 出力 | `cmd_vel_in` を購読 → `apply` → `cmd_vel_out` に publish（`geometry_msgs/msg/Twist`、QoS depth 10） |
| `dt_seconds` | 購読者からは `kCommandPeriodSeconds`（0.05）を渡す |

1. **メンバを宣言する。** `ClassLoader`、フィルタの列（`std::vector<std::shared_ptr<drill::VelocityFilter>>`）、
   購読者、出し手。**`ClassLoader` を、フィルタの列より先に宣言します。**
2. **`configure()`**: `filters` を読み、各 `name` について `<name>.plugin` を読み、プラグインを作り、
   `initialize(shared_from_this(), name)` を呼んで、**読んだ順に**列へ入れます。購読者と出し手もここで作ります。
3. **未知の plugin 名のとき**（`pluginlib::PluginlibException`）は、段ごとの `try / catch` で受け、
   `RCLCPP_ERROR` に「どの段（`name`）の、どの plugin 名が、なぜ（`e.what()`）」読めなかったかを出して、
   **その段だけ飛ばします。** 残りの段は組み続け、ノードは落としません。
4. **`apply(command, dt_seconds)`**: 列の順にフィルタを通した結果を返します（空なら `command` のまま）。
5. **`filter_count()`**: 組み上がった段の数（読めなかった段は数えない）を返します。

## 動かしてみる

付属の YAML（`config/filters.yaml`）で起動して、別の端末から指令を送ってみます。

```bash
./drill run 18
source install/setup.bash
ros2 run drill_18_pluginlib_yaml filter_pipeline_node --ros-args \
  --params-file install/drill_18_pluginlib_yaml/share/drill_18_pluginlib_yaml/config/filters.yaml
```

```bash
ros2 topic echo /cmd_vel_out                                   # 別端末
ros2 topic pub -r 10 /cmd_vel_in geometry_msgs/msg/Twist "{linear: {x: 1.0}}"   # 別端末
```

`filters` の並びを入れ替えたり、`plugin` を打ち間違えたりして、出力がどう変わるかを見てください。
YAML は `--params-file` の代わりに、launch の `parameters=[...]` でも同じように渡せます。

## つまずきポイント

- **順番が結果を変えます。** 上限で切ってからしきい値で 0 にするのと、しきい値を通してから切るのとでは、
  結果が違います（テストはこの 2 通りを比べます）。`filters` の並びのまま `push_back` してください。
- YAML の最上位のキー（`filter_host`）は、**ノード名と一致**している必要があります。
  違うと、パラメータが読まれず、`filters` は空のままです。
- `<name>.plugin` は**宣言してから**読みます。宣言していないパラメータは、YAML に書いてあっても読めません。
  `<name>.max_linear` などは、プラグインが `initialize` の中で宣言します（ホストは宣言しません）。
- 1 つの段の失敗で全体を止めないでください。読めなかった段を**列に入れない**ことも忘れずに。
- 購読者と出し手はメンバに持ちます。ローカル変数にすると `configure()` を抜けた時点で消えます。
- `ClassLoader` を、フィルタの列より先に宣言してください（逆だと、壊れるときに class_loader の警告が出て、未定義の動作になりえます。課題 17 と同じです）。

## テスト

```bash
./drill run 18
```

YAML は `test/` にあり、`--ros-args --params-file` でノードに読ませます。

| テスト | 見ているところ |
| --- | --- |
| `AppliesFiltersInListedOrder` | `filters` の並びの順に通しているか（同じ 2 段を入れ替えた YAML 2 つで結果が変わるか） |
| `PassesEachStageItsOwnParameters` | 各段の `plugin` で選び、各プラグインに自分の `<name>.<項目>` が渡るか（指定しない項目は既定のまま） |
| `SkipsStageWithUnknownPluginAndKeepsTheRest` | 未知の plugin 名で落ちず、段の名前と plugin 名をログに出し、その段だけ飛ばして残りは動くか |
| `PassesThroughWhenFiltersIsEmpty` | `filters` が無いとき、段が 0 で通過するか |
| `PublishesFilteredCommandToCmdVelOut` | `cmd_vel_in` を購読し、フィルタを通して `cmd_vel_out` に出すか |
| `PipelineWithLiveFiltersIsDestroyedWithoutClassLoaderWarning` | フィルタを持ったまま壊しても class_loader の警告が出ないか（`ClassLoader` を先に宣言したか） |

## 参考

- 公式: [Creating and using plugins (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Pluginlib.html)
- 読み物: [ROS2講習15b: pluginlib](../../docs/ros2/15b_pluginlib.md)
- 前の課題: 16（プラグインを作る）、17（`ClassLoader` で読み込む）
