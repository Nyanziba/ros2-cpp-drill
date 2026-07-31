# rclcpp の設計思想

この文書は、この練習帳の各課題で出てくる `create_publisher()` や `spin()` が 「なぜそう書くように作られているのか」を、rclcpp のヘッダとドキュメントを実際に 読んで説明するものです。API を丸暗記するのではなく、rclcpp が何を大事にして 組まれているコードなのかを掴んでおけば、初めて見る API でも「たぶんこう振る舞う はずだ」と構造から推測できるようになります。

対象読者は C++ は書けるが ROS 2 は初めて〜中級の方です。01〜11 の初級・中級課題を 一通り触ったあと、12〜15 の上級課題（QoS、Executor とコールバックグループ、ゼロコピー、 コンポーネント化）に進む前の下敷きとして読むことを想定しています。 C++ 自体に不安があれば、先に [C++ 講習](cpp/README.md) を読んでください。

本文中で「Jazzy では」と書いている箇所は、バージョンによって変わりうる実装の詳細です。 それ以外は rclcpp の設計としてある程度普遍的な話のつもりですが、断定を避けたい箇所は 「〜と読める」のように留保しています。引用したファイルはすべて `/opt/ros/jazzy/include/` 以下にあるので、手元で `grep` して確かめられます。

## 目次

1. [なぜ rclcpp は薄いのか](#1-なぜ-rclcpp-は薄いのか)
2. [Node は神クラスではなく、インタフェースの束](#2-node-は神クラスではなくインタフェースの束)
3. [すべては「作って、shared_ptr で持つ」](#3-すべては作ってshared_ptr-で持つ)
4. [待つのは Executor の仕事](#4-待つのは-executor-の仕事)
5. [コールバックグループ — 並行実行の単位](#5-コールバックグループ--並行実行の単位)
6. [メッセージの所有権とゼロコピー](#6-メッセージの所有権とゼロコピー)
7. [QoS は契約](#7-qos-は契約)
8. [パラメータは「宣言してから使う」](#8-パラメータは宣言してから使う)
9. [1 プロセス 1 ノードをやめる](#9-1-プロセス-1-ノードをやめる)
10. [rclcpp を効率的に使うための指針](#10-rclcpp-を効率的に使うための指針)
11. [もっと深く知るには](#11-もっと深く知るには)

---

## 1. なぜ rclcpp は薄いのか

> 要約: rclcpp は「C++ の顔をした rcl」です。ROS の概念（ノード、トピック、 パラメータ…）の実際のロジックは C 言語の rcl 層にあり、rclcpp はそれに shared_ptr による寿命管理とテンプレートによる型付けを被せているだけです。 さらに下には rmw という抽象層があり、その先で実際の DDS 実装が通信します。

`rclcpp::Node` や `rclcpp::Publisher` を見ていると、C++ のクラス階層がすべてを やっているように見えます。しかし `publisher.hpp` の publish の実装を辿ると、 最終的にやっていることは 1 行の C 関数呼び出しです。

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/publisher.hpp
auto status = rcl_publish(publisher_handle_.get(), &msg, nullptr);
```

`node.hpp` の include を見ても、`rcl/error_handling.h` と `rcl/node.h` という C ヘッダが真っ先に取り込まれています。rclcpp は独自に通信ロジックを実装している わけではなく、C 言語で書かれた `rcl`（ROS Client Library）の薄いラッパーです。

なぜ間に C の層が挟まっているのでしょうか。公式ドキュメント （`Concepts/Basic/About-Client-Libraries.rst`）はこう説明しています。

> rather than implementing the common functionality from scratch, client libraries make use of a common core ROS Client Library (RCL) interface that implements logic and behavior of ROS concepts that is not language-specific.

ノード名の解決やパラメータの扱いといった「ROS の概念としてのロジック」を rcl に 1 箇所だけ実装しておけば、C++ の rclcpp と Python の rclpy が同じ挙動を共有できます。 利点は 2 つ、一貫性（rcl のロジックを直せば全言語バインディングに反映される）と 保守性（バグ修正が 1 箇所で済む）だとドキュメントは述べています。C 言語が選ばれて いるのは「他の言語がラップしやすい言語だから」です。

層構造を図にすると次のようになります。

```
+----------------------------------------------------+
|  あなたのコード（MinimalPublisher など）              |
+----------------------------------------------------+
|  rclcpp   … C++ の型・shared_ptr による寿命管理        |
|            （Node, Publisher<T>, Executor, ...）     |
+----------------------------------------------------+
|  rcl      … 言語非依存の C API                        |
|            （rcl_publish, rcl_node_init, ...）       |
+----------------------------------------------------+
|  rmw      … DDS を抽象化した C インタフェース           |
|            （rmw_publish, rmw_qos_profile_t, ...）   |
+----------------------------------------------------+
|  DDS 実装（Fast DDS / Cyclone DDS / ...）             |
+----------------------------------------------------+
```

`rclcpp::QoS` が `rmw_qos_profile_t` をラップしていること（`qos.hpp` が `rmw/qos_profiles.h` を include している）や、`rcl/wait.h` を使って Executor が 待つこと（`executor.hpp` の include）からも、この層構造がそのままヘッダの依存関係に 現れているのが分かります。

この見方を持っておくと、rclcpp の API に迷ったときに「rcl の対応する関数を C++ で ラップしただけのはずだ」という当たりがつけられます。逆に、rclcpp が分厚く感じる部分 （テンプレート、コールバックグループ、Executor まわり）は、C++ ならではの 「寿命管理」と「型安全性」のために足された層で、ROS の概念そのものではありません。

---

## 2. Node は神クラスではなく、インタフェースの束

> 要約: `rclcpp::Node` は「何でも屋」に見えますが、実体は `NodeBaseInterface` / `NodeTopicsInterface` / `NodeParametersInterface` などの 小さなインタフェースを 11 個束ねたファサードです。`create_publisher()` のような フリー関数はこれらのインタフェースだけを要求するので、`Node` を継承していない クラス（コンポーネントやライフサイクルノード）でも同じ仕組みが使い回せます。

`node.hpp` の `Node` クラスの private メンバを見ると、実データはこのクラス自身が 持つのではなく、各インタフェースへの `shared_ptr` として保持されています。

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp
rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_base_;
rclcpp::node_interfaces::NodeTopicsInterface::SharedPtr node_topics_;
rclcpp::node_interfaces::NodeServicesInterface::SharedPtr node_services_;
rclcpp::node_interfaces::NodeTimersInterface::SharedPtr node_timers_;
rclcpp::node_interfaces::NodeParametersInterface::SharedPtr node_parameters_;
rclcpp::node_interfaces::NodeClockInterface::SharedPtr node_clock_;
rclcpp::node_interfaces::NodeGraphInterface::SharedPtr node_graph_;
// ほか NodeLogging / NodeTimeSource / NodeTypeDescriptions / NodeWaitables
```

`/opt/ros/jazzy/include/rclcpp/rclcpp/node_interfaces/` には、これらが 1 ファイル 1 責務でずらりと並んでいます。役割は名前の通りで、代表的なものを挙げます。

| インタフェース | 責務 |
| --- | --- |
| `NodeBaseInterface` | ノード名・namespace・`rcl_node_t` の生ハンドル、コールバックグループの管理 |
| `NodeTopicsInterface` | Publisher / Subscription の生成と登録（`node_topics.hpp`） |
| `NodeServicesInterface` | Service / Client の生成と登録 |
| `NodeParametersInterface` | パラメータの宣言・取得・変更（`declare_parameter` などの実体） |
| `NodeGraphInterface` | `ros2 node info` 相当のグラフ情報 |
| `NodeWaitablesInterface` | Executor が直接待てる汎用エンティティ（`Waitable`）の登録 |

`node.hpp` の冒頭コメントは `Node` をこう紹介しています。

> Node is the single point of entry for creating publishers and subscribers.

「唯一の入口」であっても「実装の本体」ではありません。実際に `create_publisher()` の実装（`node_impl.hpp` / `create_publisher.hpp`）を辿ると、`Node` は自分では何も 作らず `node_topics_interface->create_publisher(...)` に丸投げしています。

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/create_publisher.hpp（要約）
auto node_topics_interface = rclcpp::node_interfaces::get_node_topics_interface(node_topics);
auto pub = node_topics_interface->create_publisher(topic_name, factory, actual_qos);
node_topics_interface->add_publisher(pub, options.callback_group);
return std::dynamic_pointer_cast<PublisherT>(pub);
```

着目したいのは、`rclcpp::create_publisher<MessageT>(...)`（`Node::create_publisher()` が内部で呼ぶフリー関数）が要求しているのは「`get_node_topics_interface()` という メソッドを持つ何か」だけだという点です。`node_topics.hpp` の `NodeTopics` クラスも、 実データを持たず `NodeBaseInterface*` と `NodeTimersInterface*` への生ポインタを 保持するだけの薄い実装です。

この「必要なインタフェースだけを要求する」設計が効いてくるのが、ライフサイクル ノード（`rclcpp_lifecycle::LifecycleNode`）やコンポーネントです。`rclcpp::Node` を 継承しなくても、同じ `NodeBaseInterface` 等を内部に持ってさえいれば `create_publisher()` / `create_subscription()` をそのまま使い回せます。 「`Node` を継承する」ことが唯一の正解ではなく、「必要なインタフェースを一式揃えた クラスを作る」ことが本質です（`node_interfaces/node_interfaces.hpp` の `NodeInterfaces<...>` は、まさに「必要な分だけ束ねる」ためのヘルパです）。 この分割の恩恵は課題 15（コンポーネント化）で直接効いてきます（9 章）。

---

## 3. すべては「作って、shared_ptr で持つ」

> 要約: `create_publisher()` などが返すのは `shared_ptr` です。しかも rclcpp 側 （コールバックグループ）はそのエンティティを `weak_ptr` でしか持ちません。 呼び出し側が返り値を握り続けない限り、そのオブジェクトは消えます。 「戻り値をメンバ変数に代入し忘れると動かない」という初学者最初のつまずきは、 事故ではなくこの所有権モデルの必然です。

`node.hpp` の宣言を見ると、`create_publisher` / `create_subscription` / `create_wall_timer` / `create_service` / `create_client` はすべて `std::shared_ptr<...>` を返します。ここまでは想像がつくと思いますが、重要なのは rclcpp の内部がこの Publisher を**強参照で持っていない**ことです。 `callback_group.hpp` の実装を見ると、登録された Publisher / Subscription / Timer / Service / Client は、すべて `weak_ptr` のベクタとして保持されています。

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/callback_group.hpp
for (auto & weak_ptr : vect_ptrs) {
  auto ref_ptr = weak_ptr.lock();
  ...
}
```

`add_publisher()` などのシグネチャは `SharedPtr` を引数に取ります（271〜287 行目）が、 内部では `weak_ptr` に変換して保持します。`create_publisher()` の戻り値をどこにも 保存しなければ、関数を抜けた瞬間に参照カウントが 0 になり、`weak_ptr::lock()` は 常に `nullptr` を返します。エラーにもならず、ただ「何も起きない」だけです。 課題 01 の README が「つまずきポイント」の 1 番目に挙げている

> `create_publisher()` / `create_wall_timer()` の戻り値は必ずメンバ変数に代入します。 ローカル変数で受けるとコンストラクタを抜けた時点で破棄され、何も起きません。

は、この weak_ptr 設計から論理的に導かれる結果です。「うっかりミス」ではなく 「所有権を持っていないものは消える」という、C++ として自然な振る舞いに過ぎません。

同じ構造はノードと Executor の関係にも現れます。`Executor::add_node()` は `std::shared_ptr<rclcpp::Node>` を引数に取りますが（`executor.hpp`）、実際に保持する `executor_entities_collector.hpp` の内部コレクションはこう定義されています。

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/executors/executor_entities_collector.hpp
using NodeCollection = std::set<
  rclcpp::node_interfaces::NodeBaseInterface::WeakPtr,
  std::owner_less<rclcpp::node_interfaces::NodeBaseInterface::WeakPtr>>;
NodeCollection weak_nodes_ RCPPUTILS_TSA_GUARDED_BY(mutex_);
```

Executor はノードを**所有しません**。ノードを生かし続けるのは `main()` の ローカル変数や、テストヘルパの `std::vector<rclcpp::Node::SharedPtr>` の責任です。 所有権の連鎖をまとめると次のようになります。

```
main() / テストコード
   │  shared_ptr（強参照・唯一の所有者）
   ▼
rclcpp::Node（あなたのノード）
   │  shared_ptr（NodeTopicsInterface 経由でエンティティを作る）
   ▼
Publisher<T> / Subscription<T> / TimerBase / ...
   ▲
   │  weak_ptr（生死の確認だけ。所有はしない）
CallbackGroup ◀───── weak_ptr ───── Executor
```

「強く握っている」矢印は常に上（あなたのコード）から下（rclcpp のエンティティ） へしか伸びていません。rclcpp 自身は自分が作ったものを弱くしか握らない設計です。 これはおそらく、循環参照（Node が Publisher を持ち、Publisher のコールバックが Node を参照し…）を防ぐための、shared_ptr を使う上での定石でもあります。

---

## 4. 待つのは Executor の仕事

> 要約: Publisher や Subscription 自身はメッセージを「待つ」処理を持ちません。 待機は Executor の役目で、rcl の wait set にエンティティを登録し、`rcl_wait` でブロックし、準備ができたものだけを取り出して実行する、という流れです。 `spin` 系関数は「何を」「どれだけ」処理するかが少しずつ違います。

ROS 2 のメッセージは rclcpp 側にキューとして溜まるのではなく、ミドルウェア（DDS） 側に残ります。公式ドキュメント（`Concepts/Intermediate/About-Executors.rst`）は これを次のように説明しています。

> Rather than queuing messages at the client library layer, messages remain in the middleware until processing. An Executor uses a "wait set" mechanism with binary flags per queue to detect available messages and expired timers.

Executor の役目は、この「まだ来ていないかもしれないもの」を効率よく待って来たら 実行することです。`executor.hpp` の protected メンバには、この流れがそのまま 関数として並んでいます。

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/executor.hpp
void collect_entities();                       // ① 待つべきエンティティを集める
void wait_for_work(std::chrono::nanoseconds);  // ② wait set を組み立て rcl_wait でブロック
bool get_next_ready_executable(AnyExecutable &); // ③ 準備完了のものを 1 つ取り出す
```

`wait_for_work()` のコメントには「Builds a set of waitable entities, which are passed to the middleware. After building wait set, waits on middleware to notify.」とあります。実体は `rcl/wait.h`（`/opt/ros/jazzy/include/rcl/rcl/wait.h`） の `rcl_wait_set_t` で、DDS の下から「準備できている／いない」という二値のフラグを 受け取る仕組みです。`spin()` はこの ①→②→③→実行 のループを回し続けているだけです。

`spin` にはいくつか亜種があり、「どこまで処理するか」が異なります （`executor.hpp` のドキュメントコメントより）。

| 関数 | 何をするか | ブロックするか |
| --- | --- | --- |
| `spin()` | 無限ループで ①〜③ を繰り返す | する（`cancel()` まで） |
| `spin_once(timeout)` | **準備できたものを 1 個だけ**実行する | `timeout` 分 |
| `spin_some(max_duration)` | 既に準備できているものを全部実行するが、実行中に新しく準備できたものは拾わない | 基本しない |
| `spin_all(max_duration)` | `max_duration` が尽きるまで「集めては実行」を繰り返す（実行中に増えた分も拾う） | `max_duration` 分 |
| `spin_until_future_complete(future, timeout)` | `spin_once` 相当を繰り返しながら future の完了を待つ | `timeout` 分 |

`spin_once` の「1 個だけ」という制約は、複数の Subscription やタイマーを 1 つの Executor に載せているときに問題になります。この練習帳の共通テストヘルパ `tools/drill_harness.hpp` の `spin_until()` は、次のコメントを付けて `spin_all` を 選んでいます。

```cpp
// tools/drill_harness.hpp
// spin_once は 1 回に 1 つしか処理しないため、購読が複数ある構成では
// 先に登録された側だけが実行され続けてもう一方が飢餓状態になる。
// spin_all で「今処理できる分を全部」処理する。
exec.spin_all(20ms);
```

受講者のノードのタイマーとテスト用 probe ノードの Subscription を同じ `SingleThreadedExecutor` に載せた場合、`spin_once` を回し続けると片方が優先され 続けてもう片方がなかなか実行されない「飢餓」が起こり得ます。`spin_all` なら、 その周ですでに準備できているものを取りこぼさず処理してから戻るため、複数のノードを 1 つの Executor に載せて両方の振る舞いを検証したいテストに向いています。

なお `spin_until_future_complete()` は便利な反面、コールバックグループの制約と 衝突しやすい関数です（5 章、課題 13 で扱います）。

---

## 5. コールバックグループ — 並行実行の単位

> 要約: コールバックグループは「同時に実行してよいコールバックの単位」です。 既定の `MutuallyExclusive` は同じグループ内で 1 つずつしか実行しないため 安全ですが、グループ内で「別のコールバックの完了を待つ」構成を組むと デッドロックします。`Reentrant` は並行実行を許す代わりに、スレッドセーフの 責任が書き手に移ります。

すべてのコールバック（Subscription、Timer、Service、Client の完了処理など）は どこかの `CallbackGroup` に属しています。`callback_group.hpp` はグループの種類を こう定義しています。

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/callback_group.hpp
enum class CallbackGroupType { MutuallyExclusive, Reentrant };
```

- **MutuallyExclusive**（既定）: 同じグループのコールバックは、Executor が何本 スレッドを持っていても同時には 1 つしか実行されません。
- **Reentrant**: 同じグループの複数のコールバック（自分自身との並行実行を含む）が 複数スレッドで同時に実行されえます。

`MutuallyExclusive` は「同じノードの状態を複数のコールバックが同時に触って壊す」 ことを防ぐ安全策ですが、この安全策自体がデッドロックの原因になることがあります。 課題 13（`exercises/13_executors/README.md`）はこれを実演する課題です。

サブスクライバのコールバックの中で、同じグループのサービスクライアントの応答を 同期的に待つと（`future.wait_for(2s)`）、次の循環が生じます。

- `trigger_callback` は「クライアントの完了処理」が実行されて `future` が 埋まるのを待っている
- 「クライアントの完了処理」は `trigger_callback` と同じ `MutuallyExclusive` グループに属しているため、`trigger_callback` の実行が終わるまで Executor に実行してもらえない

`SingleThreadedExecutor` はもちろん、グループを分けていない限り `MultiThreadedExecutor` でスレッド数を増やしても解決しません。「別グループなら 同時に実行してよい」という情報を Executor に与えていないからです。解決策は、 サブスクライバとクライアントを**別の** `MutuallyExclusive` グループに分け、 かつ `MultiThreadedExecutor` に複数コールバックを同時処理できる余力を持たせる ことです。両方が無関係な処理である以上、`Reentrant` にする必要はなく、 それぞれのグループ内で排他されていれば十分です。

Executor の実装は次の 3 種類を押さえておくと見通しが良くなります （`executors/` には他に実験的な `events_cbg_executor` もあります）。

| Executor | スレッド数 | 特徴 |
| --- | --- | --- |
| `SingleThreadedExecutor` | 1 | `rclcpp::spin()` が使うもの。01〜07 はこれで足ります。グループを分けても同時実行はできません。 |
| `MultiThreadedExecutor` | 複数（既定 0 = CPU コア数、最低 2） | グループが分かれていれば別スレッドで同時実行できます。課題 13 で使うのはこれです。 |
| `StaticSingleThreadedExecutor` | 1 | エンティティ一覧を spin 開始時に一度だけ構築（オーバーヘッド減）。スレッドは 1 本のままなので課題 13 の解決にはなりません。 |

Jazzy にはこの他に `rclcpp::experimental::executors::EventsExecutor` （`experimental/executors/events_executor/events_executor.hpp`）もあります。 `experimental` 名前空間にある通りまだ安定版の API ではなく、wait set を毎回 組み立て直す代わりに rmw 層からのイベント通知を直接受け取る設計だとヘッダの コメントから読み取れますが、本書では紹介に留めます。

---

## 6. メッセージの所有権とゼロコピー

> 要約: `publish()` には値を渡す版と `unique_ptr` を渡す版があり、前者は必ず 1 回コピーが発生し、後者は条件が揃えばコピー 0 回で届きます。この橋渡しを しているのが `IntraProcessManager` です。DDS を経由しない分だけ速く、 大きなメッセージほど効果があります。

`publisher.hpp` の `publish()` オーバーロードにこの違いがはっきり現れています。

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/publisher.hpp（要約）
// unique_ptr 版：所有権をそのまま受け取る
void publish(std::unique_ptr<T, ROSMessageTypeDeleter> msg)
{
  if (!intra_process_is_enabled_) { this->do_inter_process_publish(*msg); return; }
  this->do_intra_process_publish(std::move(msg));  // コピーせずそのまま渡す
}

// 値渡し版：まず複製してから unique_ptr 版に委譲する
void publish(const T & msg)
{
  if (!intra_process_is_enabled_) { this->do_inter_process_publish(msg); return; }
  // Otherwise we have to allocate memory in a unique_ptr and pass it along.
  auto unique_msg = this->duplicate_ros_message_as_unique_ptr(msg);
  this->publish(std::move(unique_msg));
}
```

値渡し版は「メモリを確保して複製してから」unique_ptr 版に委譲します。呼び出し側の `msg` はローカル変数として生き続けるので、コピーせずに済ませる方法がありません。 一方 unique_ptr 版は `std::move` で所有権そのものを受け取るので、そのメモリを そのまま中間バッファに保管できます。

この「中間バッファ」の実体が `rclcpp::experimental::IntraProcessManager` （`experimental/intra_process_manager.hpp`）です。ヘッダのコメントによると、

> A singleton instance of this class is owned by a rclcpp::Context and a rclcpp::Node can use an associated Context to get an instance of this class. Nodes which do not have a common Context will not exchange intra process messages because they do not share access to the same instance of this class.

`IntraProcessManager` は **`Context` ごと（多くの場合プロセスごと）に 1 つ**の シングルトンで、Publisher と Subscription はここに登録されることで互いを 見つけます。別プロセスのノードとは `Context` を共有しないので、そもそも プロセス内通信の対象になり得ません。

ゼロコピーが成立する条件を、課題 14（`exercises/14_zero_copy/README.md`）の 整理に沿ってまとめます。

**効く条件（すべて満たす必要がある）**

- Publisher と Subscription が同一プロセス内にある
- 両方のノードが `rclcpp::NodeOptions().use_intra_process_comms(true)` で作られている
  （**既定は `false`**。`node_options.hpp` の `bool use_intra_process_comms_ {false};}`）
- QoS の History が `KeepLast` / `KeepAll`（depth を持つ形）である
- publish 側が `std::move(unique_ptr)` で所有権を渡している

**効かない条件（コピーに戻る）**

- Publisher と Subscription が別プロセスにある（DDS 越しでシリアライズが必要）
- `use_intra_process_comms` を有効にしていない。**同一プロセスに載せるだけでは効かない**
  （コンポーネント化しても自動では有効にならない。9 章を参照）
- publish に値を渡している（`publish(message)`）
- 複数の購読者がいて、一部が書き換え可能な形（`UniquePtr` / `SharedPtr`）で受け取る
  （単独所有を満たすためにその分のコピーが作られる）

購読側の引数を `const T &` にしてもコピーは発生しません（Jazzy で実測。送信側と受信側で
メッセージのアドレスが一致します）。`any_subscription_callback.hpp` は値で受け取る形を
そもそもサポートしていないので、「値で受けるとコピーになる」という書き方は不正確です。
型は**メッセージをどう扱いたいか**（その場で読むだけ / 保持する / 書き換える）で選びます。

大きなメッセージ（`sensor_msgs::msg::Image` や `PointCloud2` など数百 KB〜数 MB に なるもの）ほどこの差が効きます。値渡しは毎回全体をコピーするため段数が増えるほど コピー回数が線形に増えますが、`std::move(unique_ptr)` ならコピーは 0 回のままです。 `/opt/ros/jazzy/include/intra_process_demo/image_pipeline/camera_node.hpp` の `CameraNode` はこの手法でカメラ画像を publish しています。

もう 1 つ、`publisher.hpp` には `borrow_loaned_message()` という API もあります （`loaned_message.hpp` の `LoanedMessage<T>` を返します）。RMW 実装がメッセージ用の メモリを直接貸し出せる場合にヒープ確保そのものを飛ばす仕組みで、`LoanedMessage` の コメントによれば「ミドルウェアが貸し出しに対応していればそれを使い、対応していなけ れば渡されたアロケータで普通に確保する」というフォールバック付きです。対応の有無は `PublisherBase::can_loan_messages()`（`publisher_base.hpp`）で確認できます。 プロセス内通信のゼロコピーとは別の、DDS 実装レベルでのゼロコピーの仕組みです。

---

## 7. QoS は契約

> 要約: QoS（Quality of Service）は DDS 由来の「通信の品質に関する約束事」で、 Publisher と Subscription の QoS が噛み合わないと接続が確立しません。 `rclcpp::QoS` は「要求」と「提供」という非対称な関係で互換性を判定します。

ROS 2 のトピック通信が繋がらないとき、原因の多くは QoS の不一致です。これはバグ というより設計そのもので、公式ドキュメント （`Concepts/Intermediate/About-Quality-of-Service-Settings.rst`）は Subscription と Publisher の関係をこう説明しています。

> Subscriptions request a QoS profile that is the "minimum quality" that it is willing to accept, and publishers offer a QoS profile that is the "maximum quality" that it is able to provide.

Subscription は「これより悪い品質は受け付けない」という最低ラインを**要求**し、 Publisher は「これだけの品質なら出せる」という上限を**提供**します。接続が成立 するのは、要求された全ポリシーについて提供側が要求側以上に厳しい場合だけです。

主要なポリシーは 7 つあります（同ドキュメントより）。

| ポリシー | 選択肢 | 意味 |
| --- | --- | --- |
| History | KeepLast(N) / KeepAll | 直近 N 件だけ保持するか、全件保持するか |
| Depth | 数値 | History が KeepLast のときのキューの深さ |
| Reliability | Reliable / BestEffort | 再送してでも届けるか、落としてもよいか |
| Durability | TransientLocal / Volatile | 過去のサンプルを後から来た Subscription のために保持するか |
| Deadline | 時間 | 連続する publish の間隔の上限 |
| Lifespan | 時間 | publish から受信までにメッセージが有効な時間 |
| Liveliness | Automatic / ManualByTopic | 生存確認を自動でするか、明示的なアサートを要求するか |

`rclcpp/qos.hpp` ではこれらを `QoS` クラスのメソッドチェーンとして設定します （課題 12 の `qos_nodes.cpp` より）。

```cpp
// exercises/12_qos/src/qos_nodes.cpp
rclcpp::QoS qos(rclcpp::KeepLast(1));
qos.transient_local();
qos.reliable();
```

互換性のルールは「厳しい方が緩い方を包含できるか」で考えると素直です。

- **Reliability**: Reliable な Subscription は BestEffort な Publisher とは 繋がりません。逆に BestEffort な Subscription は Reliable な Publisher と繋がります。
- **Durability**: TransientLocal な Publisher は Volatile な Subscription を 満たせますが、逆（Volatile な Publisher が TransientLocal な要求を満たす）は できません。
- **Deadline / Liveliness lease**: Subscription 側の値が Publisher 側の値以上に 緩ければ互換です。
- **Liveliness**: ManualByTopic を要求する Subscription は Automatic な Publisher とは繋がりません。

課題 12 の `LatchedPublisher` / `LatchedSubscriber` が `KeepLast(1) + transient_local + reliable` を使っているのは、いわゆる「ラッチ（latched）トピック」パターンです。 TransientLocal な Publisher は最後に publish した値を保持しているため、 Subscription が後から起動しても最新の設定値を受け取れます。

`rclcpp::QoS` にはよく使う組み合わせを名前付きにした既製プロファイルもあり、 `qos.hpp` に実際にコメント付きで定義されています。

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/qos.hpp
/**
 * Sensor Data QoS class
 *    - History: Keep last, Depth: 5,
 *    - Reliability: Best effort, Durability: Volatile, ...
 */
class RCLCPP_PUBLIC SensorDataQoS : public QoS { ... };
```

`SensorDataQoS` は depth 5 の BestEffort で、カメラやレーザーのように「多少 取りこぼしても最新のデータが大事」なトピック向けです。同様に `ParametersQoS` （depth 1000、Reliable）や `ServicesQoS`（depth 10、Reliable）もあり、既定値は `rmw/qos_profiles.h` に定数として定義されています。QoS を 1 から組む前に、近い 既製プロファイルがないか見るとよいでしょう。

なお `rclcpp::qos_check_compatible(pub_qos, sub_qos)`（`qos.hpp`）を使うと、実際に 繋げる前にコードから互換性を確認できます。戻り値は `Ok` / `Warning` / `Error` の 3 段階で、「システムデフォルト」など実行時まで確定しない値が混ざる場合は `Warning` になるとヘッダに明記されています。

---

## 8. パラメータは「宣言してから使う」

> 要約: rclcpp のパラメータは `declare_parameter()` で型と既定値を宣言してからで ないと `get_parameter()` できません。実行時に想定外の型・想定外のパラメータが 紛れ込む事故を防ぐための制約です。受け取り方にはポーリング（毎回読む）と イベント通知（変わった時だけ呼ばれる）の 2 通りがあります。

`node.hpp` の `get_parameter()` 系メソッドのドキュメントコメントには繰り返し `ParameterNotDeclaredException` が登場します。宣言していないパラメータを読もうと すると、既定では例外が飛ぶということです。公式ドキュメント （`Concepts/Basic/About-Parameters.rst`）はこう説明しています。

> a node needs to declare all of the parameters that it will accept during its lifetime.

宣言時には型・既定値・`ParameterDescriptor`（説明文などのメタデータ）を指定し、 既定では宣言済みパラメータの型を実行時に変えることもできません。これにより 「パラメータ名のタイプミス」「文字列のつもりが数値で上書きされた」といった事故を 実行時エラーとして早期に検出できます。

課題 06（`exercises/06_parameters/README.md`）の `MinimalParam` はこのパターンの 最も基本的な形で、タイマのコールバックの**中で毎回** `get_parameter()` を呼び直し ます。一度読んでキャッシュするのではなく、使うたびに読み直すのが公式チュートリアル の主旨です。これが「ポーリング」方式で、実装は単純ですが変化がなくても毎回読みに 行くコストがあります。

もう 1 つの方式が課題 07（`exercises/07_param_events/README.md`）の `ParameterEventHandler` を使う「イベント通知」方式です。`parameter_event_handler.hpp` を見ると、このクラスは内部で `/parameter_events` トピックを購読しています。 `add_parameter_callback("an_int_param", cb)` で登録したコールバックは、実際に そのパラメータが変更された時だけ呼ばれます。ヘッダのコメントは次の注意を明記して います。

> Note: the object returned from add_parameter_callback must be captured or the callback will [be unregistered].

戻り値（`ParameterCallbackHandle::SharedPtr`）を変数に保持しておかないと、登録した 直後にコールバックが解除されます。3 章で説明した「戻り値を shared_ptr で持たないと 消える」設計がここにも表れている例で、課題 07 の README が「今回いちばんの ハマりどころ」として挙げているのはこの点です。

どちらを使うべきかの目安は次の通りです。

- **ポーリング（課題 06）** — もともと一定周期で処理するループがあり、そのついでに 最新の値を使いたいだけの場合。実装が単純で、寿命管理も不要。
- **イベント通知（課題 07）** — パラメータが変わった瞬間に何かを実行したい場合。 変化がなければコールバックは呼ばれず無駄な読み出しがない一方、 `ParameterEventHandler` と `ParameterCallbackHandle` を生存させ続ける責任が増える。

`/parameter_events` はグローバルなトピックなので、あるノードのパラメータ変更を 別のノードが監視することもできます。`ParameterEventHandler` がノード単位ではなく トピック購読として実装されているのは、この汎用性のためだと読めます。

---

## 9. 1 プロセス 1 ノードをやめる

> 要約: `RCLCPP_COMPONENTS_REGISTER_NODE` で登録したノードは、`ros2 run` で単独 プロセス起動する代わりに、コンポーネントコンテナに動的ロードして他のノードと 同じプロセスで動かせます。これが成立するために、ノードのコンストラクタは `rclcpp::NodeOptions` を受け取る作法が要求されます。

課題 15（`composable_talker.cpp`）は次のような形をしています。

```cpp
// exercises/15_composition/src/composable_talker.cpp
ComposableTalker::ComposableTalker(const rclcpp::NodeOptions & options)
: Node("composable_talker", options), count_(0) { /* ... */ }

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(ComposableTalker)
```

`RCLCPP_COMPONENTS_REGISTER_NODE` の定義 （`rclcpp_components/rclcpp_components/register_node_macro.hpp`）には、登録できる クラスの条件がコメントで明記されています。

> Valid arguments for NodeClass shall have a constructor that takes a single argument that is a `rclcpp::NodeOptions` instance, and a method of the signature `rclcpp::node_interfaces::NodeBaseInterface::SharedPtr get_node_base_interface`. Note: NodeClass does not need to inherit from `rclcpp::Node`, but it is the easiest way.

ここでも 2 章の「Node は必要なインタフェースを揃えたファサードに過ぎない」という 設計が効いています。コンポーネントとして登録するために `rclcpp::Node` を継承する **必要はなく**、`get_node_base_interface()` さえ持っていればよいとコメントが明言 しています。とはいえ実務上最も簡単な方法は継承なので、ほとんどのコンポーネントは この課題と同じ形になります。

なぜコンストラクタが `NodeOptions` を要求するのでしょうか。コンポーネント コンテナ（`rclcpp_components::ComponentManager`）は実行時に共有ライブラリを `dlopen` し、`NodeFactoryTemplate<NodeClass>` 経由でインスタンスを生成します。 このとき渡せる引数は `NodeOptions` 1 つだけで、そこにノードに必要な設定が すべて詰め込まれています。`node_options.hpp` の既定値コメントを見ると、次のような 項目が運ばれることが分かります。

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/node_options.hpp（既定値のコメントより抜粋）
//   - parameter_overrides = {}
//   - use_intra_process_comms = false
//   - automatically_declare_parameters_from_overrides = false
```

具体的には、コマンドライン引数（`arguments()`）、パラメータの上書き （`parameter_overrides()`）、そして課題 14 で使った `use_intra_process_comms()` が ここに含まれます。「複数のノードを同じプロセスに詰め込む」というコンポーネントの 目的そのものが `use_intra_process_comms(true)` によるゼロコピー通信と特に相性が よいのは偶然ではありません。同じプロセスに載るノード同士なら、6 章の `IntraProcessManager` がそのまま使えるからです。

ただし **コンポーネント化すればゼロコピーになる、わけではありません。** 上の既定値
コメントのとおり `use_intra_process_comms` は `false` のままなので、明示的に有効に
しない限り、同一プロセスに載っていても通信は DDS を経由します（ローカルでの実測でも、
同一プロセス・既定オプションでは送信側と受信側でメッセージのアドレスが一致しません）。
公式の `composition_demo_launch.py` も有効化していないため、あの launch で起動した
talker / listener は同一プロセスにいながら DDS 経由で通信しています。有効にするには、

- CLI: `ros2 component load /ComponentManager <pkg> <plugin> -e use_intra_process_comms:=true`
- launch: `ComposableNode(..., extra_arguments=[{'use_intra_process_comms': True}])`

のように、コンテナに読み込ませる時点で `NodeOptions` に流し込む必要があります。
つまりコンポーネント化は「同一プロセスに載せる」という**前提条件を満たすだけ**で、
ゼロコピーそのものは別途オプトインする、という関係です。

コンストラクタで受け取った `NodeOptions` を `Node("...", options)` に**そのまま** 渡す作法を崩すと、`ros2 run` の単体起動では問題ないのに、コンポーネントコンテナに 載せた途端 `use_intra_process_comms` やパラメータの上書きが effective にならない、 という分かりにくい不具合になります。課題 14 の README が「`Node("...")` と書いて しまうと `use_intra_process_comms` が伝わらず、常にコピーが発生してテストが落ちま す」と注意しているのは、この経路の必然性を踏まえたものです。

---

## 10. rclcpp を効率的に使うための指針

> 要約: ここまでの構造を踏まえた、実務で効く経験則です。理由も添えているので、 「なぜそうすべきか」を構造から思い出せるようにしてあります。

1. **QoS は設計の最初に決める。** あとから片方だけ変えると 7 章の互換性ルールで 静かに繋がらなくなります。センサ系は `SensorDataQoS`、設定配布は `transient_local`、と用途で先に決めます。
2. **コールバックの中に重い処理・ブロッキング処理を置かない。** 4 章の通り、 1 つのコールバックが長引くとその Executor（同じグループ、あるいは `SingleThreadedExecutor` なら全部）が止まります。
3. **コールバック内で他の完了を同期的に待つなら、コールバックグループを設計する。** 5 章のデッドロックは「同じグループで待ち合わせる」ときに起きます。待つ側と 待たれる側は別グループに分け、`MultiThreadedExecutor` にスレッドの余力を 持たせます。
4. **コールバックの中から `spin_until_future_complete()` を呼ばない。** すでに Executor の中で動いているコールバックから同じノードに対して spin を始めようと すると例外またはデッドロックになります。`future.wait_for()` で待つか設計を 見直します。
5. **大きなメッセージはゼロコピーが効く形で書く。** `publish(msg)` ではなく `publish(std::make_unique<T>(...))`、購読側は `ConstSharedPtr` で受けます （6 章）。
6. **購読コールバックの引数は `msg::X::ConstSharedPtr` を基本形にする。** プロセス内通信が有効なときにゼロコピーの恩恵をそのまま受けられ、無効なときも コストは増えません。
7. **`create_publisher()` などの戻り値は必ずメンバ変数に保持する。** 3 章の通り rclcpp 側は weak_ptr でしか持っていないので、持たなければ消えます。
8. **`rclcpp::Rate` より `create_wall_timer()` を優先する。** `Rate::sleep()` は 呼び出したスレッドをブロックします。Executor のスレッドで使うと他のコールバック が処理できなくなりますが、`create_wall_timer()` は Executor の待ち合わせに 乗るので共存できます。
9. **高頻度ログには `RCLCPP_INFO_THROTTLE` などを使う。** `logging.hpp` の `_THROTTLE` 系マクロは指定間隔でしかログを出さず、ログ出力自体が処理を 圧迫するのを防げます。
10. **パラメータは「宣言してから使う」を徹底し、型を明示する。** 8 章の通り、 未宣言のパラメータへのアクセスは例外になります。
11. **パラメータの変化に反応したいのか、周期処理のついでに読みたいのかを区別する。** 前者なら `ParameterEventHandler`、後者なら毎回の `get_parameter()` で十分です。
12. **コンポーネント化を見越すなら最初から `NodeOptions` を受け取るコンストラクタ で書く。** あとから差し込むのは 9 章の通り面倒です。
13. **Executor の種類は「並行実行が必要かどうか」で選ぶ。** 不要なら `SingleThreadedExecutor`（`rclcpp::spin()`）で十分です。
14. **`spin_once` を「1 回で全部処理してくれるもの」だと思わない。** 4 章の通り 1 個だけ処理します。複数エンティティを確実に処理したいなら `spin_some` / `spin_all` を検討します（`tools/drill_harness.hpp` の選択も参照）。
15. **迷ったらヘッダを読む。** rclcpp のヘッダは Doxygen コメントが充実しており、 「この関数は何を呼んでいるか」まで書かれていることが多いです。

---

## 11. もっと深く知るには

> 要約: この文書で引用した一次情報の場所と、練習帳の課題との対応表です。 実装を疑ったら、まずここに挙げたヘッダやドキュメントに当たってください。

### ヘッダ（`/opt/ros/jazzy/include/` 以下）

| トピック | 主なファイル |
| --- | --- |
| Node とインタフェース分割 | `rclcpp/rclcpp/node.hpp`, `rclcpp/rclcpp/node_interfaces/*.hpp` |
| Publisher / Subscription 生成 | `rclcpp/rclcpp/create_publisher.hpp`, `create_subscription.hpp`, `node_interfaces/node_topics.hpp` |
| コールバックグループ | `rclcpp/rclcpp/callback_group.hpp` |
| Executor 全般 | `rclcpp/rclcpp/executor.hpp`, `executors/single_threaded_executor.hpp`, `executors/multi_threaded_executor.hpp`, `executors/static_single_threaded_executor.hpp` |
| 実験的な EventsExecutor | `rclcpp/rclcpp/experimental/executors/events_executor/events_executor.hpp` |
| Publisher の publish 実装 | `rclcpp/rclcpp/publisher.hpp`, `publisher_base.hpp` |
| ゼロコピー / プロセス内通信 | `rclcpp/rclcpp/experimental/intra_process_manager.hpp`, `loaned_message.hpp` |
| QoS | `rclcpp/rclcpp/qos.hpp`, `rmw/rmw/qos_profiles.h` |
| パラメータ | `rclcpp/rclcpp/node_interfaces/node_parameters_interface.hpp`, `parameter_event_handler.hpp` |
| コンポーネント | `rclcpp_components/rclcpp_components/register_node_macro.hpp`, `node_options.hpp` |
| rcl（C 層） | `rcl/rcl/node.h`, `rcl/rcl/wait.h` |

### 公式ドキュメント・設計文書

- Concepts / Basic / About-Client-Libraries — rclcpp・rclpy・rcl の関係 （`https://docs.ros.org/en/jazzy/Concepts/Basic/About-Client-Libraries.html`）
- Concepts / Intermediate / About-Executors — Executor と wait set （`https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Executors.html`）
- Concepts / Intermediate / About-Quality-of-Service-Settings — QoS 一覧と互換性 （`https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Quality-of-Service-Settings.html`）
- Concepts / Basic / About-Parameters — パラメータの宣言・サービス （`https://docs.ros.org/en/jazzy/Concepts/Basic/About-Parameters.html`）
- Concepts / Basic / About-Discovery — DDS による自動検出 （`https://docs.ros.org/en/jazzy/Concepts/Basic/About-Discovery.html`）
- Tutorials / Demos / Setting up efficient intra-process communication — プロセス内通信のチュートリアル（Jazzy では「Concepts」ではなく 「Tutorials/Demos」配下に置かれています） （`https://docs.ros.org/en/jazzy/Tutorials/Demos/Intra-Process-Communication.html`）
- design.ros2.org / articles / intraprocess_communications — IntraProcessManager 設計の背景（提案当時の記述であり、実装の細部は Jazzy のヘッダの方が正確です） （`https://design.ros2.org/articles/intraprocess_communications.html`）
- How-To-Guides / Using callback groups （`https://docs.ros.org/en/jazzy/How-To-Guides/Using-callback-groups.html`）

`docs.ros.org` が閲覧できない環境では、上記ページの `.rst` ソースを `https://raw.githubusercontent.com/ros2/ros2_documentation/jazzy/source/<パス>.rst` の形で取得できます（本書もその方法で内容を確認しています）。

### 課題との対応表

| 章 | 対応する課題 |
| --- | --- |
| 1. なぜ rclcpp は薄いのか | 全課題の前提知識 |
| 2. Node はインタフェースの束 | 15（コンポーネント化） |
| 3. 作って shared_ptr で持つ | 01, 02, 05, 07（戻り値の保持） |
| 4. 待つのは Executor の仕事 | 全課題（テストヘルパの `spin_all` の理由） |
| 5. コールバックグループ | 13 |
| 6. ゼロコピー | 14 |
| 7. QoS は契約 | 12 |
| 8. パラメータの宣言と監視 | 06, 07 |
| 9. コンポーネント化 | 15 |

`ros2/rclcpp` のソースリポジトリ（`https://github.com/ros2/rclcpp`）を Jazzy タグで チェックアウトすれば、ここで引用したヘッダの `.cpp` 側の実装（`node_topics.cpp` や `intra_process_manager.cpp` など）も読めます。本書はヘッダのドキュメントコメントと 宣言から読み取れる範囲に留めており、実装本体（`.cpp`）までは踏み込んでいません。 挙動をさらに正確に追いたい場合はそちらも参照してください。
