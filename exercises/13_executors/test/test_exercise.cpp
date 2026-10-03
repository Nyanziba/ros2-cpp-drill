// このファイルは編集しません（採点用）。
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "drill/relay_with_service.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using AddTwoInts = RelayWithService::AddTwoInts;
using namespace std::chrono_literals;

namespace
{

/// "add_two_ints" のサーバ役と "sum" の購読を兼ねる probe ノード。
///
/// サーバ役はラムダで a + b を返すだけ。RelayWithService がこの応答を
/// コールバックの中で同期的に待つ構成なので、probe 側は普通に
/// create_service で立てるだけでよい（probe と RelayWithService は別ノード＝
/// 別の Executor 管理下にあるので、probe 側にコールバックグループの分離は不要）。
struct Probe
{
  rclcpp::Node::SharedPtr node;
  rclcpp::Service<AddTwoInts>::SharedPtr server;
  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr trigger_publisher;
  rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sum_subscription;
  std::vector<int32_t> received;

  Probe()
  : node(rclcpp::Node::make_shared(drill::unique_name("probe")))
  {
    server = node->create_service<AddTwoInts>(
      "add_two_ints",
      [](
        const std::shared_ptr<AddTwoInts::Request> request,
        std::shared_ptr<AddTwoInts::Response> response) {
        response->sum = request->a + request->b;
      });

    trigger_publisher = node->create_publisher<std_msgs::msg::Int32>("trigger", 10);

    sum_subscription = node->create_subscription<std_msgs::msg::Int32>(
      "sum", 10,
      [this](std_msgs::msg::Int32::ConstSharedPtr msg) {
        received.push_back(msg->data);
      });
  }
};

}  // namespace

TEST_F(DrillTest, SubscriptionAndClientAreInDifferentCallbackGroups)
{
  auto node = std::make_shared<RelayWithService>();

  auto sub_group = node->subscription_group();
  auto client_group = node->client_group();

  ASSERT_NE(sub_group, nullptr)
    << drill::localized(
    "subscription_group() が nullptr です。\n"
    "  - コンストラクタで create_callback_group() を呼び、\n"
    "    subscription_group_ に入れましたか？",
    "subscription_group() is nullptr.\n"
    "  - Did you call create_callback_group() in the constructor and\n"
    "    assign the result to subscription_group_?");
  ASSERT_NE(client_group, nullptr)
    << drill::localized(
    "client_group() が nullptr です。\n"
    "  - コンストラクタで create_callback_group() を呼び、\n"
    "    client_group_ に入れましたか？",
    "client_group() is nullptr.\n"
    "  - Did you call create_callback_group() in the constructor and\n"
    "    assign the result to client_group_?");
  EXPECT_NE(sub_group, client_group)
    << drill::localized(
    "subscription_group() と client_group() が同じオブジェクトです。\n"
    "  - コールバックグループは 2 つ、別々に create_callback_group() を\n"
    "    呼んで作る必要があります。同じ変数を両方に代入していませんか？",
    "subscription_group() and client_group() are the same object.\n"
    "  - You need 2 callback groups, each made by its own call to\n"
    "    create_callback_group(). Did you assign the same variable to both?");
}

TEST_F(DrillTest, Trigger21PublishesSum42)
{
  auto node = std::make_shared<RelayWithService>();
  Probe probe;

  std_msgs::msg::Int32 msg;
  msg.data = 21;
  bool sent = false;

  // spin_until_multithreaded を使う理由: RelayWithService は
  // トリガーのコールバックの中でサービスの応答を待つ構成なので、
  // SingleThreadedExecutor では正解のコードでも必ずデッドロックしてタイムアウトする。
  ASSERT_TRUE(
    drill::spin_until_multithreaded(
      {node, probe.node},
      [&probe]() {return !probe.received.empty();},
      5s,
      [&]() {
        if (!sent && probe.trigger_publisher->get_subscription_count() > 0) {
          probe.trigger_publisher->publish(msg);
          sent = true;
        }
      }))
    << drill::localized(
    "\"trigger\" に 21 を送りましたが、\"sum\" に何も届きませんでした。\n"
    "コールバックグループを分けましたか？ "
    "同じグループだと応答を待つ間に応答処理が実行できず、必ずタイムアウトします",
    "Sent 21 to \"trigger\", but nothing arrived on \"sum\".\n"
    "Did you split the callback groups? "
    "With the same group, the response cannot be handled while waiting for it, "
    "so it always times out");

  ASSERT_FALSE(probe.received.empty());
  EXPECT_EQ(probe.received.front(), 42)
    << drill::localized(
    "\"sum\" に届いた値が 42 ではありません。実際の値: ",
    "The value that arrived on \"sum\" is not 42. Actual value: ")
    << probe.received.front() << "\n"
    << drill::localized(
    "  - request->a = request->b = msg.data にしていますか？\n"
    "  - future.get()->sum を publish していますか？",
    "  - Do you set request->a = request->b = msg.data?\n"
    "  - Do you publish future.get()->sum?");
}

TEST_F(DrillTest, RespondsToEveryConsecutiveTrigger)
{
  auto node = std::make_shared<RelayWithService>();
  Probe probe;

  const int32_t inputs[3] = {1, 10, 100};
  const int32_t expected[3] = {2, 20, 200};

  for (int i = 0; i < 3; ++i) {
    const auto before = probe.received.size();
    std_msgs::msg::Int32 msg;
    msg.data = inputs[i];
    bool sent = false;

    // discovery が終わるまでは publish が届かないので、購読が見えてから
    // 1 回だけ送って応答を待つ（test 2 / test 4 と同じやり方）。
    ASSERT_TRUE(
      drill::spin_until_multithreaded(
        {node, probe.node},
        [&probe, before]() {return probe.received.size() > before;},
        5s,
        [&]() {
          if (!sent && probe.trigger_publisher->get_subscription_count() > 0) {
            probe.trigger_publisher->publish(msg);
            sent = true;
          }
        }))
      << drill::localized("", "No response to trigger number ")
      << (i + 1)
      << drill::localized(" 回目の trigger (", " (")
      << inputs[i]
      << drill::localized(
      ") に応答がありませんでした。\n"
      "デッドロックしている可能性があります。"
      "コールバックグループを分けましたか？ "
      "同じグループだと応答を待つ間に応答処理が実行できず、必ずタイムアウトします",
      ").\n"
      "It may be deadlocked. "
      "Did you split the callback groups? "
      "With the same group, the response cannot be handled while waiting for it, "
      "so it always times out");

    EXPECT_EQ(probe.received.back(), expected[i])
      << drill::localized("", "The response of call number ")
      << (i + 1)
      << drill::localized(" 回目の応答が ", " is not ")
      << expected[i]
      << drill::localized(" になっていません。実際の値: ", ". Actual value: ")
      << probe.received.back();
  }
}

TEST_F(DrillTest, HandlesNegativeValues)
{
  auto node = std::make_shared<RelayWithService>();
  Probe probe;

  std_msgs::msg::Int32 msg;
  msg.data = -7;
  bool sent = false;

  ASSERT_TRUE(
    drill::spin_until_multithreaded(
      {node, probe.node},
      [&probe]() {return !probe.received.empty();},
      5s,
      [&]() {
        if (!sent && probe.trigger_publisher->get_subscription_count() > 0) {
          probe.trigger_publisher->publish(msg);
          sent = true;
        }
      }))
    << drill::localized(
    "-7 を送りましたが \"sum\" に何も届きませんでした。",
    "Sent -7, but nothing arrived on \"sum\".");

  EXPECT_EQ(probe.received.front(), -14)
    << drill::localized(
    "-7 + -7 の応答が -14 になっていません。実際の値: ",
    "The response for -7 + -7 is not -14. Actual value: ")
    << probe.received.front();
}
