#include "drill/minimal_param.hpp"

using namespace std::chrono_literals;

// I AM NOT DONE

MinimalParam::MinimalParam()
: Node("minimal_param_node")
{
  // TODO: "my_parameter" という文字列パラメータを、既定値 "world" で宣言すること。
  //       宣言には rcl_interfaces::msg::ParameterDescriptor を添えて、
  //       description を "This parameter is mine!" にすること。
  //       使う API は declare_parameter（3 引数の形）。

  // TODO: 1000ms 周期のタイマを作り、timer_ に入れること。
  //       呼び出すのは下の timer_callback。使う API は create_wall_timer と std::bind。
}

void MinimalParam::timer_callback()
{
  // TODO: "my_parameter" の値をその都度読み出して、"Hello <値>!" とログに出すこと。
  //       使う API は get_parameter と、文字列として取り出す as_string。

  // TODO: 公式チュートリアルの主旨がここです。呼ばれるたびに "my_parameter" を
  //       "world" に戻すこと。こうすると ros2 param set で変えても 1 秒後には
  //       "world" に戻ります（＝パラメータは保持せず毎回読むもの、という設計の確認）。
  //       使う API は set_parameters（std::vector<rclcpp::Parameter> を渡す形）。
  //
  // コードの形が知りたければ ./drill hint 06
}
