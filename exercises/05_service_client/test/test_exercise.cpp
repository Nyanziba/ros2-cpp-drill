// このファイルは編集しません（採点用）。
#include <future>
#include <memory>

#include "drill/add_two_ints_client.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

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

TEST_F(DrillTest, CanCreateAddTwoIntsClient)
{
  Server server;
  auto client = std::make_shared<AddTwoIntsClient>();

  ASSERT_TRUE(client->wait_for_server(3s))
    << drill::localized(
    "サーバを起動した状態でも wait_for_server(3s) が true になりませんでした。\n"
    "  - コンストラクタで client_ = this->create_client<AddTwoInts>(\"add_two_ints\"); "
    "していますか？\n"
    "  - wait_for_server で client_->wait_for_service(timeout) を呼び、"
    "見つかったら true を返していますか？",
    "wait_for_server(3s) did not return true even though the server is running.\n"
    "  - Does the constructor do client_ = this->create_client<AddTwoInts>(\"add_two_ints\");?\n"
    "  - Does wait_for_server call client_->wait_for_service(timeout) and return true "
    "when the service is found?");
}

TEST_F(DrillTest, SendRequestReturnsCorrectSum)
{
  Server server;
  auto client = std::make_shared<AddTwoIntsClient>();

  ASSERT_TRUE(client->wait_for_server(3s))
    << drill::localized(
    "wait_for_server(3s) が true になりませんでした。先にこのテストより上の"
    "CanCreateAddTwoIntsClient を通してください。",
    "wait_for_server(3s) did not return true. Make the CanCreateAddTwoIntsClient test "
    "above pass first.");

  auto future = client->send_request(41, 1);

  ASSERT_TRUE(
    drill::spin_until(
      {client, server.node},
      [&future]() {return future.wait_for(0s) == std::future_status::ready;}, 5s))
    << drill::localized(
    "send_request の応答が 5 秒待っても届きませんでした。\n"
    "  - request->a / request->b に a, b を代入していますか？\n"
    "  - client_->async_send_request(request) の戻り値を return していますか？",
    "The response to send_request did not arrive within 5 seconds.\n"
    "  - Do you assign a and b to request->a / request->b?\n"
    "  - Do you return the value of client_->async_send_request(request)?");

  EXPECT_EQ(future.get()->sum, 42)
    << drill::localized(
    "41 + 1 の応答が 42 になっていません。"
    "request->a = a; request->b = b; を確認してください。",
    "The response for 41 + 1 is not 42. "
    "Check request->a = a; request->b = b;.");
}

TEST_F(DrillTest, HandlesNegativeNumbers)
{
  Server server;
  auto client = std::make_shared<AddTwoIntsClient>();

  ASSERT_TRUE(client->wait_for_server(3s))
    << drill::localized(
    "wait_for_server(3s) が true になりませんでした。",
    "wait_for_server(3s) did not return true.");

  auto future = client->send_request(-100, -23);

  ASSERT_TRUE(
    drill::spin_until(
      {client, server.node},
      [&future]() {return future.wait_for(0s) == std::future_status::ready;}, 5s))
    << drill::localized(
    "send_request の応答が 5 秒待っても届きませんでした。",
    "The response to send_request did not arrive within 5 seconds.");

  EXPECT_EQ(future.get()->sum, -123)
    << drill::localized(
    "-100 + -23 の応答が -123 になっていません。a, b は int64_t なので"
    "負の値もそのまま代入できます。",
    "The response for -100 + -23 is not -123. a and b are int64_t, "
    "so negative values can be assigned as they are.");
}

TEST_F(DrillTest, WaitForServerReturnsFalseWhenNoServer)
{
  auto client = std::make_shared<AddTwoIntsClient>();

  const auto start = std::chrono::steady_clock::now();
  const bool found = client->wait_for_server(500ms);
  const auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_FALSE(found)
    << drill::localized(
    "サーバがいないのに wait_for_server(500ms) が true を返しました。\n"
    "  client_->wait_for_service(timeout) の戻り値をそのまま使っていますか？",
    "wait_for_server(500ms) returned true even though there is no server.\n"
    "  Do you use the return value of client_->wait_for_service(timeout) as it is?");
  EXPECT_LT(elapsed, 1s)
    << drill::localized(
    "wait_for_server(500ms) の呼び出しに 1 秒以上かかりました。timeout 引数を"
    "そのまま client_->wait_for_service に渡していますか？",
    "The call to wait_for_server(500ms) took 1 second or more. "
    "Do you pass the timeout argument to client_->wait_for_service as it is?");
}
