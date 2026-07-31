#include "drill/add_two_ints_client.hpp"

// I AM NOT DONE

AddTwoIntsClient::AddTwoIntsClient()
: Node("add_two_ints_client")
{
  // TODO: "add_two_ints" サービスのクライアントを作り、client_ に入れること。
  //       使う API は create_client。
}

bool AddTwoIntsClient::wait_for_server(std::chrono::nanoseconds timeout)
{
  // TODO: サーバが現れるまで timeout だけ待ち、見つかったら true を返すこと。
  //       使う API は client_->wait_for_service。
  //
  // TODO: 見つからなかったときは、公式と同じ 2 通りの扱いをすること。
  //       - rclcpp::ok() が false（Ctrl-C などで落ちた）なら、
  //         "Interrupted while waiting for the service. Exiting." を RCLCPP_ERROR で出す
  //       - まだ待てるなら "service not available, waiting again..." を RCLCPP_INFO で出す
  //       どちらの場合も false を返す（呼び出し側の main がループします）。
  //
  // コードの形が知りたければ ./drill hint 05
  (void)timeout;
  return false;
}

rclcpp::Client<AddTwoIntsClient::AddTwoInts>::FutureAndRequestId
AddTwoIntsClient::send_request(int64_t a, int64_t b)
{
  auto request = std::make_shared<AddTwoInts::Request>();

  // TODO: リクエストに a と b を詰めること。
  //       ここが空のままだと 0 + 0 を送ってしまい、応答の sum が合いません。
  (void)a;
  (void)b;

  // 下の 1 行は最初から書いてあります。
  // FutureAndRequestId は既定コンストラクタを持たない（ムーブのみの）型なので、
  // 「何も返さない空実装」を書けません。型の都合による例外で、課題ではありません。
  // 非同期でリクエストを送り、その場では待たずに future を返している形を読んでおいてください。
  return client_->async_send_request(request);
}
