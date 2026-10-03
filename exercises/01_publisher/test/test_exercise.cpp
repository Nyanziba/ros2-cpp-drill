// このファイルは編集しません（採点用）。
#include <memory>
#include <string>
#include <vector>

#include "drill/minimal_publisher.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

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

TEST_F(DrillTest, PublishesToTopicTopic)
{
  auto talker = std::make_shared<MinimalPublisher>();
  Probe probe;

  ASSERT_TRUE(
    drill::spin_until({talker, probe.node}, [&probe]() {return probe.received.size() >= 2;}, 8s))
    << drill::localized(
    "\"topic\" に 8 秒待っても 2 件届きませんでした（受信 ",
    "Waited 8 seconds but \"topic\" did not get 2 messages (received ")
    << probe.received.size()
    << drill::localized(
    " 件）。\n"
    "  - create_publisher<std_msgs::msg::String>(\"topic\", 10) を publisher_ に入れましたか？\n"
    "  - create_wall_timer(500ms, ...) を timer_ に入れましたか？\n"
    "  - タイマのコールバックで publisher_->publish(message) を呼んでいますか？",
    ").\n"
    "  - Did you assign create_publisher<std_msgs::msg::String>(\"topic\", 10) to publisher_?\n"
    "  - Did you assign create_wall_timer(500ms, ...) to timer_?\n"
    "  - Does the timer callback call publisher_->publish(message)?");
}

TEST_F(DrillTest, BodyIsHelloWorldWithSequenceNumber)
{
  auto talker = std::make_shared<MinimalPublisher>();
  Probe probe;

  ASSERT_TRUE(
    drill::spin_until({talker, probe.node}, [&probe]() {return probe.received.size() >= 3;}, 8s))
    << drill::localized("3 件受信できませんでした（受信 ", "Did not receive 3 messages (received ")
    << probe.received.size() << drill::localized(" 件）。", ").");

  EXPECT_EQ(probe.received[0], "Hello, world! 0")
    << drill::localized(
    "1 通目が \"Hello, world! 0\" になっていません。実際の値: \"",
    "The 1st message is not \"Hello, world! 0\". Actual value: \"")
    << probe.received[0] << "\"";
  EXPECT_EQ(probe.received[1], "Hello, world! 1")
    << drill::localized(
    "2 通目が \"Hello, world! 1\" になっていません。count_ を後置インクリメント"
    "（count_++）していますか？ 実際の値: \"",
    "The 2nd message is not \"Hello, world! 1\". Do you post-increment count_ "
    "(count_++)? Actual value: \"")
    << probe.received[1] << "\"";
  EXPECT_EQ(probe.received[2], "Hello, world! 2")
    << drill::localized(
    "3 通目が \"Hello, world! 2\" になっていません。実際の値: \"",
    "The 3rd message is not \"Hello, world! 2\". Actual value: \"")
    << probe.received[2] << "\"";
}

TEST_F(DrillTest, PublishesAboutEvery500Milliseconds)
{
  auto talker = std::make_shared<MinimalPublisher>();
  Probe probe;

  ASSERT_TRUE(
    drill::spin_until({talker, probe.node}, [&probe]() {return probe.stamps.size() >= 3;}, 8s))
    << drill::localized(
    "3 件受信できませんでした。周期が遅すぎませんか？",
    "Did not receive 3 messages. Is the period too slow?");

  // discovery 直後は詰まって届くことがあるので、2 通目以降の間隔を見る。
  const auto span = std::chrono::duration_cast<std::chrono::milliseconds>(
    probe.stamps[2] - probe.stamps[1]).count();
  EXPECT_GE(span, 250)
    << drill::localized("publish 周期が速すぎます（実測 ", "The publish period is too fast (measured ")
    << span << drill::localized(" ms）。500ms ですか？", " ms). Is it 500ms?");
  EXPECT_LE(span, 900)
    << drill::localized("publish 周期が遅すぎます（実測 ", "The publish period is too slow (measured ")
    << span << drill::localized(" ms）。500ms ですか？", " ms). Is it 500ms?");
}

TEST_F(DrillTest, LogsSameAsOfficialPublishing)
{
  drill::LogCapture logs;
  auto talker = std::make_shared<MinimalPublisher>();
  Probe probe;

  ASSERT_TRUE(
    drill::spin_until({talker, probe.node}, [&probe]() {return !probe.received.empty();}, 8s))
    << drill::localized("publish されていません。", "Nothing was published.");

  EXPECT_TRUE(logs.contains("Publishing: 'Hello, world! 0'"))
    << drill::localized("公式と同じログが出ていません。\n", "The same log as the official example is missing.\n")
    << "  RCLCPP_INFO(this->get_logger(), \"Publishing: '%s'\", message.data.c_str());\n"
    << drill::localized("  実際に出ていたログ:", "  Logs that were printed:") << logs.dump();
}
