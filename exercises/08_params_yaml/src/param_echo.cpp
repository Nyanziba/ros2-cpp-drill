// このファイルは編集しません。
//
// YAML から読み込んだパラメータをログに出してすぐ終了するだけの的（まと）です。
// テストはこのノードを
//   ros2 run drill_08_params_yaml param_echo --ros-args --params-file <受講者のYAML>
// の形で起動し、標準出力（ログ）に出た値を見て採点します。
//
// 出力される値がすべて既定値のままなら、YAML が正しく読み込めていません。
#include <cstdio>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("param_echo");

  node->declare_parameter<std::string>("my_parameter", "world");
  node->declare_parameter<int64_t>("an_int_param", 0);
  node->declare_parameter<double>("a_double_param", 0.0);
  node->declare_parameter<std::vector<std::string>>("a_string_list", std::vector<std::string>{});

  const std::string my_parameter = node->get_parameter("my_parameter").as_string();
  const int64_t an_int_param = node->get_parameter("an_int_param").as_int();
  const double a_double_param = node->get_parameter("a_double_param").as_double();
  const std::vector<std::string> a_string_list =
    node->get_parameter("a_string_list").as_string_array();

  RCLCPP_INFO(node->get_logger(), "my_parameter=%s", my_parameter.c_str());
  RCLCPP_INFO(node->get_logger(), "an_int_param=%ld", static_cast<long>(an_int_param));

  // 小数第1位まで固定で出す（例: 1.5）。
  char double_buf[64];
  std::snprintf(double_buf, sizeof(double_buf), "%.1f", a_double_param);
  RCLCPP_INFO(node->get_logger(), "a_double_param=%s", double_buf);

  std::string joined;
  for (size_t i = 0; i < a_string_list.size(); ++i) {
    if (i != 0) {
      joined += ",";
    }
    joined += a_string_list[i];
  }
  RCLCPP_INFO(node->get_logger(), "a_string_list=[%s]", joined.c_str());

  rclcpp::shutdown();
  return 0;
}
