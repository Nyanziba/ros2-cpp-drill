// このファイルは編集しません（採点用）。
#include <memory>

#include "drill/node_with_parameters.hpp"
#include "drill_harness.hpp"

using DrillTest = drill::DrillTest;
using namespace std::chrono_literals;

TEST_F(DrillTest, an_int_paramが整数型で既定値0で宣言されている)
{
  auto node = std::make_shared<NodeWithParameters>();

  ASSERT_TRUE(node->has_parameter("an_int_param"))
    << "\"an_int_param\" が宣言されていません。\n"
    << "  this->declare_parameter(\"an_int_param\", 0); をコンストラクタで呼びましたか？";

  const auto param = node->get_parameter("an_int_param");
  EXPECT_EQ(param.get_type(), rclcpp::ParameterType::PARAMETER_INTEGER)
    << "\"an_int_param\" が整数型で宣言されていません。既定値に 0（整数リテラル）を"
    << "渡しましたか？";
  EXPECT_EQ(param.as_int(), 0)
    << "\"an_int_param\" の既定値が 0 になっていません。実際の値: " << param.as_int();
}

TEST_F(DrillTest, an_int_paramをsetするとlatest_valueが更新される)
{
  auto node = std::make_shared<NodeWithParameters>();

  ASSERT_TRUE(node->has_parameter("an_int_param"))
    << "\"an_int_param\" が宣言されていないため、このテストは実行できません。\n"
    << "  まず declare_parameter() を済ませてください（他のテストの失敗メッセージを参照）。";

  // set_parameter は /parameter_events に一度だけ publish するため、
  // discovery が終わるまでのタイミングによっては取りこぼす可能性がある。
  // tick で毎周期 set し直すことで、確実にコールバックへ届くのを待つ。
  ASSERT_TRUE(
    drill::spin_until(
      {node}, [&node]() {return node->latest_value() == 42;}, 4s,
      [&node]() {node->set_parameter(rclcpp::Parameter("an_int_param", 42));}))
    << "\"an_int_param\" を 42 に set しても、4 秒待っても latest_value() が 42 に"
    << "なりませんでした（実際の値: " << node->latest_value() << "）。\n"
    << "  - param_subscriber_ = std::make_shared<rclcpp::ParameterEventHandler>(this); "
    << "していますか？\n"
    << "  - add_parameter_callback(\"an_int_param\", cb) の戻り値をメンバ（cb_handle_）"
    << "に保持していますか？ 保持しないと、その場でコールバックが解除されます。\n"
    << "  - コールバックの中で latest_value_ = p.as_int(); をしていますか？";
}

TEST_F(DrillTest, 公式と同じcbログを出している)
{
  drill::LogCapture logs;
  auto node = std::make_shared<NodeWithParameters>();

  ASSERT_TRUE(node->has_parameter("an_int_param"))
    << "\"an_int_param\" が宣言されていないため、このテストは実行できません。\n"
    << "  まず declare_parameter() を済ませてください（他のテストの失敗メッセージを参照）。";

  ASSERT_TRUE(
    drill::spin_until(
      {node},
      [&logs]() {
        return logs.contains(
          "cb: Received an update to parameter \"an_int_param\" of type integer: \"7\"");
      },
      4s,
      [&node]() {node->set_parameter(rclcpp::Parameter("an_int_param", 7));}))
    << "公式と同じログが出ていません。\n"
    << "  RCLCPP_INFO(this->get_logger(),\n"
    << "    \"cb: Received an update to parameter \\\"%s\\\" of type %s: \\\"%ld\\\"\",\n"
    << "    p.get_name().c_str(), p.get_type_name().c_str(), p.as_int());\n"
    << "  add_parameter_callback の戻り値をメンバに保持していますか？\n"
    << "  実際に出ていたログ:" << logs.dump();
}

TEST_F(DrillTest, 2回目の変更でもコールバックが呼ばれる)
{
  auto node = std::make_shared<NodeWithParameters>();

  ASSERT_TRUE(node->has_parameter("an_int_param"))
    << "\"an_int_param\" が宣言されていないため、このテストは実行できません。\n"
    << "  まず declare_parameter() を済ませてください（他のテストの失敗メッセージを参照）。";

  ASSERT_TRUE(
    drill::spin_until(
      {node}, [&node]() {return node->latest_value() == 1;}, 4s,
      [&node]() {node->set_parameter(rclcpp::Parameter("an_int_param", 1));}))
    << "前提となる 1 回目の変更で latest_value() が更新されませんでした"
    << "（他のテストの失敗メッセージも参照）。";

  ASSERT_TRUE(
    drill::spin_until(
      {node}, [&node]() {return node->latest_value() == 2;}, 4s,
      [&node]() {node->set_parameter(rclcpp::Parameter("an_int_param", 2));}))
    << "2 回目の変更で latest_value() が更新されませんでした（実際の値: "
    << node->latest_value() << "）。\n"
    << "  add_parameter_callback の戻り値（cb_handle_）が生き続けていますか？\n"
    << "  一時オブジェクトとして受けてしまうと、コールバックはすぐに解除され"
    << "1 回目しか呼ばれません。";
}
