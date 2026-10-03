// I AM NOT DONE
#include "drill/filter_pipeline.hpp"

FilterPipeline::FilterPipeline(const rclcpp::NodeOptions & options)
: Node("filter_host", options)
{
  // TODO: ClassLoader を、メンバの初期化子リストで作ること。
  //         loader_("drill_18_pluginlib_yaml", "drill::VelocityFilter")
}

void FilterPipeline::configure()
{
  // TODO: パラメータ "filters"（文字列の配列。既定は空）を宣言して読むこと。
  //
  // TODO: 配列の名前 name それぞれについて、この順で、
  //         1. "<name>.plugin"（文字列）を宣言して読む
  //         2. loader_.createSharedInstance(その名前) でプラグインを作る
  //         3. initialize(shared_from_this(), name) を呼ぶ
  //            （プラグインが "<name>.<項目>" を自分で宣言して読む）
  //         4. 列の末尾に入れる
  //
  // TODO: 1 つ読み込むごとに try / catch で包むこと。pluginlib::PluginlibException が
  //       飛んだら、RCLCPP_ERROR で「どの段（name）の、どの plugin 名が、なぜ」読めなかったかを
  //       出して、その段だけ飛ばす。ほかの段は組み続けること（ノードは落とさない）。
  //
  // TODO: "cmd_vel_in" を購読し（QoS depth 10）、受け取った指令を
  //       apply(*message, kCommandPeriodSeconds) に通して、"cmd_vel_out" に publish すること。
}

geometry_msgs::msg::Twist FilterPipeline::apply(
  const geometry_msgs::msg::Twist & command, double dt_seconds)
{
  // TODO: 組み上がった列を、並んだ順に filter(結果, dt_seconds) で通して返すこと。
  (void)dt_seconds;
  return command;
}

std::size_t FilterPipeline::filter_count() const
{
  // TODO: 組み上がった列の長さを返すこと。
  return 0;
}
