// I AM NOT DONE
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
  // TODO: メンバを宣言すること。
  //   - pluginlib::ClassLoader<drill::VelocityFilter>   読み込む係
  //   - std::shared_ptr<drill::VelocityFilter>          選んだフィルタ（空 = 通過）
  // メンバは宣言の逆順に壊れる。どちらを先に宣言するかで、破棄のときに警告が出るかどうかが決まる。
};
