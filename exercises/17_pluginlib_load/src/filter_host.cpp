// I AM NOT DONE
#include "drill/filter_host.hpp"

FilterHost::FilterHost(const rclcpp::NodeOptions & options)
: Node("filter_host", options)
{
  // TODO: ClassLoader を、メンバの初期化子リストで作ること。
  //         loader_("drill_17_pluginlib_load", "drill::VelocityFilter")
  //       第 1 引数は基底クラスがあるパッケージ、第 2 引数は基底クラスの名前。
}

void FilterHost::configure()
{
  // TODO: パラメータ "filter_type"（文字列。既定は空）を宣言して読み、その名前のプラグインを
  //       loader_.createSharedInstance(名前) で作り、
  //       initialize(shared_from_this(), kFilterInstanceName) を呼んでメンバに入れること。
  //
  // TODO: 名前が見つからないなど pluginlib::PluginlibException が飛んだら、
  //       受けて、RCLCPP_ERROR で「どの名前が読み込めなかったか」と e.what() をログに出し、
  //       フィルタは空のまま（通過）にすること。ノードは落とさない。
  //       使える名前の一覧も出すと、打ち間違いにすぐ気づける。
}

geometry_msgs::msg::Twist FilterHost::apply(
  const geometry_msgs::msg::Twist & command, double dt_seconds)
{
  // TODO: フィルタがあれば filter(command, dt_seconds) の結果を返し、
  //       空なら command をそのまま返すこと。
  (void)dt_seconds;
  return command;
}

std::vector<std::string> FilterHost::available_filter_names()
{
  // TODO: loader_.getDeclaredClasses() を返すこと。
  return {};
}
