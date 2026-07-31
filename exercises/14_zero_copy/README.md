# 課題 14: ゼロコピー（unique_ptr とプロセス内通信）〔上級〕

公式ドキュメント
[Intra-Process Communication（デモ解説）](https://docs.ros.org/en/jazzy/Tutorials/Demos/Intra-Process-Communication.html)
にある「`publish(値)` と `publish(std::move(unique_ptr))` では、実際に運ばれるデータの
経路が違う」という話を、自分の手で確かめます。

## やること

`src/zero_copy_nodes.cpp` の TODO を埋めてください。

| 項目 | 値 |
| --- | --- |
| Talker ノード名 | `zero_copy_talker` |
| Listener ノード名 | `zero_copy_listener` |
| トピック名 | `zero_copy` |
| 型 | `std_msgs::msg::String` |
| QoS depth | 10 |
| Talker の `data` | 自分自身（メッセージ）のアドレスを 16 進文字列にしたもの |

クラス宣言（`include/drill/zero_copy_nodes.hpp`）は与えてあります。両クラスとも
コンストラクタは `const rclcpp::NodeOptions & options` を受け取り、
`Node("...", options)` にそのまま渡します。プロセス内通信を有効にするかどうかを
決めるのは呼び出し側（テストや `zero_copy_demo_main.cpp`）だからです。

## `publish(値)` と `publish(std::move(unique_ptr))` の違い

```cpp
auto message = std_msgs::msg::String();
message.data = "...";
publisher_->publish(message);              // (1) 値渡し
```

```cpp
auto msg = std::make_unique<std_msgs::msg::String>();
msg->data = "...";
publisher_->publish(std::move(msg));       // (2) 所有権を渡す
```

(1) は「値」を渡すので、rclcpp は publish の入口でメッセージをまず 1 回
コピーしてから中間バッファ（IntraProcessManager）に積みます。呼び出し側の
`message` はそのローカル変数のまま生き続けるので、コピーせずに済ませる方法が
ありません。

(2) は `unique_ptr` の「所有権」を `std::move` でそのまま渡すので、コピーは
一切発生しません。呼び出し側はもう `msg` を持っていない（`move` された）ので、
rclcpp はそのメモリをそのまま中間バッファに保管し、条件が揃えば購読側にも
同じアドレスのまま届けられます。

## プロセス内通信が効く条件 / 効かない条件

効く条件（すべて満たす必要がある）:

- Publisher と Subscription が**同一プロセス**内にある。
- 両方のノードが `rclcpp::NodeOptions().use_intra_process_comms(true)` で
  作られている。**既定値は `false`** なので、明示的に有効にしないと効きません
  （`rclcpp/node_options.hpp`: `bool use_intra_process_comms_ {false};`）。
- publish 側が `std::move(unique_ptr)` で所有権を渡している。
- QoS の History が `KeepLast` / `KeepAll`（depth を持つ形）であること。

効かない条件（コピーに戻る）:

- Publisher と Subscription が別プロセス（別の `ros2 run` / 別ノードプロセス）
  にある。DDS 越しの通信になるのでシリアライズ・デシリアライズが必要。
- **`use_intra_process_comms` を有効にしていない。** 同一プロセスに載せただけでは
  効きません（ローカルの実測でも、同一プロセス・既定オプションでは送信側と受信側で
  メッセージのアドレスが一致しませんでした）。
- publish に「値」を渡している（`publish(message)`）。
- 複数の購読者がいて、その一部が**書き換え可能な形**（`UniquePtr` / `SharedPtr`）で
  受け取りたい場合。単独所有を満たすためにその分のコピーが作られます。

### 購読側の引数の型について

`rclcpp/any_subscription_callback.hpp` が受け付ける形は
`const T &` / `T::UniquePtr` / `T::ConstSharedPtr` / `const T::ConstSharedPtr &` /
`T::SharedPtr`（＋ `MessageInfo` 付き・シリアライズ版）です。**「値で受け取る」形は
そもそもサポートされていません**（コンパイルが通りません）。

プロセス内通信が有効なら、`const T &` で受けてもコピーは発生しません
（Jazzy で実測。送信側と受信側のアドレスが一致します）。型は「コピーの有無」ではなく
**メッセージをどう扱いたいか**で選んでください。

| 受け方 | 意味 |
| --- | --- |
| `const T &` | その場で読むだけ。コールバックを抜けたら触れない |
| `T::ConstSharedPtr` | 読むだけ。保持したり他へ渡したりできる |
| `T::UniquePtr` | 単独所有。書き換えて再 publish できる |

なお `transient_local` の QoS でもプロセス内通信そのものは成立しました（Jazzy で実測）。
ただし「あとから参加した購読者への再送」はプロセス内バッファの外側の話なので、
そこは別に考えてください。

## 大きなデータでこれが効く理由

画像（`sensor_msgs::msg::Image`）や点群（`sensor_msgs::msg::PointCloud2`）は
数百 KB〜数 MB になることがあります。`publish(値)` はこのデータ全体を
メモリコピーしてから運ぶため、パイプラインの段数が増えるほどコピー回数が
線形に増えていきます。`publish(std::move(unique_ptr))` ならコピーが 0 回になるので、
同じパイプラインでも CPU 時間とメモリ帯域を大きく節約できます。
`/opt/ros/jazzy/include/intra_process_demo/image_pipeline/camera_node.hpp` の
`CameraNode` はまさにこの手法でカメラ画像を publish しています。

## 動かしてみる

テストが通ったら、`zero_copy_demo` 実行ファイルで実際にアドレスが一致する様子を
ログで確認できます。

```bash
source install/setup.bash
ros2 run drill_14_zero_copy zero_copy_demo
```

`Published message with address: 0x...` と `Received message with address: 0x...`
が交互に出て、同じ実行の中では常に同じアドレスになっているはずです。

比較として、公式の `intra_process_demo` パッケージにある `two_node_pipeline` も
動かしてみると良いでしょう（`camera_node` を単純化した、送受信 2 ノードだけの例）。

```bash
ros2 run intra_process_demo two_node_pipeline
```

## つまずきポイント

- `create_publisher()` / `create_subscription()` の戻り値は必ずメンバ変数に
  代入すること。
- `publish_once()` で作った `msg` を `publisher_->publish(msg)` と値で渡すと、
  その時点でコピーが発生してテスト 2（アドレス一致）が落ちます。
  `std::move(msg)` を忘れないこと。
- コールバックの引数は宣言済みの `ConstSharedPtr` のままにしてください。
  （`const &` でもゼロコピー自体は成立しますが、ヘッダの宣言と signature が
  合わなくなるとビルドが通りません。型の選び方は上の節を参照。）
- `NodeOptions` はコンストラクタで受け取ったものを `Node("...", options)` に
  そのまま渡すこと。ここで `Node("...")` と書いてしまうと
  `use_intra_process_comms` が伝わらず、常にコピーが発生してテストが落ちます。

## テスト

```bash
./drill run 14
```

| テスト | 見ているところ |
| --- | --- |
| `zero_copyトピックで通信できている` | Publisher / Subscription が動いているか |
| `送信側と受信側のアドレスが一致する` | `std::move` で publish しているか、購読側が `ConstSharedPtr` か |
| `dataの16進文字列も同じアドレスを指している` | `data` に正しくアドレスを書き込んでいるか |
| `複数回publishしても毎回アドレスが一致する` | 毎回きちんとゼロコピーになっているか |

## 参考

- 公式: [Intra-Process Communication（デモ解説）](https://docs.ros.org/en/jazzy/Tutorials/Demos/Intra-Process-Communication.html)
- 公式デモ: [ros2/demos intra_process_demo](https://github.com/ros2/demos/tree/jazzy/intra_process_demo)
- ローカルの実装例: `/opt/ros/jazzy/include/intra_process_demo/image_pipeline/camera_node.hpp`
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
