// このファイルは編集しません（採点用）。
#include <memory>
#include <string>
#include <vector>

#include "drill/minimal_subscriber.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using namespace std::chrono_literals;

namespace
{

/// 捕まえたログの中に needle を含む行がいくつあるか数える。
int count_lines_containing(const drill::LogCapture & logs, const std::string & needle)
{
  int n = 0;
  for (const auto & line : logs.lines()) {
    if (line.find(needle) != std::string::npos) {
      ++n;
    }
  }
  return n;
}

}  // namespace

TEST_F(DrillTest, SubscribesToTopicAndLogs)
{
  drill::LogCapture logs;
  auto listener = std::make_shared<MinimalSubscriber>();
  auto probe = rclcpp::Node::make_shared(drill::unique_name("probe"));
  auto pub = probe->create_publisher<std_msgs::msg::String>("topic", 10);

  std_msgs::msg::String msg;
  msg.data = "hello drill";
  auto tick = [&]() {pub->publish(msg);};

  ASSERT_TRUE(
    drill::spin_until(
      {listener, probe}, [&logs]() {return logs.contains("I heard: 'hello drill'");}, 5s, tick))
    << drill::localized(
    "\"topic\" に publish しても \"I heard: 'hello drill'\" というログが出ませんでした。\n"
    "  - create_subscription を subscription_ に代入しましたか？\n"
    "  - トピック名は \"topic\"、型は std_msgs::msg::String、QoS depth は 10 ですか？\n"
    "  - topic_callback の中で RCLCPP_INFO(this->get_logger(), \"I heard: '%s'\", "
    "msg.data.c_str()); を呼んでいますか？\n"
    "  実際に出ていたログ:",
    "Published to \"topic\" but the log \"I heard: 'hello drill'\" did not appear.\n"
    "  - Did you assign create_subscription to subscription_?\n"
    "  - Is the topic name \"topic\", the type std_msgs::msg::String, and the QoS depth 10?\n"
    "  - Does topic_callback call RCLCPP_INFO(this->get_logger(), \"I heard: '%s'\", "
    "msg.data.c_str());?\n"
    "  Logs that were printed:")
    << logs.dump();
}

TEST_F(DrillTest, LogsEveryTimeForMultipleMessages)
{
  drill::LogCapture logs;
  auto listener = std::make_shared<MinimalSubscriber>();
  auto probe = rclcpp::Node::make_shared(drill::unique_name("probe"));
  auto pub = probe->create_publisher<std_msgs::msg::String>("topic", 10);

  int counter = 0;
  auto tick = [&]() {
      std_msgs::msg::String msg;
      msg.data = "seq-" + std::to_string(counter++);
      pub->publish(msg);
    };

  ASSERT_TRUE(
    drill::spin_until(
      {listener, probe},
      [&logs]() {return count_lines_containing(logs, "I heard: 'seq-") >= 3;}, 5s, tick))
    << drill::localized(
    "\"I heard: 'seq-*'\" というログが 3 件届く前にタイムアウトしました（届いた件数: ",
    "Timed out before 3 \"I heard: 'seq-*'\" logs arrived (logs received: ")
    << count_lines_containing(logs, "I heard: 'seq-")
    << drill::localized(
    "）。\n"
    "  購読が最初の 1 件で止まっていませんか？ subscription_ をコンストラクタの"
    "ローカル変数ではなくメンバ変数に代入していますか？\n"
    "  実際に出ていたログ:",
    ").\n"
    "  Does the subscription stop after the first message? Did you assign subscription_ "
    "to the member variable instead of a local variable in the constructor?\n"
    "  Logs that were printed:")
    << logs.dump();
}

TEST_F(DrillTest, NodeNameIsMinimalSubscriber)
{
  auto listener = std::make_shared<MinimalSubscriber>();

  EXPECT_STREQ(listener->get_name(), "minimal_subscriber")
    << drill::localized(
    "ノード名が \"minimal_subscriber\" になっていません。実際の名前: \"",
    "The node name is not \"minimal_subscriber\". Actual name: \"")
    << listener->get_name() << "\"\n"
    << drill::localized(
    "  コンストラクタで Node(\"minimal_subscriber\") を呼んでいますか？",
    "  Does the constructor call Node(\"minimal_subscriber\")?");
}
