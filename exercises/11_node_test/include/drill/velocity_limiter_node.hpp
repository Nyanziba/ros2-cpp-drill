// このファイルは編集しません（完成済みの実物）。
#pragma once

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64.hpp>

/// limit_velocity() を呼ぶだけの薄いノード。
///
/// "cmd_vel_raw" (std_msgs::msg::Float64) を購読し、limit_velocity() を通した
/// 結果を "cmd_vel_limited" に publish する。パラメータ max_speed / max_delta を
/// 宣言しておき、コールバックのたびに読み直す。
///
/// ロジック（limit_velocity）が rclcpp::Node の外に切り出されているおかげで、
/// このノード自身がやっていることは「購読して、関数を呼んで、publish する」
/// だけの配線に薄くできる。ノードを起動して spin しなければ確かめられないのは
/// この配線部分だけで、境界値や符号といった本質的な計算のテストは
/// test/test_exercise.cpp 側（rclcpp 不要）で済んでいる。
class VelocityLimiterNode : public rclcpp::Node
{
public:
  VelocityLimiterNode();

private:
  void topic_callback(const std_msgs::msg::Float64 & msg);

  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr subscription_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr publisher_;
  double previous_;
};
