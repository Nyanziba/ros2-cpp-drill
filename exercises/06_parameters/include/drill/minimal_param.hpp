// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/rclcpp.hpp>

/// 公式チュートリアル
/// 「Using parameters in a class (C++)」の MinimalParam。
///
/// 公式との違いは 2 点だけです。
///   1. main() を別ファイル（src/minimal_param_main.cpp）に分けている
///     （テストがこのクラスを直接生成して検証するため）。
///   2. タイマのコールバックを、公式のようにコンストラクタ内のラムダではなく
///     メンバ関数 timer_callback() にしている
///     （テストから呼び出しやすくするため。中身は公式と同一）。
class MinimalParam : public rclcpp::Node
{
public:
  MinimalParam();

private:
  void timer_callback();

  rclcpp::TimerBase::SharedPtr timer_;
};
