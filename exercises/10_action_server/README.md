# 課題 10: アクションサーバを作る 〔中級〕

公式チュートリアル
[Writing an action server and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Writing-an-Action-Server-Client/Cpp.html)
のサーバ側をそのまま書きます。

## トピック / サービス / アクションの使い分け

トピックは「垂れ流し」、サービスは「一発リクエスト・一発応答」です。アクションは
時間のかかる処理（数秒〜数分）を頼みたいときに使います。目標（Goal）を送ると
途中経過（Feedback）が何度も届き、最後に結果（Result）が返ります。おまけに
キャンセルもできます。今回の Fibonacci はまさにその練習台です。

## やること

`src/fibonacci_action_server.cpp` の TODO を埋めてください。仕様は公式チュートリアルと同一です。

| 項目 | 値 |
| --- | --- |
| ノード名 | `fibonacci_action_server` |
| アクション名 | `fibonacci` |
| 型 | `action_tutorials_interfaces::action::Fibonacci` |
| 目標受理 | `handle_goal` で `ACCEPT_AND_EXECUTE` を返す |
| キャンセル受理 | `handle_cancel` で `ACCEPT` を返す |
| 実行の開始 | `handle_accepted` で別スレッドに `execute` を投げて `detach()` |
| 計算 | `sequence` を `{0, 1}` で始め、`i = 1` から `order - 1` まで `sequence[i] + sequence[i-1]` を `push_back` しながら `publish_feedback` |

`Fibonacci` の中身は次で確認できます。

```
$ ros2 interface show action_tutorials_interfaces/action/Fibonacci
int32 order
---
int32[] sequence
---
int32[] partial_sequence
```

`---` の上から順に、目標（`Fibonacci::Goal`）、結果（`Fibonacci::Result`）、
途中経過（`Fibonacci::Feedback`）です。サービスの `Request`/`Response` が
2 段だったのに対し、アクションは 3 段あります。

クラス宣言（`include/drill/fibonacci_action_server.hpp`）は与えてあります。4 つの
コールバック（`handle_goal` / `handle_cancel` / `handle_accepted` / `execute`）を
埋めることと、コンストラクタで `rclcpp_action::create_server` の戻り値を
`action_server_` に入れることを考えてください。

### 公式との違い

公式チュートリアルとの違いは次の 2 点だけです。ロジック（コールバックの中身、
ログの文言、結果の中身）は公式と完全に同一です。

- `main()` を別ファイル（`src/fibonacci_action_server_main.cpp`）に分けている点
  （テストからクラスを直接生成して検証できるようにするため）。
- `execute()` のループ周期を、公式の 1 秒（`rclcpp::Rate loop_rate(1)`）から
  **20ms** に変えている点。公式どおり 1 秒にすると `order` が大きいテストが
  遅くなりすぎるための措置です。

## 動かしてみる

テストが通ったら、公式チュートリアルと同じように手で動かせます。

```bash
source install/setup.bash
ros2 run drill_10_action_server fibonacci_action_server
```

別の端末で:

```bash
ros2 action list                                                          # /fibonacci が見えるか
ros2 action info /fibonacci -t
ros2 action send_goal /fibonacci action_tutorials_interfaces/action/Fibonacci "{order: 5}" --feedback
```

`--feedback` を付けると、サーバ側の端末に `Publish feedback` のログが、
クライアント側の端末に途中経過（`partial_sequence`）が何度も表示され、
最後に `Result: sequence=[0, 1, 1, 2, 3, 5]` のような結果が出れば成功です。

`Ctrl-C` で送信を打ち切るとキャンセル要求になり、サーバ側に
`Received request to cancel goal` / `Goal canceled` のログが出ます。

## つまずきポイント

- `create_server()` の戻り値は必ず `action_server_` に代入します。ローカル変数で
  受けるとコンストラクタを抜けた時点で破棄され、アクションが消えます
  （01 の Publisher/Timer、03 の Service と同じ罠）。
- `handle_accepted` は「すぐ返る」ことが大事です。ここで `execute` を直接呼ぶと
  Executor が長時間ブロックされ、他のコールバック（キャンセル要求など）を
  一切処理できなくなります。公式どおり `std::thread{...}.detach()` で
  別スレッドに逃がしてください。
- `execute` の中の `is_canceling()` チェックは、`publish_feedback` より**前**に
  置きます（公式の順序どおり）。後ろに置くとキャンセル直後の feedback が
  1 回多く飛んでしまいます。
- `sequence.push_back(sequence[i] + sequence[i - 1])` の添字はループ変数 `i` を
  そのまま使います。`sequence` は `{0, 1}` から始まっているので、`i = 1` の時点で
  `sequence[1] + sequence[0]` を計算することになります。
- `goal->order` は `int32_t` です。ループの条件は `i < goal->order` であり、
  `<=` ではありません。

## テスト

```bash
./drill run 10
```

| テスト | 見ているところ |
| --- | --- |
| `fibonacciアクションサーバを公開している` | `create_server` を `action_server_` に入れているか |
| `order5の目標を送るとフィボナッチ数列を返す` | 計算ロジックと `succeed(result)` |
| `実行中にfeedbackが1回以上届く` | `publish_feedback` を呼んでいるか |
| `キャンセル要求を受理する` | `handle_cancel` と `is_canceling()` / `canceled(result)` |

## 参考

- 公式: [Writing an action server and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Writing-an-Action-Server-Client/Cpp.html)
- 公式: [Creating an action](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Creating-an-Action.html)
- ローカルの実装例: `/opt/ros/jazzy/share/action_tutorials_cpp/`
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
