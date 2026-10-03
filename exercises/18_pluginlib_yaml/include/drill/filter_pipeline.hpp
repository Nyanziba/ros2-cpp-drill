// I AM NOT DONE
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <geometry_msgs/msg/twist.hpp>
#include <pluginlib/class_loader.hpp>
#include <rclcpp/rclcpp.hpp>

#include "drill/velocity_filter.hpp"

/// cmd_vel_in の周期を 20 Hz と決めうちにして、filter に渡す dt_seconds にする。
/// （本物は受信時刻の差を使うほうがよい。ここでは課題を絞るために固定にしている。）
inline constexpr double kCommandPeriodSeconds = 0.05;

/// YAML の filters と <name>.plugin から、pluginlib のプラグインの列を組む。
/// nav2 が controller や costmap の層を YAML の名前の列で組むのと同じ形。
///
///   filter_host:
///     ros__parameters:
///       filters: ["deadband", "clamp"]     # この順に通す
///       deadband:
///         plugin: "drill/DeadbandFilter"   # <name>.plugin が pluginlib の名前
///         linear_threshold: 0.05           # <name>.<項目> は、そのプラグイン自身が宣言する
///
/// 使い方:
///   auto pipeline = std::make_shared<FilterPipeline>(options);
///   pipeline->configure();      // プラグインの読み込みはここ（shared_from_this() が要るため）
class FilterPipeline : public rclcpp::Node
{
public:
  explicit FilterPipeline(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

  /// YAML を読んで、プラグインの列を組む。作った直後に 1 度だけ呼ぶ。
  void configure();

  /// 並んだ順にフィルタを通す。1 つも無ければ command をそのまま返す。
  geometry_msgs::msg::Twist apply(const geometry_msgs::msg::Twist & command, double dt_seconds);

  /// 実際に組み上がった段の数（読み込めなかった段は数えない）。
  std::size_t filter_count() const;

private:
  // TODO: メンバを宣言すること。
  //   - pluginlib::ClassLoader<drill::VelocityFilter>                   読み込む係
  //   - std::vector<std::shared_ptr<drill::VelocityFilter>>             組み上がった列
  //   - rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr      cmd_vel_in の購読者
  //   - rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr         cmd_vel_out の出し手
  // メンバは宣言の逆順に壊れる。ClassLoader を、フィルタの列より先に宣言すること。
};
