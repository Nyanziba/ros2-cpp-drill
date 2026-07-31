// このファイルは編集しません（採点用）。
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "drill/num_node.hpp"
#include "drill_harness.hpp"

using DrillTest = drill::DrillTest;
using Num = NumNode::Num;
using AddThreeInts = NumNode::AddThreeInts;
using namespace std::chrono_literals;

namespace
{

/// probe ノードで "num" を購読し、受信した num を集める。
struct NumProbe
{
  rclcpp::Node::SharedPtr node;
  rclcpp::Subscription<Num>::SharedPtr subscription;
  std::vector<std::int64_t> received;

  NumProbe()
  : node(rclcpp::Node::make_shared(drill::unique_name("probe")))
  {
    subscription = node->create_subscription<Num>(
      "num", 10,
      [this](Num::ConstSharedPtr msg) {
        received.push_back(msg->num);
      });
  }
};

/// probe ノードから "add_three_ints" を呼ぶためのクライアント一式。
struct AddProbe
{
  rclcpp::Node::SharedPtr node;
  rclcpp::Client<AddThreeInts>::SharedPtr client;

  AddProbe()
  : node(rclcpp::Node::make_shared(drill::unique_name("probe")))
  {
    client = node->create_client<AddThreeInts>("add_three_ints");
  }
};

/// server と probe を spin しながら add_three_ints を呼び、sum を返す。
/// サーバが見つからない・応答が返らない場合は std::nullopt。
std::optional<int64_t> call_add_three(
  const rclcpp::Node::SharedPtr & server, AddProbe & probe,
  int64_t a, int64_t b, int64_t c)
{
  if (!drill::spin_until(
      {server, probe.node},
      [&probe]() {return probe.client->service_is_ready();}, 5s))
  {
    return std::nullopt;
  }

  auto request = std::make_shared<AddThreeInts::Request>();
  request->a = a;
  request->b = b;
  request->c = c;
  auto future = probe.client->async_send_request(request);

  if (!drill::spin_until(
      {server, probe.node},
      [&future]() {return future.wait_for(0s) == std::future_status::ready;}, 5s))
  {
    return std::nullopt;
  }

  return future.get()->sum;
}

}  // namespace

TEST_F(DrillTest, Num型がint64のnumという1フィールドで定義されている)
{
  // ここでコンパイルが通ること自体が検証です。
  // msg/Num.msg のフィールド名や型が違うと（num でない、int64 でない等）、
  // drill_03_custom_interface::msg::Num に num メンバが無くなり、
  // このテストファイル自体がコンパイルエラーになります。
  //
  //   例: error: no member named 'num' in
  //       'drill_03_custom_interface::msg::Num_<std::allocator<void> >'
  //
  // その場合は msg/Num.msg を確認してください。
  Num msg;
  msg.num = 42;
  EXPECT_EQ(msg.num, 42)
    << "Num::num に代入した値が読み出せませんでした。実際の値: " << msg.num;
}

TEST_F(DrillTest, numトピックにNumがpublishされている)
{
  auto node = std::make_shared<NumNode>();
  NumProbe probe;

  ASSERT_TRUE(
    drill::spin_until({node, probe.node}, [&probe]() {return probe.received.size() >= 2;}, 8s))
    << "\"num\" に8秒待っても2件届きませんでした（受信 " << probe.received.size() << " 件）。\n"
    << "  - create_publisher<Num>(\"num\", 10) を publisher_ に入れましたか？\n"
    << "  - create_wall_timer(500ms, ...) を timer_ に入れましたか？\n"
    << "  - タイマのコールバックで publisher_->publish(message) を呼んでいますか？";

  EXPECT_EQ(probe.received[0], 0)
    << "1通目の num が 0 になっていません。実際の値: " << probe.received[0];
  EXPECT_EQ(probe.received[1], 1)
    << "2通目の num が 1 になっていません。count_ を後置インクリメント"
    << "（count_++）していますか？ 実際の値: " << probe.received[1];
}

TEST_F(DrillTest, add_three_intsサービスを公開している)
{
  auto node = std::make_shared<NumNode>();
  AddProbe probe;

  ASSERT_TRUE(
    drill::spin_until(
      {node, probe.node},
      [&probe]() {return probe.client->service_is_ready();}, 5s))
    << "\"add_three_ints\" サービスが5秒待っても見つかりませんでした。\n"
    << "  - srv/AddThreeInts.srv に a, b, c と sum を書きましたか？\n"
    << "  - create_service<AddThreeInts>(\"add_three_ints\", ...) を service_ に"
    << " 入れましたか？";
}

TEST_F(DrillTest, 3つの整数の和を返す)
{
  auto node = std::make_shared<NumNode>();
  AddProbe probe;

  auto sum = call_add_three(node, probe, 1, 2, 3);
  ASSERT_TRUE(sum.has_value())
    << "add_three_ints を呼びましたが応答が返ってきませんでした。\n"
    << "  - add_three_ints() の中で response に書き込んでいますか（return ではありません）？";
  EXPECT_EQ(sum.value(), 6)
    << "1 + 2 + 3 の応答が 6 になっていません。実際の値: " << sum.value() << "\n"
    << "  response->sum = request->a + request->b + request->c; を書きましたか？";

  auto negative = call_add_three(node, probe, -5, 3, 2);
  ASSERT_TRUE(negative.has_value()) << "-5 + 3 + 2 の呼び出しに応答がありませんでした。";
  EXPECT_EQ(negative.value(), 0)
    << "-5 + 3 + 2 が 0 になっていません。実際の値: " << negative.value();
}

TEST_F(DrillTest, 公式と同じ書式でIncoming_requestログを出している)
{
  drill::LogCapture logs;
  auto node = std::make_shared<NumNode>();
  AddProbe probe;

  auto sum = call_add_three(node, probe, 1, 2, 3);
  ASSERT_TRUE(sum.has_value()) << "add_three_ints の呼び出しに応答がありませんでした。";

  EXPECT_TRUE(logs.contains("Incoming request"))
    << "04課題（AddTwoInts）と同じ書式のログが出ていません。\n"
    << "  RCLCPP_INFO(this->get_logger(), \"Incoming request\\na: %ld b: %ld c: %ld\","
    << " request->a, request->b, request->c);\n"
    << "  実際に出ていたログ:" << logs.dump();
}
