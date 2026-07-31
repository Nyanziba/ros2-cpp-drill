// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstdint>
#include <memory>

#include <rclcpp/rclcpp.hpp>

/// 公式チュートリアル
/// 「Monitoring for parameter changes (C++)」の SampleNodeWithParameters。
///
/// 公式との違いは 2 点です。
///   1. main() を別ファイル（src/parameter_event_handler_main.cpp）に分けている
///     （テストがこのクラスを直接生成して検証するため）。
///   2. メンバ `latest_value_` とアクセサ `latest_value()` を追加している
///     （公式には無い）。コールバックが実際に呼ばれたことをテストから
///     確認するための仕掛けで、詳しくは README を参照。
class NodeWithParameters : public rclcpp::Node
{
public:
  NodeWithParameters();

  /// コールバックで最後に受け取った値（テスト用。公式には無いメンバ）。
  int64_t latest_value() const;

private:
  std::shared_ptr<rclcpp::ParameterEventHandler> param_subscriber_;
  std::shared_ptr<rclcpp::ParameterCallbackHandle> cb_handle_;
  int64_t latest_value_{0};
};
