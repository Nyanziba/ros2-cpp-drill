# 課題 02: トピックを購読する 〔初級〕

公式チュートリアル
[Writing a simple publisher and subscriber (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)
の `MinimalSubscriber` をそのまま書きます。

## やること

`src/minimal_subscriber.cpp` の TODO を埋めてください。仕様は公式チュートリアルと同一です。

| 項目 | 値 |
| --- | --- |
| ノード名 | `minimal_subscriber` |
| トピック名 | `topic` |
| 型 | `std_msgs::msg::String` |
| QoS depth | 10 |
| コールバック引数 | `const std_msgs::msg::String & msg`（ポインタではなく参照） |
| ログ | `I heard: '<本文>'` |

クラス宣言（`include/drill/minimal_subscriber.hpp`）は与えてあります。メンバ変数
`subscription_` に何を入れるかを考えてください。

## 動かしてみる

テストが通ったら、公式チュートリアルと同じように手で動かせます。

```bash
source install/setup.bash
ros2 run drill_02_subscriber listener
```

別の端末で:

```bash
ros2 topic pub /topic std_msgs/msg/String "{data: 'こんにちは'}" --once
ros2 node info /minimal_subscriber
```

`listener` を実行している端末に `I heard: 'こんにちは'` と出れば成功です。

## つまずきポイント

- `create_subscription()` の戻り値は必ず `subscription_` に代入します。
  ローカル変数で受けるとコンストラクタを抜けた時点で破棄され、コールバックが一度も呼ばれません。
- コールバックの引数は `const std_msgs::msg::String & msg` です。公式のパブリッシャ側
  （`msg->data` のようにポインタで受け取る例）と混同しないこと。
- `std::bind(&MinimalSubscriber::topic_callback, this, _1)` の `_1` を使うには
  `using std::placeholders::_1;` が必要です。

## テスト

```bash
./drill run 02
```

| テスト | 見ているところ |
| --- | --- |
| `topicを購読してログに出している` | Subscription が動いていて `I heard: '<本文>'` を出しているか |
| `複数通受信しても毎回ログが出る` | 購読が最初の 1 件で止まっていないか |
| `ノード名がminimal_subscriberになっている` | コンストラクタでのノード名指定 |

## 参考

- 公式: [Writing a simple publisher and subscriber (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)
- ローカルの実装例: `/opt/ros/jazzy/share/examples_rclcpp_minimal_subscriber/`
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
