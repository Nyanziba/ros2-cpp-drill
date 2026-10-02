// このファイルは編集しません（採点用）。
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

#include "drill/zero_copy_nodes.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using namespace std::chrono_literals;

namespace
{

/// last_received_payload() を 16 進文字列とみなしてアドレスに戻す。
std::uintptr_t parse_hex_address(const std::string & payload)
{
  std::uintptr_t value = 0;
  std::istringstream ss(payload);
  ss >> std::hex >> value;
  return value;
}

}  // namespace

TEST_F(DrillTest, CommunicatesOnZeroCopyTopic)
{
  const auto options = rclcpp::NodeOptions().use_intra_process_comms(true);
  auto talker = std::make_shared<ZeroCopyTalker>(options);
  auto listener = std::make_shared<ZeroCopyListener>(options);

  talker->publish_once();

  ASSERT_TRUE(
    drill::spin_until({talker, listener}, [&listener]() {return listener->count() >= 1;}, 3s))
    << drill::localized(
    "\"zero_copy\" に 3 秒待っても届きませんでした（受信 ",
    "Nothing arrived on \"zero_copy\" after waiting 3 seconds (received ")
    << listener->count()
    << drill::localized(
    " 件）。\n"
    "  - create_publisher<std_msgs::msg::String>(\"zero_copy\", 10) を publisher_ に"
    " 入れましたか？\n"
    "  - create_subscription で \"zero_copy\" を購読し、subscription_ に入れましたか？\n"
    "  - publish_once() の最後で publisher_->publish(...) を呼んでいますか？",
    ").\n"
    "  - Did you assign create_publisher<std_msgs::msg::String>(\"zero_copy\", 10) "
    "to publisher_?\n"
    "  - Did you subscribe to \"zero_copy\" with create_subscription and assign it "
    "to subscription_?\n"
    "  - Does publish_once() call publisher_->publish(...) at the end?");

  EXPECT_FALSE(listener->last_received_payload().empty())
    << drill::localized(
    "data が空でした。topic_callback で last_received_payload_ = msg->data を"
    " していますか？",
    "data was empty. Does topic_callback do last_received_payload_ = msg->data?");
}

TEST_F(DrillTest, PublisherAndSubscriberAddressesMatch)
{
  const auto options = rclcpp::NodeOptions().use_intra_process_comms(true);
  auto talker = std::make_shared<ZeroCopyTalker>(options);
  auto listener = std::make_shared<ZeroCopyListener>(options);

  talker->publish_once();

  ASSERT_TRUE(
    drill::spin_until({talker, listener}, [&listener]() {return listener->count() >= 1;}, 3s))
    << drill::localized("メッセージが届きませんでした。", "The message did not arrive.");

  EXPECT_EQ(talker->last_published_address(), listener->last_received_address())
    << drill::localized("送信側のアドレス 0x", "The sender address 0x")
    << std::hex << talker->last_published_address()
    << drill::localized(" と受信側のアドレス 0x", " and the receiver address 0x")
    << listener->last_received_address()
    << std::dec
    << drill::localized(
    " が一致しません（＝コピーが発生しています）。\n"
    "  - publish に値を渡していませんか？ std::unique_ptr を std::move で渡すと"
    "コピーが消えます（publisher_->publish(std::move(msg))）。\n"
    "  - 購読側を ConstSharedPtr で受けていますか？（コールバックの引数の型を"
    "変えていないか確認）",
    " do not match (= a copy was made).\n"
    "  - Are you passing a value to publish? Passing a std::unique_ptr with std::move "
    "removes the copy (publisher_->publish(std::move(msg))).\n"
    "  - Does the subscriber receive ConstSharedPtr? (Check that you did not change "
    "the type of the callback argument.)");
}

TEST_F(DrillTest, HexStringInDataPointsToSameAddress)
{
  const auto options = rclcpp::NodeOptions().use_intra_process_comms(true);
  auto talker = std::make_shared<ZeroCopyTalker>(options);
  auto listener = std::make_shared<ZeroCopyListener>(options);

  talker->publish_once();

  ASSERT_TRUE(
    drill::spin_until({talker, listener}, [&listener]() {return listener->count() >= 1;}, 3s))
    << drill::localized("メッセージが届きませんでした。", "The message did not arrive.");

  const auto address_in_payload = parse_hex_address(listener->last_received_payload());
  EXPECT_EQ(address_in_payload, listener->last_received_address())
    << drill::localized("data に書き込んだアドレス（0x", "The address written in data (0x")
    << std::hex << address_in_payload
    << drill::localized(
    "）と実際に受信したメッセージのアドレス（0x",
    ") and the address of the message actually received (0x")
    << listener->last_received_address()
    << std::dec
    << drill::localized(
    "）が一致しません。\n"
    "  msg->data にアドレスを書き込んでから publish していますか？"
    "（ss << std::hex << reinterpret_cast<std::uintptr_t>(msg.get())）",
    ") do not match.\n"
    "  Do you write the address to msg->data before publishing? "
    "(ss << std::hex << reinterpret_cast<std::uintptr_t>(msg.get()))");
}

TEST_F(DrillTest, AddressesMatchOnEveryRepeatedPublish)
{
  const auto options = rclcpp::NodeOptions().use_intra_process_comms(true);
  auto talker = std::make_shared<ZeroCopyTalker>(options);
  auto listener = std::make_shared<ZeroCopyListener>(options);

  for (std::size_t i = 1; i <= 3; ++i) {
    talker->publish_once();

    ASSERT_TRUE(
      drill::spin_until({talker, listener}, [&listener, i]() {return listener->count() >= i;}, 3s))
      << drill::localized("", "Publish number ") << i
      << drill::localized(" 回目の publish が届きませんでした。", " did not arrive.");

    EXPECT_EQ(talker->last_published_address(), listener->last_received_address())
      << drill::localized("", "The addresses do not match on publish number ") << i
      << drill::localized(
      " 回目でアドレスが一致しません。\n"
      "  - publish に値を渡していませんか？ std::unique_ptr を std::move で渡すと"
      "コピーが消えます。\n"
      "  - 購読側を ConstSharedPtr で受けていますか？",
      ".\n"
      "  - Are you passing a value to publish? Passing a std::unique_ptr with std::move "
      "removes the copy.\n"
      "  - Does the subscriber receive ConstSharedPtr?");
  }
}
