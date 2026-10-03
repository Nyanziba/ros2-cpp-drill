#pragma once

#include <memory>
#include <string>
#include <vector>

#include <geometry_msgs/msg/twist.hpp>
#include <pluginlib/class_loader.hpp>
#include <rclcpp/rclcpp.hpp>

#include "drill/velocity_filter.hpp"

/// plugin に渡す名前空間。パラメータは "filter.<項目>"（例: filter.max_linear）になる。
inline constexpr char kFilterInstanceName[] = "filter";

/// パラメータ filter_type で選んだ 1 つのフィルタ（pluginlib のプラグイン）を、
/// 速度指令にかけるノード。
///
/// 使い方:
///   auto host = std::make_shared<FilterHost>(options);
///   host->configure();          // プラグインの読み込みはここ
///
/// なぜコンストラクタで読み込まないのか: プラグインの initialize は
/// rclcpp::Node::SharedPtr を受け取る。shared_from_this() は、make_shared が終わって
/// 自分が shared_ptr に入った後でないと使えない（コンストラクタの中では例外になる）。
class FilterHost : public rclcpp::Node
{
public:
  explicit FilterHost(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

  /// プラグインを読み込む。作った直後に 1 度だけ呼ぶ。
  void configure();

  /// 選んだフィルタを通す。読み込めなかったとき（フィルタが空）は、command をそのまま返す。
  geometry_msgs::msg::Twist apply(const geometry_msgs::msg::Twist & command, double dt_seconds);

  /// 使えるプラグインの名前の一覧（ClassLoader の getDeclaredClasses）。
  std::vector<std::string> available_filter_names();

private:
  // 宣言の順番が大事。メンバは宣言の逆順に壊れるので、
  // ClassLoader を先に宣言すれば、フィルタが先に壊れ、ClassLoader が最後に壊れる。
  // 逆にすると、フィルタが残っているのに ClassLoader が先に壊れ、class_loader が警告
  // （SEVERE WARNING）を出す。未定義の動作になりうる。
  pluginlib::ClassLoader<drill::VelocityFilter> loader_;
  std::shared_ptr<drill::VelocityFilter> filter_;
};
