# 課題 01: トピックに publish する 〔初級〕

公式チュートリアル
[Writing a simple publisher and subscriber (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)
の `MinimalPublisher` をそのまま書きます。

## やること

`src/minimal_publisher.cpp` の TODO を埋めてください。仕様は公式チュートリアルと同一です。

| 項目 | 値 |
| --- | --- |
| ノード名 | `minimal_publisher` |
| トピック名 | `topic` |
| 型 | `std_msgs::msg::String` |
| QoS depth | 10 |
| 周期 | 500ms |
| 本文 | `"Hello, world! " + std::to_string(count_++)` |
| ログ | `Publishing: '<本文>'` |

クラス宣言（`include/drill/minimal_publisher.hpp`）は与えてあります。メンバ変数
`publisher_` / `timer_` / `count_` に何を入れるかを考えてください。

## 動かしてみる

テストが通ったら、公式チュートリアルと同じように手で動かせます。

```bash
source install/setup.bash
ros2 run drill_01_publisher talker
```

別の端末で:

```bash
ros2 topic echo /topic
ros2 topic hz /topic          # 2Hz になっているか
ros2 node info /minimal_publisher
```

## つまずきポイント

- `create_publisher()` / `create_wall_timer()` の戻り値は必ずメンバ変数に代入します。
  ローカル変数で受けるとコンストラクタを抜けた時点で破棄され、何も起きません。
- `std::bind(&MinimalPublisher::timer_callback, this)` の `this` を忘れないこと。
- `count_++` は**後置**です。1 通目は `Hello, world! 0` になります。

## テスト

```bash
./drill run 01
```

| テスト | 見ているところ |
| --- | --- |
| `topicトピックにpublishしている` | Publisher とタイマが動いているか |
| `本文がHello_worldと連番になっている` | 本文の組み立てと `count_++` |
| `おおよそ500ミリ秒周期でpublishしている` | タイマの周期 |
| `公式と同じPublishingログを出している` | `RCLCPP_INFO` の書式 |

## 参考

- 公式: [Writing a simple publisher and subscriber (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)
- ローカルの実装例: `/opt/ros/jazzy/share/examples_rclcpp_minimal_publisher/`
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
