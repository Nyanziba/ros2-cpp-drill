// このファイルは編集しません（採点用）。
#include <memory>
#include <string>
#include <vector>

#include "drill/minimal_publisher.hpp"
#include "drill_harness.hpp"

using DrillTest = drill::DrillTest;
using namespace std::chrono_literals;

namespace
{

/// probe ノードで "topic" を購読し、受信した data を集める。
struct Probe
{
  rclcpp::Node::SharedPtr node;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription;
  std::vector<std::string> received;
  std::vector<std::chrono::steady_clock::time_point> stamps;

  Probe()
  : node(rclcpp::Node::make_shared(drill::unique_name("probe")))
  {
    subscription = node->create_subscription<std_msgs::msg::String>(
      "topic", 10,
      [this](std_msgs::msg::String::ConstSharedPtr msg) {
        received.push_back(msg->data);
        stamps.push_back(std::chrono::steady_clock::now());
      });
  }
};

}  // namespace

TEST_F(DrillTest, topicトピックにpublishしている)
{
  auto talker = std::make_shared<MinimalPublisher>();
  Probe probe;

  ASSERT_TRUE(
    drill::spin_until({talker, probe.node}, [&probe]() {return probe.received.size() >= 2;}, 8s))
    << "\"topic\" に 8 秒待っても 2 件届きませんでした（受信 " << probe.received.size() << " 件）。\n"
    << "  - create_publisher<std_msgs::msg::String>(\"topic\", 10) を publisher_ に入れましたか？\n"
    << "  - create_wall_timer(500ms, ...) を timer_ に入れましたか？\n"
    << "  - タイマのコールバックで publisher_->publish(message) を呼んでいますか？";
}

TEST_F(DrillTest, 本文がHello_worldと連番になっている)
{
  auto talker = std::make_shared<MinimalPublisher>();
  Probe probe;

  ASSERT_TRUE(
    drill::spin_until({talker, probe.node}, [&probe]() {return probe.received.size() >= 3;}, 8s))
    << "3 件受信できませんでした（受信 " << probe.received.size() << " 件）。";

  EXPECT_EQ(probe.received[0], "Hello, world! 0")
    << "1 通目が \"Hello, world! 0\" になっていません。"
    << "実際の値: \"" << probe.received[0] << "\"";
  EXPECT_EQ(probe.received[1], "Hello, world! 1")
    << "2 通目が \"Hello, world! 1\" になっていません。count_ を後置インクリメント"
    << "（count_++）していますか？ 実際の値: \"" << probe.received[1] << "\"";
  EXPECT_EQ(probe.received[2], "Hello, world! 2")
    << "3 通目が \"Hello, world! 2\" になっていません。"
    << "実際の値: \"" << probe.received[2] << "\"";
}

TEST_F(DrillTest, おおよそ500ミリ秒周期でpublishしている)
{
  auto talker = std::make_shared<MinimalPublisher>();
  Probe probe;

  ASSERT_TRUE(
    drill::spin_until({talker, probe.node}, [&probe]() {return probe.stamps.size() >= 3;}, 8s))
    << "3 件受信できませんでした。周期が遅すぎませんか？";

  // discovery 直後は詰まって届くことがあるので、2 通目以降の間隔を見る。
  const auto span = std::chrono::duration_cast<std::chrono::milliseconds>(
    probe.stamps[2] - probe.stamps[1]).count();
  EXPECT_GE(span, 250) << "publish 周期が速すぎます（実測 " << span << " ms）。500ms ですか？";
  EXPECT_LE(span, 900) << "publish 周期が遅すぎます（実測 " << span << " ms）。500ms ですか？";
}

TEST_F(DrillTest, 公式と同じPublishingログを出している)
{
  drill::LogCapture logs;
  auto talker = std::make_shared<MinimalPublisher>();
  Probe probe;

  ASSERT_TRUE(
    drill::spin_until({talker, probe.node}, [&probe]() {return !probe.received.empty();}, 8s))
    << "publish されていません。";

  EXPECT_TRUE(logs.contains("Publishing: 'Hello, world! 0'"))
    << "公式と同じログが出ていません。\n"
    << "  RCLCPP_INFO(this->get_logger(), \"Publishing: '%s'\", message.data.c_str());\n"
    << "  実際に出ていたログ:" << logs.dump();
}
