# 課題 05: サービスクライアントを作る 〔初級〕

公式チュートリアル
[Writing a simple service and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Service-And-Client.html)
のクライアント側（`add_two_ints_client`）を書きます。

## 公式からの逸脱

公式チュートリアルはクライアントの処理（サービス待ち・リクエスト送信・結果待ち）を
すべて `main()` の中に直接書きます。この課題ではテストからクラスを直接使えるように、
`AddTwoIntsClient : public rclcpp::Node` というクラスにまとめ、次の 2 つのメソッドに
切り出しています。

- `bool wait_for_server(std::chrono::nanoseconds timeout)`
  — 公式の `while (!client->wait_for_service(1s)) { ... }` の中身（1 回分）
- `rclcpp::Client<AddTwoInts>::FutureAndRequestId send_request(int64_t a, int64_t b)`
  — 公式の「リクエストを作って `async_send_request` する」部分

ループさせるかどうかは `main()`（`src/client_main.cpp`）側の責任にしています。
`wait_for_server` 自体はサーバが見つからなくても 1 回で戻ってくる関数です。

## やること

`src/add_two_ints_client.cpp` の TODO を埋めてください。

| 項目 | 値 |
| --- | --- |
| ノード名 | `add_two_ints_client` |
| サービス名 | `add_two_ints` |
| 型 | `example_interfaces::srv::AddTwoInts` |

埋める場所は 2 か所です。

1. **コンストラクタ**: `client_ = this->create_client<AddTwoInts>("add_two_ints");`
2. **`wait_for_server`**: `client_->wait_for_service(timeout)` を呼び、見つかったら
   `true`。見つからなければ公式と同じ手順で `rclcpp::ok()` を確認し、ログを出して
   `false` を返します。

`send_request` は**書き終わっています**。`FutureAndRequestId` は既定コンストラクタを
持たない型（コピー禁止・ムーブのみ）なので、「中身が空でもコンパイルは通る TODO」が
書けません。そのため、この課題では `send_request` を TODO の対象から外し、
`create_client` と `wait_for_server` の 2 か所を課題にしています。中身
（`request->a = a; request->b = b; return client_->async_send_request(request);`）は
読んで、リクエストの詰め方を理解しておいてください。

## 動かしてみる

テストが通ったら、課題04（`server`）を別端末で起動してから動かせます。

```bash
# 端末1
source install/setup.bash
ros2 run drill_04_service_server server

# 端末2
source install/setup.bash
ros2 run drill_05_service_client client 20 22
```

端末2に `Sum: 42` のようなログが出れば成功です。

## つまずきポイント

- `create_client()` の戻り値は必ず `client_` に代入します。ローカル変数で受けると
  コンストラクタを抜けた時点で破棄され、`send_request` の中で使えません。
- `wait_for_server` はサーバが見つからなくても**例外を投げず** `false` を返すだけです。
  ループさせて何度も呼ぶのは `main()` の仕事です。
- `client_->wait_for_service(timeout)` の戻り値をそのまま使ってください。
  `true`/`false` を自分で作り直す必要はありません。

## テスト

```bash
./drill run 05
```

テストは課題03の `server` には依存せず、テスト自身がサーバ役（probe ノード）を
`create_service<AddTwoInts>("add_two_ints", ...)` で用意します。

| テスト | 見ているところ |
| --- | --- |
| `add_two_intsのクライアントを作れている` | `create_client` と `wait_for_server` の基本形 |
| `send_requestで応答のsumが正しい` | リクエストの中身が正しく届いているか（41+1=42） |
| `負の数でも正しく計算できる` | `int64_t` の負値もそのまま扱えているか |
| `サーバがいないときwait_for_serverがfalseを返す` | `wait_for_service` の戻り値をそのまま返しているか（1 秒以内に終わること） |

## 参考

- 公式: [Writing a simple service and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Service-And-Client.html)
- ローカルの実装例: `/opt/ros/jazzy/share/examples_rclcpp_minimal_client/`
- 仕組みの解説: [docs/rclcpp-の設計思想.md](../../docs/rclcpp-の設計思想.md)
