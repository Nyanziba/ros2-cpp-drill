// このファイルは編集しません（採点用）。
#include <memory>

#include "drill/node_with_parameters.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using namespace std::chrono_literals;

TEST_F(DrillTest, AnIntParamIsDeclaredAsIntegerWithDefaultZero)
{
  auto node = std::make_shared<NodeWithParameters>();

  ASSERT_TRUE(node->has_parameter("an_int_param"))
    << drill::localized(
    "\"an_int_param\" が宣言されていません。\n"
    "  this->declare_parameter(\"an_int_param\", 0); をコンストラクタで呼びましたか？",
    "\"an_int_param\" is not declared.\n"
    "  Did you call this->declare_parameter(\"an_int_param\", 0); in the constructor?");

  const auto param = node->get_parameter("an_int_param");
  EXPECT_EQ(param.get_type(), rclcpp::ParameterType::PARAMETER_INTEGER)
    << drill::localized(
    "\"an_int_param\" が整数型で宣言されていません。既定値に 0（整数リテラル）を"
    "渡しましたか？",
    "\"an_int_param\" is not declared as an integer. Did you pass 0 (an integer literal) "
    "as the default value?");
  EXPECT_EQ(param.as_int(), 0)
    << drill::localized(
    "\"an_int_param\" の既定値が 0 になっていません。実際の値: ",
    "The default value of \"an_int_param\" is not 0. Actual value: ")
    << param.as_int();
}

TEST_F(DrillTest, SettingAnIntParamUpdatesLatestValue)
{
  auto node = std::make_shared<NodeWithParameters>();

  ASSERT_TRUE(node->has_parameter("an_int_param"))
    << drill::localized(
    "\"an_int_param\" が宣言されていないため、このテストは実行できません。\n"
    "  まず declare_parameter() を済ませてください（他のテストの失敗メッセージを参照）。",
    "\"an_int_param\" is not declared, so this test cannot run.\n"
    "  Call declare_parameter() first (see the failure messages of the other tests).");

  // set_parameter は /parameter_events に一度だけ publish するため、
  // discovery が終わるまでのタイミングによっては取りこぼす可能性がある。
  // tick で毎周期 set し直すことで、確実にコールバックへ届くのを待つ。
  ASSERT_TRUE(
    drill::spin_until(
      {node}, [&node]() {return node->latest_value() == 42;}, 4s,
      [&node]() {node->set_parameter(rclcpp::Parameter("an_int_param", 42));}))
    << drill::localized(
    "\"an_int_param\" を 42 に set しても、4 秒待っても latest_value() が 42 に"
    "なりませんでした（実際の値: ",
    "Set \"an_int_param\" to 42, but latest_value() did not become 42 "
    "within 4 seconds (actual value: ")
    << node->latest_value()
    << drill::localized(
    "）。\n"
    "  - param_subscriber_ = std::make_shared<rclcpp::ParameterEventHandler>(this); "
    "していますか？\n"
    "  - add_parameter_callback(\"an_int_param\", cb) の戻り値をメンバ（cb_handle_）"
    "に保持していますか？ 保持しないと、その場でコールバックが解除されます。\n"
    "  - コールバックの中で latest_value_ = p.as_int(); をしていますか？",
    ").\n"
    "  - Did you do param_subscriber_ = std::make_shared<rclcpp::ParameterEventHandler>(this);?\n"
    "  - Do you keep the return value of add_parameter_callback(\"an_int_param\", cb) "
    "in a member (cb_handle_)? If you do not, the callback is removed immediately.\n"
    "  - Do you do latest_value_ = p.as_int(); inside the callback?");
}

TEST_F(DrillTest, LogsSameCbMessageAsOfficial)
{
  drill::LogCapture logs;
  auto node = std::make_shared<NodeWithParameters>();

  ASSERT_TRUE(node->has_parameter("an_int_param"))
    << drill::localized(
    "\"an_int_param\" が宣言されていないため、このテストは実行できません。\n"
    "  まず declare_parameter() を済ませてください（他のテストの失敗メッセージを参照）。",
    "\"an_int_param\" is not declared, so this test cannot run.\n"
    "  Call declare_parameter() first (see the failure messages of the other tests).");

  ASSERT_TRUE(
    drill::spin_until(
      {node},
      [&logs]() {
        return logs.contains(
          "cb: Received an update to parameter \"an_int_param\" of type integer: \"7\"");
      },
      4s,
      [&node]() {node->set_parameter(rclcpp::Parameter("an_int_param", 7));}))
    << drill::localized("公式と同じログが出ていません。\n", "The same log as the official example is missing.\n")
    << "  RCLCPP_INFO(this->get_logger(),\n"
    << "    \"cb: Received an update to parameter \\\"%s\\\" of type %s: \\\"%ld\\\"\",\n"
    << "    p.get_name().c_str(), p.get_type_name().c_str(), p.as_int());\n"
    << drill::localized(
    "  add_parameter_callback の戻り値をメンバに保持していますか？\n"
    "  実際に出ていたログ:",
    "  Do you keep the return value of add_parameter_callback in a member?\n"
    "  Logs that were printed:")
    << logs.dump();
}

TEST_F(DrillTest, CallbackFiresOnSecondChangeToo)
{
  auto node = std::make_shared<NodeWithParameters>();

  ASSERT_TRUE(node->has_parameter("an_int_param"))
    << drill::localized(
    "\"an_int_param\" が宣言されていないため、このテストは実行できません。\n"
    "  まず declare_parameter() を済ませてください（他のテストの失敗メッセージを参照）。",
    "\"an_int_param\" is not declared, so this test cannot run.\n"
    "  Call declare_parameter() first (see the failure messages of the other tests).");

  ASSERT_TRUE(
    drill::spin_until(
      {node}, [&node]() {return node->latest_value() == 1;}, 4s,
      [&node]() {node->set_parameter(rclcpp::Parameter("an_int_param", 1));}))
    << drill::localized(
    "前提となる 1 回目の変更で latest_value() が更新されませんでした"
    "（他のテストの失敗メッセージも参照）。",
    "latest_value() was not updated by the 1st change, which this test needs "
    "(see the failure messages of the other tests).");

  ASSERT_TRUE(
    drill::spin_until(
      {node}, [&node]() {return node->latest_value() == 2;}, 4s,
      [&node]() {node->set_parameter(rclcpp::Parameter("an_int_param", 2));}))
    << drill::localized(
    "2 回目の変更で latest_value() が更新されませんでした（実際の値: ",
    "latest_value() was not updated by the 2nd change (actual value: ")
    << node->latest_value()
    << drill::localized(
    "）。\n"
    "  add_parameter_callback の戻り値（cb_handle_）が生き続けていますか？\n"
    "  一時オブジェクトとして受けてしまうと、コールバックはすぐに解除され"
    "1 回目しか呼ばれません。",
    ").\n"
    "  Does the return value of add_parameter_callback (cb_handle_) stay alive?\n"
    "  If you take it as a temporary object, the callback is removed immediately "
    "and is called only for the 1st change.");
}
