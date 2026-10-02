// このファイルは編集しません（採点用）。
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "drill/num_node.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using Num = NumNode::Num;
using AddThreeInts = NumNode::AddThreeInts;
using namespace std::chrono_literals;

namespace
{

/// probe ノードで "num" を購読し、受信した num を集める。
struct NumProbe
{
  rclcpp::Node::SharedPtr node;
  rclcpp::Subscription<Num>::SharedPtr subscription;
  std::vector<std::int64_t> received;

  NumProbe()
  : node(rclcpp::Node::make_shared(drill::unique_name("probe")))
  {
    subscription = node->create_subscription<Num>(
      "num", 10,
      [this](Num::ConstSharedPtr msg) {
        received.push_back(msg->num);
      });
  }
};

/// probe ノードから "add_three_ints" を呼ぶためのクライアント一式。
struct AddProbe
{
  rclcpp::Node::SharedPtr node;
  rclcpp::Client<AddThreeInts>::SharedPtr client;

  AddProbe()
  : node(rclcpp::Node::make_shared(drill::unique_name("probe")))
  {
    client = node->create_client<AddThreeInts>("add_three_ints");
  }
};

/// server と probe を spin しながら add_three_ints を呼び、sum を返す。
/// サーバが見つからない・応答が返らない場合は std::nullopt。
std::optional<int64_t> call_add_three(
  const rclcpp::Node::SharedPtr & server, AddProbe & probe,
  int64_t a, int64_t b, int64_t c)
{
  if (!drill::spin_until(
      {server, probe.node},
      [&probe]() {return probe.client->service_is_ready();}, 5s))
  {
    return std::nullopt;
  }

  auto request = std::make_shared<AddThreeInts::Request>();
  request->a = a;
  request->b = b;
  request->c = c;
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

TEST_F(DrillTest, NumMessageHasSingleInt64NumField)
{
  // ここでコンパイルが通ること自体が検証です。
  // msg/Num.msg のフィールド名や型が違うと（num でない、int64 でない等）、
  // drill_03_custom_interface::msg::Num に num メンバが無くなり、
  // このテストファイル自体がコンパイルエラーになります。
  //
  //   例: error: no member named 'num' in
  //       'drill_03_custom_interface::msg::Num_<std::allocator<void> >'
  //
  // その場合は msg/Num.msg を確認してください。
  Num msg;
  msg.num = 42;
  EXPECT_EQ(msg.num, 42)
    << drill::localized(
    "Num::num に代入した値が読み出せませんでした。実際の値: ",
    "Could not read back the value assigned to Num::num. Actual value: ")
    << msg.num;
}

TEST_F(DrillTest, PublishesNumOnNumTopic)
{
  auto node = std::make_shared<NumNode>();
  NumProbe probe;

  ASSERT_TRUE(
    drill::spin_until({node, probe.node}, [&probe]() {return probe.received.size() >= 2;}, 8s))
    << drill::localized(
    "\"num\" に8秒待っても2件届きませんでした（受信 ",
    "Waited 8 seconds but \"num\" did not get 2 messages (received ")
    << probe.received.size()
    << drill::localized(
    " 件）。\n"
    "  - create_publisher<Num>(\"num\", 10) を publisher_ に入れましたか？\n"
    "  - create_wall_timer(500ms, ...) を timer_ に入れましたか？\n"
    "  - タイマのコールバックで publisher_->publish(message) を呼んでいますか？",
    ").\n"
    "  - Did you assign create_publisher<Num>(\"num\", 10) to publisher_?\n"
    "  - Did you assign create_wall_timer(500ms, ...) to timer_?\n"
    "  - Does the timer callback call publisher_->publish(message)?");

  EXPECT_EQ(probe.received[0], 0)
    << drill::localized(
    "1通目の num が 0 になっていません。実際の値: ",
    "The num of the 1st message is not 0. Actual value: ")
    << probe.received[0];
  EXPECT_EQ(probe.received[1], 1)
    << drill::localized(
    "2通目の num が 1 になっていません。count_ を後置インクリメント"
    "（count_++）していますか？ 実際の値: ",
    "The num of the 2nd message is not 1. Do you post-increment count_ "
    "(count_++)? Actual value: ")
    << probe.received[1];
}

TEST_F(DrillTest, ExposesAddThreeIntsService)
{
  auto node = std::make_shared<NumNode>();
  AddProbe probe;

  ASSERT_TRUE(
    drill::spin_until(
      {node, probe.node},
      [&probe]() {return probe.client->service_is_ready();}, 5s))
    << drill::localized(
    "\"add_three_ints\" サービスが5秒待っても見つかりませんでした。\n"
    "  - srv/AddThreeInts.srv に a, b, c と sum を書きましたか？\n"
    "  - create_service<AddThreeInts>(\"add_three_ints\", ...) を service_ に"
    " 入れましたか？",
    "The \"add_three_ints\" service was not found after waiting 5 seconds.\n"
    "  - Did you write a, b, c and sum in srv/AddThreeInts.srv?\n"
    "  - Did you assign create_service<AddThreeInts>(\"add_three_ints\", ...) to "
    "service_?");
}

TEST_F(DrillTest, ReturnsSumOfThreeIntegers)
{
  auto node = std::make_shared<NumNode>();
  AddProbe probe;

  auto sum = call_add_three(node, probe, 1, 2, 3);
  ASSERT_TRUE(sum.has_value())
    << drill::localized(
    "add_three_ints を呼びましたが応答が返ってきませんでした。\n"
    "  - add_three_ints() の中で response に書き込んでいますか（return ではありません）？",
    "Called add_three_ints but no response came back.\n"
    "  - Does add_three_ints() write to response (not return it)?");
  EXPECT_EQ(sum.value(), 6)
    << drill::localized(
    "1 + 2 + 3 の応答が 6 になっていません。実際の値: ",
    "The response for 1 + 2 + 3 is not 6. Actual value: ")
    << sum.value() << "\n"
    << drill::localized(
    "  response->sum = request->a + request->b + request->c; を書きましたか？",
    "  Did you write response->sum = request->a + request->b + request->c; ?");

  auto negative = call_add_three(node, probe, -5, 3, 2);
  ASSERT_TRUE(negative.has_value())
    << drill::localized(
    "-5 + 3 + 2 の呼び出しに応答がありませんでした。",
    "No response to the call -5 + 3 + 2.");
  EXPECT_EQ(negative.value(), 0)
    << drill::localized(
    "-5 + 3 + 2 が 0 になっていません。実際の値: ",
    "-5 + 3 + 2 is not 0. Actual value: ")
    << negative.value();
}

TEST_F(DrillTest, LogsIncomingRequestInOfficialFormat)
{
  drill::LogCapture logs;
  auto node = std::make_shared<NumNode>();
  AddProbe probe;

  auto sum = call_add_three(node, probe, 1, 2, 3);
  ASSERT_TRUE(sum.has_value())
    << drill::localized(
    "add_three_ints の呼び出しに応答がありませんでした。",
    "No response to the add_three_ints call.");

  EXPECT_TRUE(logs.contains("Incoming request"))
    << drill::localized(
    "04課題（AddTwoInts）と同じ書式のログが出ていません。\n",
    "The log in the same format as exercise 04 (AddTwoInts) is missing.\n")
    << "  RCLCPP_INFO(this->get_logger(), \"Incoming request\\na: %ld b: %ld c: %ld\","
    << " request->a, request->b, request->c);\n"
    << drill::localized("  実際に出ていたログ:", "  Logs that were printed:") << logs.dump();
}
