// このファイルは編集しません（採点用）。
#include <memory>
#include <string>
#include <vector>

#include <class_loader/class_loader.hpp>
#include <rclcpp_components/node_factory.hpp>

#include "drill/composable_talker.hpp"
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

  Probe()
  : node(rclcpp::Node::make_shared(drill::unique_name("probe")))
  {
    subscription = node->create_subscription<std_msgs::msg::String>(
      "topic", 10,
      [this](std_msgs::msg::String::ConstSharedPtr msg) {
        received.push_back(msg->data);
      });
  }
};

}  // namespace

// 観点1: コンストラクタが options を Node にそのまま渡しているか。
//
// component_container は `--ros-args -r __node:=foo` のようなリマップを
// NodeOptions 経由でしか渡せない。Node("composable_talker") と決め打ちして
// options を捨ててしまうと、このリマップが一切効かなくなる。
TEST_F(DrillTest, NodeOptionsArePassedToNode)
{
  const auto options = rclcpp::NodeOptions().arguments(
    {"--ros-args", "-r", "__node:=renamed_talker"});
  auto talker = std::make_shared<ComposableTalker>(options);

  EXPECT_EQ(std::string(talker->get_name()), "renamed_talker")
    << drill::localized(
    "options で --ros-args -r __node:=renamed_talker を渡しても、"
    "ノード名が反映されませんでした（実際のノード名: \"",
    "Passed --ros-args -r __node:=renamed_talker in options, "
    "but the node name was not changed (actual node name: \"")
    << talker->get_name()
    << drill::localized(
    "\"）。\n"
    "  コンストラクタの初期化子リストを\n"
    "    : Node(\"composable_talker\", options)\n"
    "  にしていますか？ Node(\"composable_talker\") のように options を"
    "渡し忘れていませんか？",
    "\").\n"
    "  Is the initializer list of the constructor\n"
    "    : Node(\"composable_talker\", options)\n"
    "  ? Did you forget to pass options, as in Node(\"composable_talker\")?");
}

// 観点2: 実際に "topic" へ publish しているか（課題01 と同じ観点）。
TEST_F(DrillTest, PublishesToTopicTopic)
{
  auto talker = std::make_shared<ComposableTalker>(rclcpp::NodeOptions());
  Probe probe;

  ASSERT_TRUE(
    drill::spin_until({talker, probe.node}, [&probe]() {return probe.received.size() >= 2;}, 8s))
    << drill::localized(
    "\"topic\" に 8 秒待っても 2 件届きませんでした（受信 ",
    "Waited 8 seconds but \"topic\" did not get 2 messages (received ")
    << probe.received.size()
    << drill::localized(
    " 件）。\n"
    "  - create_publisher<std_msgs::msg::String>(\"topic\", 10) を publisher_ に"
    "入れましたか？\n"
    "  - create_wall_timer(200ms, ...) を timer_ に入れましたか？\n"
    "  - タイマのコールバックで publisher_->publish(message) を呼んでいますか？",
    ").\n"
    "  - Did you assign create_publisher<std_msgs::msg::String>(\"topic\", 10) "
    "to publisher_?\n"
    "  - Did you assign create_wall_timer(200ms, ...) to timer_?\n"
    "  - Does the timer callback call publisher_->publish(message)?");

  EXPECT_EQ(probe.received[0], "composable hello 0")
    << drill::localized(
    "1 通目の本文が \"composable hello 0\" になっていません。実際の値: \"",
    "The body of the 1st message is not \"composable hello 0\". Actual value: \"")
    << probe.received[0] << "\"";
}

// 観点3: RCLCPP_COMPONENTS_REGISTER_NODE(ComposableTalker) が書かれているか。
//
// これは「component_container がこのクラスをロードできるか」を直接確かめる
// 唯一の方法。ビルドされた共有ライブラリそのものを class_loader で読み込み、
// プラグインが登録されているかを見る。
//
// RCLCPP_COMPONENTS_REGISTER_NODE(ComposableTalker) の実体は
//   CLASS_LOADER_REGISTER_CLASS(
//     rclcpp_components::NodeFactoryTemplate<ComposableTalker>,
//     rclcpp_components::NodeFactory)
// であり、class_loader に登録される「クラス名」は "ComposableTalker" そのもの
// ではなく、テンプレートを展開した完全修飾名
// "rclcpp_components::NodeFactoryTemplate<ComposableTalker>" になる
// （`ros2 component load` が使う ament リソースインデックス上の名前は
// "ComposableTalker" のままだが、それは ComponentManager が内部でこの
// テンプレート名に組み立て直してから class_loader に問い合わせるため）。
// 実機の公式デモ（/opt/ros/jazzy/lib/libtalker_component.so）でも、
// 実際に登録されるクラス名は "rclcpp_components::NodeFactoryTemplate"
// <composition::Talker>" であることを確認済み。
//
// 登録マクロがなければ、ライブラリ自体は問題なくビルドできてしまうが、
// ここで名前が 1 件も見つからなくなる。
TEST_F(DrillTest, RegisteredWithRclcppComponentsRegisterNode)
{
  class_loader::ClassLoader loader(DRILL_COMPONENT_LIBRARY);

  const auto available = loader.getAvailableClasses<rclcpp_components::NodeFactory>();
  std::string available_list;
  for (const auto & name : available) {
    available_list += "\n      " + name;
  }
  if (available_list.empty()) {
    available_list = std::string("\n      ") + drill::localized("（1 件もありません）", "(none)");
  }

  const std::string expected_class =
    "rclcpp_components::NodeFactoryTemplate<ComposableTalker>";
  EXPECT_TRUE(loader.isClassAvailable<rclcpp_components::NodeFactory>(expected_class))
    << drill::localized("共有ライブラリ ", "The \"ComposableTalker\" component was not found in the shared library ")
    << DRILL_COMPONENT_LIBRARY
    << drill::localized(
    " の中に \"ComposableTalker\" コンポーネントが見つかりませんでした。\n"
    "  ソースの末尾に次の 2 行を書きましたか？\n",
    ".\n"
    "  Did you write these 2 lines at the end of the source?\n")
    << "    #include \"rclcpp_components/register_node_macro.hpp\"\n"
       "    RCLCPP_COMPONENTS_REGISTER_NODE(ComposableTalker)\n"
    << drill::localized(
    "  現在このライブラリから見えているコンポーネント一覧:",
    "  Components currently visible in this library:")
    << available_list;
}
