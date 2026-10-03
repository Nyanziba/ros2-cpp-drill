// このファイルは編集しません（採点用）。
#include <memory>
#include <string>
#include <vector>

#include <rmw/types.h>

#include "drill/qos_nodes.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using namespace std::chrono_literals;

TEST_F(DrillTest, PublisherEffectiveQosIsTransientLocalReliableDepth1)
{
  auto publisher_node = std::make_shared<LatchedPublisher>();
  const auto qos = publisher_node->actual_qos();

  // rclcpp::QoS::durability() / reliability() は RMW の enum
  // （RMW_QOS_POLICY_DURABILITY_* など）をラップした rclcpp 側の enum class を返す。
  EXPECT_EQ(qos.durability(), rclcpp::DurabilityPolicy::TransientLocal)
    << drill::localized(
    "publisher_ の QoS が TRANSIENT_LOCAL になっていません（実際の値: ",
    "The QoS of publisher_ is not TRANSIENT_LOCAL (actual value: ")
    << static_cast<int>(qos.durability())
    << " / RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL = "
    << static_cast<int>(RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL)
    << drill::localized(
    "）。\n"
    "  rclcpp::QoS qos(rclcpp::KeepLast(1)); qos.transient_local(); を"
    " create_publisher に渡しましたか？",
    ").\n"
    "  Did you pass rclcpp::QoS qos(rclcpp::KeepLast(1)); qos.transient_local(); "
    "to create_publisher?");

  EXPECT_EQ(qos.reliability(), rclcpp::ReliabilityPolicy::Reliable)
    << drill::localized(
    "publisher_ の QoS が RELIABLE になっていません（実際の値: ",
    "The QoS of publisher_ is not RELIABLE (actual value: ")
    << static_cast<int>(qos.reliability())
    << drill::localized(
    "）。qos.reliable(); を呼びましたか？",
    "). Did you call qos.reliable();?");

  EXPECT_EQ(qos.depth(), 1u)
    << drill::localized(
    "publisher_ の History depth が 1 になっていません。"
    "rclcpp::KeepLast(1) で QoS を作りましたか？",
    "The History depth of publisher_ is not 1. "
    "Did you create the QoS with rclcpp::KeepLast(1)?");
}

TEST_F(DrillTest, LateSubscriberReceivesPastPublishedValue)
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
    << drill::localized(
    "あとから起動した購読者に、過去に publish した値が 5 秒待っても届きませんでした。\n"
    "  publisher と subscription の両方を transient_local にしましたか？"
    " 片方だけでは繋がりません",
    "The value published in the past did not reach the late subscriber after waiting 5 seconds.\n"
    "  Did you make both the publisher and the subscription transient_local?"
    " One side alone does not connect");

  EXPECT_EQ(subscriber_node->last_received(), "config-v1")
    << drill::localized(
    "届いた値が publish したものと一致しません。実際の値: \"",
    "The received value does not match the published one. Actual value: \"")
    << subscriber_node->last_received() << "\"";
}

TEST_F(DrillTest, SubscriberReceivesNewlyPublishedValue)
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
    << drill::localized(
    "publish した値が購読者に届きませんでした。普通の経路（VOLATILE でも動くはずの経路）"
    "まで壊れていませんか？",
    "The published value did not reach the subscriber. Is even the normal path "
    "(the one that should work with VOLATILE) broken?");

  EXPECT_EQ(subscriber_node->last_received(), "config-v2")
    << drill::localized(
    "届いた値が publish したものと一致しません。実際の値: \"",
    "The received value does not match the published one. Actual value: \"")
    << subscriber_node->last_received() << "\"";
}

TEST_F(DrillTest, VolatileSubscriberDoesNotReceivePastValue)
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
    << drill::localized(
    "VOLATILE で購読したのに過去の値が届いてしまいました（実際に受信した値: \"",
    "A past value arrived even though the subscription is VOLATILE (value actually received: \"")
    << (received.empty() ? "" : received.front()) << "\""
    << drill::localized(
    "）。\n"
    "  LatchedPublisher の QoS は本当に TRANSIENT_LOCAL になっていますか？"
    " このテストは受講者の実装ではなく DDS の durability の仕様を確認するものです。",
    ").\n"
    "  Is the QoS of LatchedPublisher really TRANSIENT_LOCAL?"
    " This test checks the DDS durability specification, not your implementation.");

  // discovery 自体はできていることを、新しい値が届くことで確認する。
  publisher_node->publish("config-after-volatile-subscribe");
  ASSERT_TRUE(
    drill::spin_until(
      {publisher_node, probe_node}, [&received]() {return !received.empty();}, 4s))
    << drill::localized(
    "VOLATILE で購読した probe に、あとから publish した新しい値すら届きませんでした。"
    "discovery 自体が失敗しています（publisher の QoS 設定を見直してください）。",
    "Even a new value published later did not reach the VOLATILE probe. "
    "Discovery itself failed (review the QoS settings of the publisher).");

  EXPECT_EQ(received.back(), "config-after-volatile-subscribe")
    << drill::localized(
    "VOLATILE 購読者に届いた値が想定と違います。実際の値: \"",
    "The value that reached the VOLATILE subscriber is not what was expected. Actual value: \"")
    << received.back() << "\"";
}
