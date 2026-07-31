// このファイルは編集しません（採点用）。
#include <memory>
#include <string>
#include <vector>

#include "drill/minimal_subscriber.hpp"
#include "drill_harness.hpp"

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

TEST_F(DrillTest, topicを購読してログに出している)
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
    << "\"topic\" に publish しても \"I heard: 'hello drill'\" というログが出ませんでした。\n"
    << "  - create_subscription を subscription_ に代入しましたか？\n"
    << "  - トピック名は \"topic\"、型は std_msgs::msg::String、QoS depth は 10 ですか？\n"
    << "  - topic_callback の中で RCLCPP_INFO(this->get_logger(), \"I heard: '%s'\", "
       "msg.data.c_str()); を呼んでいますか？\n"
    << "  実際に出ていたログ:" << logs.dump();
}

TEST_F(DrillTest, 複数通受信しても毎回ログが出る)
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
    << "\"I heard: 'seq-*'\" というログが 3 件届く前にタイムアウトしました"
       "（届いた件数: " << count_lines_containing(logs, "I heard: 'seq-") << "）。\n"
    << "  購読が最初の 1 件で止まっていませんか？ subscription_ をコンストラクタの"
       "ローカル変数ではなくメンバ変数に代入していますか？\n"
    << "  実際に出ていたログ:" << logs.dump();
}

TEST_F(DrillTest, ノード名がminimal_subscriberになっている)
{
  auto listener = std::make_shared<MinimalSubscriber>();

  EXPECT_STREQ(listener->get_name(), "minimal_subscriber")
    << "ノード名が \"minimal_subscriber\" になっていません。実際の名前: \""
    << listener->get_name() << "\"\n"
    << "  コンストラクタで Node(\"minimal_subscriber\") を呼んでいますか？";
}
