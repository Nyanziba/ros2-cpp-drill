// このファイルは編集しません（採点用）。
#include <future>
#include <memory>

#include "drill/add_two_ints_client.hpp"
#include "drill_harness.hpp"

using DrillTest = drill::DrillTest;
using AddTwoInts = AddTwoIntsClient::AddTwoInts;
using namespace std::chrono_literals;

namespace
{

/// probe ノードで "add_two_ints" サービスを提供する。
///
/// 課題04 (server) には依存させず、テスト自身がサーバ役を用意する。
struct Server
{
  rclcpp::Node::SharedPtr node;
  rclcpp::Service<AddTwoInts>::SharedPtr service;

  Server()
  : node(rclcpp::Node::make_shared(drill::unique_name("add_two_ints_server")))
  {
    service = node->create_service<AddTwoInts>(
      "add_two_ints",
      [](const std::shared_ptr<AddTwoInts::Request> request,
        std::shared_ptr<AddTwoInts::Response> response) {
        response->sum = request->a + request->b;
      });
  }
};

}  // namespace

TEST_F(DrillTest, add_two_intsのクライアントを作れている)
{
  Server server;
  auto client = std::make_shared<AddTwoIntsClient>();

  ASSERT_TRUE(client->wait_for_server(3s))
    << "サーバを起動した状態でも wait_for_server(3s) が true になりませんでした。\n"
    << "  - コンストラクタで client_ = this->create_client<AddTwoInts>(\"add_two_ints\"); "
       "していますか？\n"
    << "  - wait_for_server で client_->wait_for_service(timeout) を呼び、"
       "見つかったら true を返していますか？";
}

TEST_F(DrillTest, send_requestで応答のsumが正しい)
{
  Server server;
  auto client = std::make_shared<AddTwoIntsClient>();

  ASSERT_TRUE(client->wait_for_server(3s))
    << "wait_for_server(3s) が true になりませんでした。先にこのテストより上の"
       "「クライアントを作れている」を通してください。";

  auto future = client->send_request(41, 1);

  ASSERT_TRUE(
    drill::spin_until(
      {client, server.node},
      [&future]() {return future.wait_for(0s) == std::future_status::ready;}, 5s))
    << "send_request の応答が 5 秒待っても届きませんでした。\n"
    << "  - request->a / request->b に a, b を代入していますか？\n"
    << "  - client_->async_send_request(request) の戻り値を return していますか？";

  EXPECT_EQ(future.get()->sum, 42)
    << "41 + 1 の応答が 42 になっていません。"
    << "request->a = a; request->b = b; を確認してください。";
}

TEST_F(DrillTest, 負の数でも正しく計算できる)
{
  Server server;
  auto client = std::make_shared<AddTwoIntsClient>();

  ASSERT_TRUE(client->wait_for_server(3s))
    << "wait_for_server(3s) が true になりませんでした。";

  auto future = client->send_request(-100, -23);

  ASSERT_TRUE(
    drill::spin_until(
      {client, server.node},
      [&future]() {return future.wait_for(0s) == std::future_status::ready;}, 5s))
    << "send_request の応答が 5 秒待っても届きませんでした。";

  EXPECT_EQ(future.get()->sum, -123)
    << "-100 + -23 の応答が -123 になっていません。a, b は int64_t なので"
       "負の値もそのまま代入できます。";
}

TEST_F(DrillTest, サーバがいないときwait_for_serverがfalseを返す)
{
  auto client = std::make_shared<AddTwoIntsClient>();

  const auto start = std::chrono::steady_clock::now();
  const bool found = client->wait_for_server(500ms);
  const auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_FALSE(found)
    << "サーバがいないのに wait_for_server(500ms) が true を返しました。\n"
    << "  client_->wait_for_service(timeout) の戻り値をそのまま使っていますか？";
  EXPECT_LT(elapsed, 1s)
    << "wait_for_server(500ms) の呼び出しに 1 秒以上かかりました。timeout 引数を"
       "そのまま client_->wait_for_service に渡していますか？";
}
