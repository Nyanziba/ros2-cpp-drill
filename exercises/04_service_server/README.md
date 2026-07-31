# 課題 04: サービスサーバを作る 〔初級〕

公式チュートリアル
[Writing a simple service and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Service-And-Client.html)
のサーバ側をそのまま書きます。

## やること

`src/add_two_ints_server.cpp` の TODO を埋めてください。仕様は公式チュートリアルと同一です。

| 項目 | 値 |
| --- | --- |
| ノード名 | `add_two_ints_server` |
| サービス名 | `add_two_ints` |
| 型 | `example_interfaces::srv::AddTwoInts` |
| 応答 | `response->sum = request->a + request->b;` |
| ログ1 | `Incoming request\na: %ld b: %ld`（`request->a`, `request->b`） |
| ログ2 | `sending back response: [%ld]`（`response->sum`） |

`AddTwoInts` の中身は次で確認できます。

```
$ ros2 interface show example_interfaces/srv/AddTwoInts
int64 a
int64 b
---
int64 sum
```

`---` の上がリクエスト（`AddTwoInts::Request`）、下がレスポンス（`AddTwoInts::Response`）です。

クラス宣言（`include/drill/add_two_ints_server.hpp`）は与えてあります。`add()` の中で
`response` に書き込むこと（`return` で返すのではありません）と、コンストラクタで
`create_service` の戻り値を `service_` に入れることを考えてください。

### 公式との違い

公式チュートリアルは `main()` の中で直接
`rclcpp::Node::make_shared("add_two_ints_server")` を呼び、`add` はクラスに属さない
自由関数として書きます。この課題ではテストからノードを直接生成して検証できるように、
`add_two_ints_server` という名前の `rclcpp::Node` 継承クラスにまとめています（公式の
`examples_rclcpp_minimal_service` パッケージと同じ形）。`main()` を別ファイル
（`src/server_main.cpp`）に分けている点も含め、これが公式との唯一の意図的な逸脱です。
ロジック（サービス名・ログの文言・応答の中身）は公式と完全に同一です。

## 動かしてみる

テストが通ったら、公式チュートリアルと同じように手で動かせます。

```bash
source install/setup.bash
ros2 run drill_04_service_server server
```

別の端末で:

```bash
ros2 service list                     # /add_two_ints が見えるか
ros2 interface show example_interfaces/srv/AddTwoInts
ros2 service call /add_two_ints example_interfaces/srv/AddTwoInts "{a: 20, b: 22}"
```

サーバ側の端末に `Incoming request` のログが、呼び出し側の端末に
`response:\nexample_interfaces.srv.AddTwoInts_Response(sum=42)` のような応答が
表示されれば成功です。

## つまずきポイント

- `create_service()` の戻り値は必ず `service_` に代入します。ローカル変数で受けると
  コンストラクタを抜けた時点で破棄され、サービスが消えます（01 の Publisher/Timer と同じ罠）。
- `add` はハンドラです。値を `return` するのではなく、引数で渡された
  `response`（`shared_ptr`）のメンバに書き込みます。
- `std::bind(&AddTwoIntsServer::add, this, _1, _2)` の `_1` `_2` は
  `std::placeholders::_1` / `_2` です。`using std::placeholders::_1;` を忘れると
  コンパイルエラーになります。
- `request->a` と `request->b` は `int64_t` です。ログの書式は `%ld` を使います
  （公式どおり）。

## テスト

```bash
./drill run 04
```

| テスト | 見ているところ |
| --- | --- |
| `add_two_intsサービスを公開している` | `create_service` を `service_` に入れているか |
| `2つの整数の和を返す` | `response->sum = request->a + request->b;` |
| `0や負の数でも正しく計算する` | 境界値（0、負数）での計算 |
| `連続して呼び出しても応答する` | 複数回のリクエストを処理できるか |
| `公式と同じIncoming_requestログを出している` | `RCLCPP_INFO` の書式 |

## 参考

- 公式: [Writing a simple service and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Service-And-Client.html)
- ローカルの実装例: `/opt/ros/jazzy/share/examples_rclcpp_minimal_service/`
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
