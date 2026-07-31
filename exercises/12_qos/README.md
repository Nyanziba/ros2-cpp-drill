# 課題 12: QoS プロファイルを設計する 〔上級〕

公式ドキュメント
[About Quality of Service settings](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Quality-of-Service-Settings.html)
にある「あとから起動した購読者にも最後の値を届ける」パターン（TRANSIENT_LOCAL）を、
自分の手で publisher / subscriber の両方に組み込みます。

## QoS とは何か

QoS（Quality of Service）は「送る側と受ける側の契約」です。トピック名と型が
一致していても、QoS が噛み合っていなければ **黙って繋がりません**。エラーは
出ません。ただログを眺めても publish しているのに何も届かない、という状態に
なるだけです。だからこそ QoS は「まず仕様を覚え、次に実際に繋がらない状態を
自分で作って壊れ方を体験する」のが一番身につきます。

## QoS ポリシー一覧

| ポリシー | 何を決めるか |
| --- | --- |
| History | Publisher/Subscription が内部にいくつメッセージを保持するかの数え方。`KEEP_LAST`（直近 depth 件）と `KEEP_ALL`（許す限り全部）。 |
| Depth | History が `KEEP_LAST` のときのキューの長さ。溢れると古いものから捨てられる。 |
| Reliability | 届け方の保証。`RELIABLE` は再送してでも届ける。`BEST_EFFORT` は届かなくても構わない（センサのように新しい値が来ればいい場合向き）。 |
| Durability | あとから来た購読者に過去のメッセージを渡すかどうか。`VOLATILE`（既定。購読開始後の分だけ）と `TRANSIENT_LOCAL`（Publisher が最後の値を保持し、後から来た購読者にも配る）。 |
| Deadline | 「このトピックは最低これくらいの周期で更新されるはず」という期待。破ったことをアプリに通知できる。 |
| Lifespan | メッセージの有効期限。過ぎたメッセージは（まだ配送前でも）捨てられる。 |
| Liveliness | Publisher が生きていることをどう判定するか。ノードダウンの検知に使う。 |

この課題で扱うのは History / Depth / Reliability / Durability の 4 つです。

## TRANSIENT_LOCAL が要る場面・要らない場面

**要る場面**（変わらないが、あとから起動したノードにも必ず要るデータ）
- 地図（`/map` のような static map。作り直すまで変わらない）
- ロボット記述（`/robot_description`。URDF は起動後ずっと同じ）
- 静的な設定値（キャリブレーション結果、初期パラメータの配信など）

**要らない場面**（次々に新しい値が来る、古い値には価値がないデータ）
- センサの生データ（LiDAR や カメラ。1 秒前の値をあとから貰っても意味がない）
- 高頻度のオドメトリや制御指令

センサ系のトピックを TRANSIENT_LOCAL にすると、Publisher 側が過去のメッセージを
保持し続けるコストがかかるだけで得るものがありません。逆に地図を VOLATILE に
すると、あとから立ち上げたノードはその地図を一生受け取れません。

## QoS が非互換だとどうなるか

**黙って繋がりません。** `ros2 topic list` にも `ros2 node info` にも出てきますし、
`rclcpp::Node::count_publishers()` / `count_subscribers()` は 0 より大きい値を
返すことすらあります（discovery 自体はできるため）。しかし実際のメッセージは
一切届きません。

繋がっているかどうかは `ros2 topic info -v <topic>` で確認します。Publisher と
Subscription それぞれの実際の QoS（RELIABILITY / DURABILITY / ...）が表示される
ので、双方を見比べてください。ズレていれば、そこが繋がらない原因です。

## 既製の QoS プロファイル

毎回 `rclcpp::QoS` を組み立てなくても、用途別の既製プロファイルが用意されています。

| プロファイル | 主な設定 | 用途 |
| --- | --- | --- |
| `rclcpp::SensorDataQoS()` | `BEST_EFFORT` + 小さめの depth | カメラ・LiDAR など、取りこぼしより低遅延を優先するセンサデータ |
| `rclcpp::SystemDefaultsQoS()` | RMW の既定値 | 特にこだわりがないとき（`create_publisher(topic, 10)` の 10 と同じ系統） |
| `rclcpp::ServicesQoS()` | `RELIABLE` + `VOLATILE` | サービス・アクションの内部トピック |
| `rclcpp::ParametersQoS()` | `RELIABLE` + `VOLATILE`、大きめ depth | パラメータサービス関連 |

この課題のように「地図・設定値の latch 配信」がしたい場合、既製プロファイルには
ぴったり合うものが無いため、`rclcpp::QoS` を自分で組み立てます。

## やること

`src/qos_nodes.cpp` の TODO を埋めてください。

| 項目 | 値 |
| --- | --- |
| `LatchedPublisher` のノード名 | `latched_publisher` |
| `LatchedSubscriber` のノード名 | `latched_subscriber` |
| トピック名 | `config` |
| 型 | `std_msgs::msg::String` |
| QoS | `KeepLast(1)` + `TRANSIENT_LOCAL` + `RELIABLE`（publisher・subscription 両方に同じもの） |
| publish ログ | `Published config: '<値>'` |
| 受信ログ | `Received config: '<値>'` |

クラス宣言（`include/drill/qos_nodes.hpp`）は与えてあります。ポイントは
**publisher と subscription の両方に、まったく同じ QoS を渡すこと** です。
片方だけ `transient_local()` にしても意味がありません。

## 動かしてみる

```bash
source install/setup.bash
ros2 run drill_12_qos qos_demo
```

`qos_demo` は publisher を作って 1 回 publish したあと、**2 秒待ってから**
subscriber を起動します。それでもログに `Received config:` が出れば、
TRANSIENT_LOCAL が効いている証拠です。

別の端末で:

```bash
ros2 topic info -v /config
ros2 topic echo /config --qos-durability transient_local --qos-depth 1
```

`ros2 topic info -v` では publisher / subscription 双方の実際の QoS
（DURABILITY・RELIABILITY など）が確認できます。`ros2 topic echo` はデフォルトが
`VOLATILE` なので、`--qos-durability transient_local` を付けないと
qos_demo が publish し終わったあとに実行しても何も表示されません
（これも「非互換で繋がらない」の一例です）。

## つまずきポイント

- `create_publisher()` / `create_subscription()` の QoS 引数に渡すのは
  `rclcpp::QoS` オブジェクトです。`10` のような整数を渡すと depth 10 の
  デフォルト QoS（`VOLATILE` + `RELIABLE`）になってしまい、TRANSIENT_LOCAL には
  なりません。
- publisher・subscription のどちらか片方だけ `transient_local()` にしても
  繋がりません。両方に同じ QoS を渡してください。
- `actual_qos()` は「要求した QoS」ではなく「実際に採用された QoS」
  （`get_actual_qos()`）を返します。RMW によっては要求と食い違うことがあるため、
  テストは常にこちらを見ます。

## テスト

```bash
./drill run 12
```

| テスト | 見ているところ |
| --- | --- |
| `publisherの実効QoSがTRANSIENT_LOCALかつRELIABLEでdepth1になっている` | `actual_qos()` の中身 |
| `あとから起動した購読者にも過去にpublishした値が届く` | TRANSIENT_LOCAL の本質（latch 配信） |
| `新しい値をpublishすれば購読者に届く` | 普通の配信経路が壊れていないか |
| `VOLATILEで購読すると過去の値は届かない` | durability は「購読側が要求した設定」で決まることの確認 |

## 参考

- 公式: [About Quality of Service settings](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Quality-of-Service-Settings.html)
- デモ: [quality_of_service_demo](https://github.com/ros2/demos/tree/jazzy/quality_of_service_demo)
- ローカルの関連パッケージ: `/opt/ros/jazzy/share/quality_of_service_demo_cpp/`
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
