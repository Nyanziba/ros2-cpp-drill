// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstdint>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "drill_03_custom_interface/msg/num.hpp"
#include "drill_03_custom_interface/srv/add_three_ints.hpp"

/// 公式チュートリアル「Custom ROS 2 interfaces」で定義する Num / AddThreeInts を
/// 実際に使うノード。
///
/// 公式は Num を publish する例（Python、練習問題の解答）と、AddThreeInts を
/// 使うサービスの例（別章）を別々に示していますが、1課題1パッケージ・
/// 1課題1概念の方針を崩さないため、この課題では両方を1つのノードにまとめて
/// あります。それぞれは独立した機能なので、担当するメンバ関数も分けています。
class NumNode : public rclcpp::Node
{
public:
  using Num = drill_03_custom_interface::msg::Num;
  using AddThreeInts = drill_03_custom_interface::srv::AddThreeInts;

  NumNode();

private:
  /// "num" トピックに Num を publish するタイマコールバック。
  void timer_callback();

  /// "add_three_ints" サービスのハンドラ。
  void add_three_ints(
    const std::shared_ptr<AddThreeInts::Request> request,
    std::shared_ptr<AddThreeInts::Response> response);

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<Num>::SharedPtr publisher_;
  rclcpp::Service<AddThreeInts>::SharedPtr service_;
  std::int64_t count_;
};
