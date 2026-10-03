// このファイルは編集しません（プラグインの基底クラスの提示）。
#pragma once

#include <string>

#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>

namespace drill
{

/// cmd_vel（速度指令）にかけるフィルタの基底クラス。pluginlib で差し替える部品になる。
///
/// 扱うのは linear.x と angular.z だけ。
class VelocityFilter
{
public:
  virtual ~VelocityFilter() = default;

  /// pluginlib はプラグインを「引数なしのコンストラクタ」で作る。
  /// コンストラクタ引数は渡せないので、設定はここで受け取る。
  ///
  /// name は YAML の名前空間（例: "clamp"）。パラメータは "<name>.<項目>" で宣言する。
  virtual void initialize(const rclcpp::Node::SharedPtr & node, const std::string & name) = 0;

  /// dt_seconds は前回の指令からの経過時間。
  virtual geometry_msgs::msg::Twist filter(
    const geometry_msgs::msg::Twist & command, double dt_seconds) = 0;

protected:
  VelocityFilter() = default;
};

}  // namespace drill
