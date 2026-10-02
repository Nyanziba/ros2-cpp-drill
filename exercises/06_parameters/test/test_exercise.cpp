// このファイルは編集しません（採点用）。
#include <string>
#include <vector>

#include "drill/minimal_param.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using namespace std::chrono_literals;

TEST_F(DrillTest, MyParameterIsStringWithDefaultWorld)
{
  auto node = std::make_shared<MinimalParam>();

  ASSERT_TRUE(node->has_parameter("my_parameter"))
    << drill::localized(
    "\"my_parameter\" が宣言されていません。\n"
    "  this->declare_parameter(\"my_parameter\", \"world\", param_desc); を"
    "コンストラクタで呼びましたか？",
    "\"my_parameter\" is not declared.\n"
    "  Did you call this->declare_parameter(\"my_parameter\", \"world\", param_desc); "
    "in the constructor?");

  EXPECT_EQ(node->get_parameter("my_parameter").get_type(), rclcpp::ParameterType::PARAMETER_STRING)
    << drill::localized(
    "\"my_parameter\" が文字列型になっていません。既定値に \"world\"（文字列リテラル）"
    "を渡しましたか？",
    "\"my_parameter\" is not a string. Did you pass \"world\" (a string literal) "
    "as the default value?");

  EXPECT_EQ(node->get_parameter("my_parameter").as_string(), "world")
    << drill::localized(
    "\"my_parameter\" の既定値が \"world\" になっていません。実際の値: \"",
    "The default value of \"my_parameter\" is not \"world\". Actual value: \"")
    << node->get_parameter("my_parameter").as_string() << "\"";
}

TEST_F(DrillTest, ParameterDescriptorHasDescription)
{
  auto node = std::make_shared<MinimalParam>();

  ASSERT_TRUE(node->has_parameter("my_parameter"))
    << drill::localized("\"my_parameter\" が宣言されていません。", "\"my_parameter\" is not declared.");

  const auto descriptor = node->describe_parameter("my_parameter");
  EXPECT_EQ(descriptor.description, "This parameter is mine!")
    << drill::localized(
    "ParameterDescriptor の description が \"This parameter is mine!\" になっていません。\n",
    "The description of ParameterDescriptor is not \"This parameter is mine!\".\n")
    << "  auto param_desc = rcl_interfaces::msg::ParameterDescriptor{};\n"
    << "  param_desc.description = \"This parameter is mine!\";\n"
    << "  this->declare_parameter(\"my_parameter\", \"world\", param_desc);\n"
    << drill::localized("実際の値: \"", "Actual value: \"") << descriptor.description << "\"";
}

TEST_F(DrillTest, TimerLogsHelloWorld)
{
  drill::LogCapture logs;
  auto node = std::make_shared<MinimalParam>();

  ASSERT_TRUE(
    drill::spin_until({node}, [&logs]() {return logs.contains("Hello world!");}, 4s))
    << drill::localized(
    "4 秒待っても \"Hello world!\" というログが出ませんでした。\n"
    "  - create_wall_timer(1000ms, ...) を timer_ に入れましたか？\n"
    "  - タイマのコールバックで RCLCPP_INFO(this->get_logger(), \"Hello %s!\", "
    "my_param.c_str()); を呼んでいますか？\n"
    "  実際に出ていたログ:",
    "The log \"Hello world!\" did not appear after waiting 4 seconds.\n"
    "  - Did you assign create_wall_timer(1000ms, ...) to timer_?\n"
    "  - Does the timer callback call RCLCPP_INFO(this->get_logger(), \"Hello %s!\", "
    "my_param.c_str());?\n"
    "  Logs that were printed:")
    << logs.dump();
}

TEST_F(DrillTest, RevertsToWorldAfterParamSet)
{
  drill::LogCapture logs;
  auto node = std::make_shared<MinimalParam>();

  ASSERT_TRUE(node->has_parameter("my_parameter"))
    << drill::localized(
    "\"my_parameter\" が宣言されていないため、このテストは実行できません。\n"
    "  まず declare_parameter() を済ませてください"
    "（他のテストの失敗メッセージを参照）。",
    "\"my_parameter\" is not declared, so this test cannot run.\n"
    "  Call declare_parameter() first (see the failure messages of the other tests).");

  node->set_parameter(rclcpp::Parameter("my_parameter", "earth"));

  ASSERT_TRUE(
    drill::spin_until({node}, [&logs]() {return logs.contains("Hello earth!");}, 4s))
    << drill::localized(
    "\"earth\" に変更した後、4 秒待っても \"Hello earth!\" というログが出ませんでした。\n"
    "  タイマのコールバックで毎回 get_parameter(\"my_parameter\") を読み直していますか？\n"
    "  実際に出ていたログ:",
    "After changing it to \"earth\", the log \"Hello earth!\" did not appear within 4 seconds.\n"
    "  Does the timer callback read get_parameter(\"my_parameter\") again every time?\n"
    "  Logs that were printed:")
    << logs.dump();

  ASSERT_TRUE(
    drill::spin_until({node}, [&logs]() {return logs.contains("Hello world!");}, 4s))
    << drill::localized(
    "\"Hello earth!\" の後、4 秒待っても \"Hello world!\" に戻りませんでした。\n"
    "  公式チュートリアルのポイントです。コールバックの最後で\n",
    "After \"Hello earth!\", the log did not go back to \"Hello world!\" within 4 seconds.\n"
    "  This is the key point of the official tutorial. At the end of the callback,\n")
    << "  std::vector<rclcpp::Parameter> all_new_parameters{\n"
    << "    rclcpp::Parameter(\"my_parameter\", \"world\")};\n"
    << drill::localized(
    "  this->set_parameters(all_new_parameters); を呼んで \"my_parameter\" を"
    "\"world\" に戻していますか？\n"
    "  実際に出ていたログ:",
    "  do you call this->set_parameters(all_new_parameters); to set \"my_parameter\" "
    "back to \"world\"?\n"
    "  Logs that were printed:")
    << logs.dump();

  EXPECT_EQ(node->get_parameter("my_parameter").as_string(), "world")
    << drill::localized(
    "ログには \"Hello world!\" が出ましたが、ノード自身の \"my_parameter\" の値が"
    "\"world\" に戻っていません。set_parameters() を呼びましたか？\n"
    "実際の値: \"",
    "\"Hello world!\" was logged, but the value of the node's own \"my_parameter\" "
    "is not back to \"world\". Did you call set_parameters()?\n"
    "Actual value: \"")
    << node->get_parameter("my_parameter").as_string() << "\"";
}
