// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

/// 公式チュートリアル
/// 「Writing a simple publisher and subscriber (C++)」の MinimalPublisher。
///
/// 公式との違いは main() を別ファイル（src/talker_main.cpp）に分けている点だけです。
/// テストがこのクラスを直接生成して検証するためです。
class MinimalPublisher : public rclcpp::Node
{
public:
  MinimalPublisher();

private:
  void timer_callback();

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  std::size_t count_;
};
