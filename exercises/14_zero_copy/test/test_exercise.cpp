// このファイルは編集しません（採点用）。
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

#include "drill/zero_copy_nodes.hpp"
#include "drill_harness.hpp"

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

TEST_F(DrillTest, zero_copyトピックで通信できている)
{
  const auto options = rclcpp::NodeOptions().use_intra_process_comms(true);
  auto talker = std::make_shared<ZeroCopyTalker>(options);
  auto listener = std::make_shared<ZeroCopyListener>(options);

  talker->publish_once();

  ASSERT_TRUE(
    drill::spin_until({talker, listener}, [&listener]() {return listener->count() >= 1;}, 3s))
    << "\"zero_copy\" に 8 秒待っても届きませんでした（受信 " << listener->count() << " 件）。\n"
    << "  - create_publisher<std_msgs::msg::String>(\"zero_copy\", 10) を publisher_ に"
       " 入れましたか？\n"
    << "  - create_subscription で \"zero_copy\" を購読し、subscription_ に入れましたか？\n"
    << "  - publish_once() の最後で publisher_->publish(...) を呼んでいますか？";

  EXPECT_FALSE(listener->last_received_payload().empty())
    << "data が空でした。topic_callback で last_received_payload_ = msg->data を"
       " していますか？";
}

TEST_F(DrillTest, 送信側と受信側のアドレスが一致する)
{
  const auto options = rclcpp::NodeOptions().use_intra_process_comms(true);
  auto talker = std::make_shared<ZeroCopyTalker>(options);
  auto listener = std::make_shared<ZeroCopyListener>(options);

  talker->publish_once();

  ASSERT_TRUE(
    drill::spin_until({talker, listener}, [&listener]() {return listener->count() >= 1;}, 3s))
    << "メッセージが届きませんでした。";

  EXPECT_EQ(talker->last_published_address(), listener->last_received_address())
    << "送信側のアドレス 0x" << std::hex << talker->last_published_address()
    << " と受信側のアドレス 0x" << listener->last_received_address()
    << std::dec << " が一致しません（＝コピーが発生しています）。\n"
    << "  - publish に値を渡していませんか？ std::unique_ptr を std::move で渡すと"
       "コピーが消えます（publisher_->publish(std::move(msg))）。\n"
    << "  - 購読側を ConstSharedPtr で受けていますか？（コールバックの引数の型を"
       "変えていないか確認）";
}

TEST_F(DrillTest, dataの16進文字列も同じアドレスを指している)
{
  const auto options = rclcpp::NodeOptions().use_intra_process_comms(true);
  auto talker = std::make_shared<ZeroCopyTalker>(options);
  auto listener = std::make_shared<ZeroCopyListener>(options);

  talker->publish_once();

  ASSERT_TRUE(
    drill::spin_until({talker, listener}, [&listener]() {return listener->count() >= 1;}, 3s))
    << "メッセージが届きませんでした。";

  const auto address_in_payload = parse_hex_address(listener->last_received_payload());
  EXPECT_EQ(address_in_payload, listener->last_received_address())
    << "data に書き込んだアドレス（0x" << std::hex << address_in_payload
    << "）と実際に受信したメッセージのアドレス（0x" << listener->last_received_address()
    << std::dec << "）が一致しません。\n"
    << "  msg->data にアドレスを書き込んでから publish していますか？"
       "（ss << std::hex << reinterpret_cast<std::uintptr_t>(msg.get())）";
}

TEST_F(DrillTest, 複数回publishしても毎回アドレスが一致する)
{
  const auto options = rclcpp::NodeOptions().use_intra_process_comms(true);
  auto talker = std::make_shared<ZeroCopyTalker>(options);
  auto listener = std::make_shared<ZeroCopyListener>(options);

  for (std::size_t i = 1; i <= 3; ++i) {
    talker->publish_once();

    ASSERT_TRUE(
      drill::spin_until({talker, listener}, [&listener, i]() {return listener->count() >= i;}, 3s))
      << i << " 回目の publish が届きませんでした。";

    EXPECT_EQ(talker->last_published_address(), listener->last_received_address())
      << i << " 回目でアドレスが一致しません。\n"
      << "  - publish に値を渡していませんか？ std::unique_ptr を std::move で渡すと"
         "コピーが消えます。\n"
      << "  - 購読側を ConstSharedPtr で受けていますか？";
  }
}
