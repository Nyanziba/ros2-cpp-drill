// このファイルは編集しません（採点用）。
#include <memory>
#include <string>
#include <vector>

#include <rmw/types.h>

#include "drill/qos_nodes.hpp"
#include "drill_harness.hpp"

using DrillTest = drill::DrillTest;
using namespace std::chrono_literals;

TEST_F(DrillTest, publisherの実効QoSがTRANSIENT_LOCALかつRELIABLEでdepth1になっている)
{
  auto publisher_node = std::make_shared<LatchedPublisher>();
  const auto qos = publisher_node->actual_qos();

  // rclcpp::QoS::durability() / reliability() は RMW の enum
  // （RMW_QOS_POLICY_DURABILITY_* など）をラップした rclcpp 側の enum class を返す。
  EXPECT_EQ(qos.durability(), rclcpp::DurabilityPolicy::TransientLocal)
    << "publisher_ の QoS が TRANSIENT_LOCAL になっていません"
       "（実際の値: " << static_cast<int>(qos.durability())
    << " / RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL = "
    << static_cast<int>(RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL) << "）。\n"
    << "  rclcpp::QoS qos(rclcpp::KeepLast(1)); qos.transient_local(); を"
       " create_publisher に渡しましたか？";

  EXPECT_EQ(qos.reliability(), rclcpp::ReliabilityPolicy::Reliable)
    << "publisher_ の QoS が RELIABLE になっていません"
       "（実際の値: " << static_cast<int>(qos.reliability()) << "）。"
       "qos.reliable(); を呼びましたか？";

  EXPECT_EQ(qos.depth(), 1u)
    << "publisher_ の History depth が 1 になっていません。"
       "rclcpp::KeepLast(1) で QoS を作りましたか？";
}

TEST_F(DrillTest, あとから起動した購読者にも過去にpublishした値が届く)
{
  // publisher を先に作って publish しておく。
  auto publisher_node = std::make_shared<LatchedPublisher>();
  publisher_node->publish("config-v1");

  // subscriber は publish より "あとで" 作る。
  auto subscriber_node = std::make_shared<LatchedSubscriber>();

  ASSERT_TRUE(
    drill::spin_until(
      {publisher_node, subscriber_node},
      [&subscriber_node]() {return subscriber_node->count() >= 1;}, 5s))
    << "あとから起動した購読者に、過去に publish した値が 5 秒待っても届きませんでした。\n"
    << "  publisher と subscription の両方を transient_local にしましたか？"
       " 片方だけでは繋がりません";

  EXPECT_EQ(subscriber_node->last_received(), "config-v1")
    << "届いた値が publish したものと一致しません。"
       "実際の値: \"" << subscriber_node->last_received() << "\"";
}

TEST_F(DrillTest, 新しい値をpublishすれば購読者に届く)
{
  // 今度は subscriber を先に作り、通常の経路（同時に動いている状態での配送）が
  // 壊れていないことを確認する。
  auto publisher_node = std::make_shared<LatchedPublisher>();
  auto subscriber_node = std::make_shared<LatchedSubscriber>();

  auto tick = [&publisher_node]() {publisher_node->publish("config-v2");};

  ASSERT_TRUE(
    drill::spin_until(
      {publisher_node, subscriber_node},
      [&subscriber_node]() {return subscriber_node->count() >= 1;}, 4s, tick))
    << "publish した値が購読者に届きませんでした。普通の経路（VOLATILE でも動くはずの経路）"
       "まで壊れていませんか？";

  EXPECT_EQ(subscriber_node->last_received(), "config-v2")
    << "届いた値が publish したものと一致しません。"
       "実際の値: \"" << subscriber_node->last_received() << "\"";
}

TEST_F(DrillTest, VOLATILEで購読すると過去の値は届かない)
{
  // 先に publish しておく（LatchedSubscriber ならこれが後から届く値）。
  auto publisher_node = std::make_shared<LatchedPublisher>();
  publisher_node->publish("config-should-not-backfill");

  // probe 側は "config" を明示的に VOLATILE（既定の durability）で購読する。
  // これは受講者の実装とは無関係に、durability は「購読側が要求した設定」で
  // 決まることを確かめるためのテスト。
  auto probe_node = rclcpp::Node::make_shared(drill::unique_name("probe"));
  rclcpp::QoS volatile_qos(rclcpp::KeepLast(1));
  volatile_qos.reliable();
  volatile_qos.durability_volatile();

  std::vector<std::string> received;
  auto probe_subscription = probe_node->create_subscription<std_msgs::msg::String>(
    "config", volatile_qos,
    [&received](std_msgs::msg::String::ConstSharedPtr msg) {
      received.push_back(msg->data);
    });

  // 短いタイムアウトで「過去の値が来ないこと」を確認する。
  const bool got_old_value = drill::spin_until(
    {publisher_node, probe_node}, [&received]() {return !received.empty();}, 1s);

  EXPECT_FALSE(got_old_value)
    << "VOLATILE で購読したのに過去の値が届いてしまいました"
       "（実際に受信した値: \"" << (received.empty() ? "" : received.front()) << "\"）。\n"
    << "  LatchedPublisher の QoS は本当に TRANSIENT_LOCAL になっていますか？"
       " このテストは受講者の実装ではなく DDS の durability の仕様を確認するものです。";

  // discovery 自体はできていることを、新しい値が届くことで確認する。
  publisher_node->publish("config-after-volatile-subscribe");
  ASSERT_TRUE(
    drill::spin_until(
      {publisher_node, probe_node}, [&received]() {return !received.empty();}, 4s))
    << "VOLATILE で購読した probe に、あとから publish した新しい値すら届きませんでした。"
       "discovery 自体が失敗しています（publisher の QoS 設定を見直してください）。";

  EXPECT_EQ(received.back(), "config-after-volatile-subscribe")
    << "VOLATILE 購読者に届いた値が想定と違います。実際の値: \"" << received.back() << "\"";
}
