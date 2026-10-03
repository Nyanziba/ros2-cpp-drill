// このファイルは編集しません（採点用）。
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include "drill/add_two_ints_server.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using AddTwoInts = AddTwoIntsServer::AddTwoInts;
using namespace std::chrono_literals;

namespace
{

/// probe ノードから "add_two_ints" を呼ぶためのクライアント一式。
struct Probe
{
  rclcpp::Node::SharedPtr node;
  rclcpp::Client<AddTwoInts>::SharedPtr client;

  Probe()
  : node(rclcpp::Node::make_shared(drill::unique_name("probe")))
  {
    client = node->create_client<AddTwoInts>("add_two_ints");
  }
};

/// server と probe を spin しながら add_two_ints を呼び、sum を返す。
/// サーバが見つからない・応答が返らない場合は std::nullopt。
std::optional<int64_t> call_add(
  const rclcpp::Node::SharedPtr & server, Probe & probe, int64_t a, int64_t b)
{
  if (!drill::spin_until(
      {server, probe.node},
      [&probe]() {return probe.client->service_is_ready();}, 5s))
  {
    return std::nullopt;
  }

  auto request = std::make_shared<AddTwoInts::Request>();
  request->a = a;
  request->b = b;
  auto future = probe.client->async_send_request(request);

  if (!drill::spin_until(
      {server, probe.node},
      [&future]() {return future.wait_for(0s) == std::future_status::ready;}, 5s))
  {
    return std::nullopt;
  }

  return future.get()->sum;
}

}  // namespace

TEST_F(DrillTest, ExposesAddTwoIntsService)
{
  auto server = std::make_shared<AddTwoIntsServer>();
  Probe probe;

  ASSERT_TRUE(
    drill::spin_until(
      {server, probe.node},
      [&probe]() {return probe.client->service_is_ready();}, 5s))
    << drill::localized(
    "\"add_two_ints\" サービスが 5 秒待っても見つかりませんでした。\n"
    "  - create_service<AddTwoInts>(\"add_two_ints\", ...) を service_ に入れましたか？\n"
    "  - コンストラクタの中で呼んでいますか？",
    "The \"add_two_ints\" service was not found after waiting 5 seconds.\n"
    "  - Did you assign create_service<AddTwoInts>(\"add_two_ints\", ...) to service_?\n"
    "  - Do you call it inside the constructor?");
}

TEST_F(DrillTest, ReturnsSumOfTwoIntegers)
{
  auto server = std::make_shared<AddTwoIntsServer>();
  Probe probe;

  auto sum = call_add(server, probe, 20, 22);
  ASSERT_TRUE(sum.has_value())
    << drill::localized(
    "add_two_ints を呼びましたが応答が返ってきませんでした。\n"
    "  - add() の中で response に書き込んでいますか（return ではありません）？",
    "Called add_two_ints but no response came back.\n"
    "  - Does add() write to response (not return it)?");
  EXPECT_EQ(sum.value(), 42)
    << drill::localized(
    "20 + 22 の応答が 42 になっていません。実際の値: ",
    "The response for 20 + 22 is not 42. Actual value: ")
    << sum.value() << "\n"
    << drill::localized(
    "  response->sum = request->a + request->b; を書きましたか？",
    "  Did you write response->sum = request->a + request->b; ?");
}

TEST_F(DrillTest, HandlesZeroAndNegativeNumbers)
{
  auto server = std::make_shared<AddTwoIntsServer>();
  Probe probe;

  auto zero = call_add(server, probe, 0, 0);
  ASSERT_TRUE(zero.has_value())
    << drill::localized("0 + 0 の呼び出しに応答がありませんでした。", "No response to the call 0 + 0.");
  EXPECT_EQ(zero.value(), 0)
    << drill::localized("0 + 0 が 0 になっていません。実際の値: ", "0 + 0 is not 0. Actual value: ")
    << zero.value();

  auto negative = call_add(server, probe, -5, 3);
  ASSERT_TRUE(negative.has_value())
    << drill::localized("-5 + 3 の呼び出しに応答がありませんでした。", "No response to the call -5 + 3.");
  EXPECT_EQ(negative.value(), -2)
    << drill::localized("-5 + 3 が -2 になっていません。実際の値: ", "-5 + 3 is not -2. Actual value: ")
    << negative.value();
}

TEST_F(DrillTest, RespondsToConsecutiveCalls)
{
  auto server = std::make_shared<AddTwoIntsServer>();
  Probe probe;

  const int64_t inputs[3][2] = {{1, 1}, {10, -3}, {100, 200}};
  const int64_t expected[3] = {2, 7, 300};

  for (int i = 0; i < 3; ++i) {
    auto sum = call_add(server, probe, inputs[i][0], inputs[i][1]);
    ASSERT_TRUE(sum.has_value())
      << drill::localized(
      "", "No response to call number ")
      << (i + 1)
      << drill::localized(
      " 回目の呼び出しに応答がありませんでした。"
      "サーバは複数回のリクエストを処理できる必要があります。",
      ". The server must be able to handle multiple requests.");
    EXPECT_EQ(sum.value(), expected[i])
      << drill::localized("", "The response of call number ")
      << (i + 1)
      << drill::localized(" 回目の応答が ", " is not ")
      << expected[i]
      << drill::localized(" になっていません。実際の値: ", ". Actual value: ")
      << sum.value();
  }
}

TEST_F(DrillTest, LogsSameIncomingRequestAsOfficial)
{
  drill::LogCapture logs;
  auto server = std::make_shared<AddTwoIntsServer>();
  Probe probe;

  auto sum = call_add(server, probe, 20, 22);
  ASSERT_TRUE(sum.has_value())
    << drill::localized(
    "add_two_ints の呼び出しに応答がありませんでした。",
    "No response to the add_two_ints call.");

  EXPECT_TRUE(logs.contains("Incoming request"))
    << drill::localized("公式と同じログが出ていません。\n", "The same log as the official example is missing.\n")
    << "  RCLCPP_INFO(this->get_logger(), \"Incoming request\\na: %ld b: %ld\","
    << " request->a, request->b);\n"
    << drill::localized("  実際に出ていたログ:", "  Logs that were printed:") << logs.dump();
}
