// このファイルは編集しません（採点用）。
#include <string>
#include <vector>

#include "drill/minimal_param.hpp"
#include "drill_harness.hpp"

using DrillTest = drill::DrillTest;
using namespace std::chrono_literals;

TEST_F(DrillTest, my_parameterが文字列で既定値worldになっている)
{
  auto node = std::make_shared<MinimalParam>();

  ASSERT_TRUE(node->has_parameter("my_parameter"))
    << "\"my_parameter\" が宣言されていません。\n"
    << "  this->declare_parameter(\"my_parameter\", \"world\", param_desc); を"
    << "コンストラクタで呼びましたか？";

  EXPECT_EQ(node->get_parameter("my_parameter").get_type(), rclcpp::ParameterType::PARAMETER_STRING)
    << "\"my_parameter\" が文字列型になっていません。既定値に \"world\"（文字列リテラル）"
    << "を渡しましたか？";

  EXPECT_EQ(node->get_parameter("my_parameter").as_string(), "world")
    << "\"my_parameter\" の既定値が \"world\" になっていません。"
    << "実際の値: \"" << node->get_parameter("my_parameter").as_string() << "\"";
}

TEST_F(DrillTest, ParameterDescriptorのdescriptionが設定されている)
{
  auto node = std::make_shared<MinimalParam>();

  ASSERT_TRUE(node->has_parameter("my_parameter"))
    << "\"my_parameter\" が宣言されていません。";

  const auto descriptor = node->describe_parameter("my_parameter");
  EXPECT_EQ(descriptor.description, "This parameter is mine!")
    << "ParameterDescriptor の description が \"This parameter is mine!\" になっていません。\n"
    << "  auto param_desc = rcl_interfaces::msg::ParameterDescriptor{};\n"
    << "  param_desc.description = \"This parameter is mine!\";\n"
    << "  this->declare_parameter(\"my_parameter\", \"world\", param_desc);\n"
    << "実際の値: \"" << descriptor.description << "\"";
}

TEST_F(DrillTest, タイマがHello_worldとログを出している)
{
  drill::LogCapture logs;
  auto node = std::make_shared<MinimalParam>();

  ASSERT_TRUE(
    drill::spin_until({node}, [&logs]() {return logs.contains("Hello world!");}, 4s))
    << "4 秒待っても \"Hello world!\" というログが出ませんでした。\n"
    << "  - create_wall_timer(1000ms, ...) を timer_ に入れましたか？\n"
    << "  - タイマのコールバックで RCLCPP_INFO(this->get_logger(), \"Hello %s!\", "
    << "my_param.c_str()); を呼んでいますか？\n"
    << "  実際に出ていたログ:" << logs.dump();
}

TEST_F(DrillTest, ros2_param_setで変えても1秒後にworldへ戻る)
{
  drill::LogCapture logs;
  auto node = std::make_shared<MinimalParam>();

  ASSERT_TRUE(node->has_parameter("my_parameter"))
    << "\"my_parameter\" が宣言されていないため、このテストは実行できません。\n"
    << "  まず declare_parameter() を済ませてください"
    << "（他のテストの失敗メッセージを参照）。";

  node->set_parameter(rclcpp::Parameter("my_parameter", "earth"));

  ASSERT_TRUE(
    drill::spin_until({node}, [&logs]() {return logs.contains("Hello earth!");}, 4s))
    << "\"earth\" に変更した後、4 秒待っても \"Hello earth!\" というログが出ませんでした。\n"
    << "  タイマのコールバックで毎回 get_parameter(\"my_parameter\") を読み直していますか？\n"
    << "  実際に出ていたログ:" << logs.dump();

  ASSERT_TRUE(
    drill::spin_until({node}, [&logs]() {return logs.contains("Hello world!");}, 4s))
    << "\"Hello earth!\" の後、4 秒待っても \"Hello world!\" に戻りませんでした。\n"
    << "  公式チュートリアルのポイントです。コールバックの最後で\n"
    << "  std::vector<rclcpp::Parameter> all_new_parameters{\n"
    << "    rclcpp::Parameter(\"my_parameter\", \"world\")};\n"
    << "  this->set_parameters(all_new_parameters); を呼んで \"my_parameter\" を"
    << "\"world\" に戻していますか？\n"
    << "  実際に出ていたログ:" << logs.dump();

  EXPECT_EQ(node->get_parameter("my_parameter").as_string(), "world")
    << "ログには \"Hello world!\" が出ましたが、ノード自身の \"my_parameter\" の値が"
    << "\"world\" に戻っていません。set_parameters() を呼びましたか？\n"
    << "実際の値: \"" << node->get_parameter("my_parameter").as_string() << "\"";
}
