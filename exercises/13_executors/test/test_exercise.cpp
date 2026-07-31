// このファイルは編集しません（採点用）。
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "drill/relay_with_service.hpp"
#include "drill_harness.hpp"

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

TEST_F(DrillTest, サブスクライバとクライアントが別のコールバックグループにいる)
{
  auto node = std::make_shared<RelayWithService>();

  auto sub_group = node->subscription_group();
  auto client_group = node->client_group();

  ASSERT_NE(sub_group, nullptr)
    << "subscription_group() が nullptr です。\n"
    << "  - コンストラクタで create_callback_group() を呼び、\n"
    << "    subscription_group_ に入れましたか？";
  ASSERT_NE(client_group, nullptr)
    << "client_group() が nullptr です。\n"
    << "  - コンストラクタで create_callback_group() を呼び、\n"
    << "    client_group_ に入れましたか？";
  EXPECT_NE(sub_group, client_group)
    << "subscription_group() と client_group() が同じオブジェクトです。\n"
    << "  - コールバックグループは 2 つ、別々に create_callback_group() を\n"
    << "    呼んで作る必要があります。同じ変数を両方に代入していませんか？";
}

TEST_F(DrillTest, triggerに21を送るとsumに42がpublishされる)
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
    << "\"trigger\" に 21 を送りましたが、\"sum\" に何も届きませんでした。\n"
    << "コールバックグループを分けましたか？ "
       "同じグループだと応答を待つ間に応答処理が実行できず、必ずタイムアウトします";

  ASSERT_FALSE(probe.received.empty());
  EXPECT_EQ(probe.received.front(), 42)
    << "\"sum\" に届いた値が 42 ではありません。実際の値: " << probe.received.front() << "\n"
    << "  - request->a = request->b = msg.data にしていますか？\n"
    << "  - future.get()->sum を publish していますか？";
}

TEST_F(DrillTest, 連続してtriggerを送っても毎回応答する)
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
      << (i + 1) << " 回目の trigger (" << inputs[i] << ") に応答がありませんでした。\n"
      << "デッドロックしている可能性があります。"
         "コールバックグループを分けましたか？ "
         "同じグループだと応答を待つ間に応答処理が実行できず、必ずタイムアウトします";

    EXPECT_EQ(probe.received.back(), expected[i])
      << (i + 1) << " 回目の応答が " << expected[i] << " になっていません。"
      << "実際の値: " << probe.received.back();
  }
}

TEST_F(DrillTest, 負の値でも正しく計算する)
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
    << "-7 を送りましたが \"sum\" に何も届きませんでした。";

  EXPECT_EQ(probe.received.front(), -14)
    << "-7 + -7 の応答が -14 になっていません。実際の値: " << probe.received.front();
}
