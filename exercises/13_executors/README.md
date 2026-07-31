# 課題 13: Executor とコールバックグループ 〔上級〕

公式ドキュメント
[Using callback groups](https://docs.ros.org/en/jazzy/How-To-Guides/Using-callback-groups.html) と
[Executors（概念）](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Executors.html)
が扱っている問題を、実際にデッドロックするコードとして体験します。

この課題のテーマは「rclcpp を効率的に使い倒す」ことです。今までの課題はすべて
`rclcpp::spin()` に任せきりで済みましたが、実務ではそれだけでは足りない場面が
必ず出てきます。その代表例が、この課題で扱う **サブスクライバのコールバックの
中でサービスの応答を同期的に待つ** というコードです。

## やること

`src/relay_with_service.cpp` の TODO を埋めてください。

| 項目 | 値 |
| --- | --- |
| ノード名 | `relay_with_service` |
| 入力トピック | `trigger`（`std_msgs::msg::Int32`、depth 10） |
| 出力トピック | `sum`（`std_msgs::msg::Int32`、depth 10） |
| 依存サービス | `add_two_ints`（`example_interfaces::srv::AddTwoInts`） |
| 動作 | `trigger` で値 `v` を受けたら `add_two_ints` に `a = v, b = v` を投げ、**コールバックの中でその応答を待って**、得られた `sum` を `sum` に publish する |

クラス宣言（`include/drill/relay_with_service.hpp`）は与えてあります。必須の設計は
次の 4 点です。

1. コールバックグループを 2 つ作る（`subscription_group_` と `client_group_`。
   どちらも `rclcpp::CallbackGroupType::MutuallyExclusive`）。
2. `rclcpp::SubscriptionOptions` の `callback_group` に `subscription_group_` を
   指定して `"trigger"` の Subscription を作る。
3. `"add_two_ints"` のクライアントを、`create_client` の第3引数
   （callback group）に `client_group_` を渡して作る。
4. コールバックの中では `future.wait_for(...)` で応答を待つ
   （`rclcpp::spin_until_future_complete()` は**使わない**。理由は下記）。

## なぜデッドロックするのか

Executor は、1 つの `MutuallyExclusive` コールバックグループから **同時に 1 つの
コールバックしか実行しません**。これは「同じノードの状態を複数のコールバックが
同時に触って壊す」ことを防ぐための、rclcpp のデフォルトの安全策です。

ここで、サブスクライバとサービスクライアントを同じコールバックグループに置いた
まま、次のようなコードを書いたとします。

```cpp
void trigger_callback(const std_msgs::msg::Int32 & msg)
{
  auto future = client_->async_send_request(request);
  future.wait_for(2s);   // ここでブロックして応答を待つ
  ...
}
```

`add_two_ints` の応答が返ってくると、rclcpp 内部では「クライアントの完了処理」を
実行するためのコールバックがキューに積まれます。しかしこのコールバックは
`client_` と同じコールバックグループに属しています。そして `trigger_callback`
自身も同じグループに属しています。Executor はいま `trigger_callback` を実行中で、
そのグループから同時に実行できるコールバックは 1 つだけなので、
「クライアントの完了処理」は `trigger_callback` が終わるまで実行されません。

ところが `trigger_callback` は、その「クライアントの完了処理」が実行されて
future の中身が埋まるのを待っています。

- `trigger_callback` は完了処理が終わるのを待っている
- 完了処理は `trigger_callback` が終わるのを待っている（同じグループだから）

これが循環し、`future.wait_for()` に渡したタイムアウトが尽きるまで、あるいは
永遠に、両者とも進めなくなります。これがこの課題で体験するデッドロックです。
`SingleThreadedExecutor` はもちろん、コールバックグループを分けていない限り
`MultiThreadedExecutor` でスレッド数を増やしても解決しません。「別グループなら
同時に実行してよい」という情報を Executor に与えていないからです。

解決策は、サブスクライバとクライアントを **別の** コールバックグループに置き、
かつ Executor に複数のコールバックを同時に処理できるだけの余力（スレッド）を
持たせることです。この 2 つが揃って初めて、`trigger_callback` が待っている間に、
別スレッドが「クライアントの完了処理」を実行できるようになります。

## MutuallyExclusive と Reentrant

コールバックグループには 2 種類あります。

- **MutuallyExclusive**（既定）: そのグループに属するコールバックは、同時に
  1 つしか実行されない。複数のコールバックが同じメンバ変数を触っても
  データ競合が起きないという安心感がある一方、この課題のように
  「グループ内で待ち合わせ」が発生する構成だとデッドロックの原因になる。
- **Reentrant**: そのグループに属する複数のコールバックが、複数のスレッドで
  **同時に** 実行されうる。同じコールバックが自分自身と並行に走ることさえある。
  スループットは上がるが、メンバ変数へのアクセスを自分でロックするなど、
  スレッドセーフにする責任は書き手に移る。

この課題では両方のグループを `MutuallyExclusive` のままにしています。
「デッドロックを解消する」ために必要なのは Reentrant にすることではなく、
**グループを分ける**ことだからです。サブスクライバとクライアントが元々
無関係な処理である以上、それぞれのグループ内で排他されていれば十分です。

## SingleThreadedExecutor / MultiThreadedExecutor / StaticSingleThreadedExecutor

- **SingleThreadedExecutor**: 1 本のスレッドで、実行可能になったコールバックを
  順番に処理する。今までの課題（01〜07）はすべてこれで足りていました。
  コールバックグループをいくつ分けても、実行するスレッドが 1 本しかなければ
  「同時に」実行されることはないので、この課題の構成では必ずデッドロックします。
- **MultiThreadedExecutor**: 複数のスレッドで、実行可能になったコールバックを
  並行に処理する。コールバックグループが分かれていれば、別グループのコールバック
  を別スレッドで同時に実行できる。この課題で使うのはこれです。
- **StaticSingleThreadedExecutor**: SingleThreadedExecutor と同じく 1 本の
  スレッドだが、待ち受けるエンティティ（Subscription や Service など）の一覧を
  spin 開始時に一度だけ静的に構築する。実行時にエンティティが増減しない構成では
  SingleThreadedExecutor よりオーバーヘッドが小さい。スレッドが 1 本である点は
  変わらないので、この課題の構成を解決する手段にはならない。

## 動かしてみる

テストが通ったら、実際に手で動かして確認できます。まず別端末でサーバを
立てます（`drill_04_service_server` の `server` がなければ
`demo_nodes_cpp` の `add_two_ints_server` でも構いません）。

```bash
source install/setup.bash
ros2 run drill_04_service_server server
# もし無ければ: ros2 run demo_nodes_cpp add_two_ints_server
```

別の端末で `relay_with_service` を立てます。

```bash
source install/setup.bash
ros2 run drill_13_executors relay_with_service
```

さらに別の端末で `sum` を購読しておき、

```bash
ros2 topic echo /sum
```

もう一つ別の端末から `trigger` に publish します。

```bash
ros2 topic pub /trigger std_msgs/msg/Int32 "{data: 21}" --once
```

`ros2 topic echo /sum` の画面に `data: 42` が出れば成功です。試しに
`src/relay_with_service_main.cpp` の `MultiThreadedExecutor` を
`SingleThreadedExecutor` に変えて動かしてみると（動作確認用に一時的に、で
構いません。確認が終わったら必ず元に戻してください）、`sum` に何も
publish されなくなることが確認できます。

## つまずきポイント

- `create_callback_group()` の戻り値を 1 つの変数に使い回すと、
  `subscription_group()` と `client_group()` が同じオブジェクトを返してしまい、
  デッドロックが再現します。**2 回**呼んで、別々のメンバに入れてください。
- `SubscriptionOptions` を作っても `callback_group` に代入し忘れると、
  サブスクライバは既定のグループ（＝ノードの `default_callback_group`）に
  入ったままになります。
- `create_client` の第3引数がコールバックグループです。第1引数（サービス名）
  第2引数（QoS）を飛ばして直接グループを渡すことはできないので、
  `rclcpp::ServicesQoS()` を明示的に挟む必要があります。
- コールバックの中で `rclcpp::spin_until_future_complete(shared_from_this(), future)`
  のようなものを書きたくなりますが、これは使えません。すでに Executor の中で
  動いているコールバックの中から、同じノードに対してもう一度 spin を始めようと
  するため、例外またはデッドロックになります。`future.wait_for()` で待ってください。

## テスト

```bash
./drill run 13
```

| テスト | 見ているところ |
| --- | --- |
| `サブスクライバとクライアントが別のコールバックグループにいる` | 2 つのグループを別々に作っているか |
| `triggerに21を送るとsumに42がpublishされる` | 一連の動作とデッドロックしていないこと |
| `連続してtriggerを送っても毎回応答する` | 一度きりの偶然ではなく、構造として解決しているか |
| `負の値でも正しく計算する` | 境界値でも壊れていないか |

## 参考

- 公式: [Using callback groups](https://docs.ros.org/en/jazzy/How-To-Guides/Using-callback-groups.html)
- 公式: [About Executors](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Executors.html)
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
