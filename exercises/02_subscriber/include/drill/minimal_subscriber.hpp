// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

/// 公式チュートリアル
/// 「Writing a simple publisher and subscriber (C++)」の MinimalSubscriber。
///
/// 公式との違いは main() を別ファイル（src/listener_main.cpp）に分けている点だけです。
/// テストがこのクラスを直接生成して検証するためです。
class MinimalSubscriber : public rclcpp::Node
{
public:
  MinimalSubscriber();

private:
  void topic_callback(const std_msgs::msg::String & msg) const;

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
};
