// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <chrono>
#include <memory>

#include <example_interfaces/srv/add_two_ints.hpp>
#include <rclcpp/rclcpp.hpp>

/// 公式チュートリアル
/// 「Writing a simple service and client (C++)」のクライアント側。
///
/// 公式との違いは 2 点です。
///   1. main() を別ファイル（src/client_main.cpp）に分けている。
///   2. 公式は main() の中で「サービスを待つ」「リクエストを送る」を
///      直接書きますが、テストから使えるように wait_for_server() /
///      send_request() という 2 つのメソッドに切り出しています。
class AddTwoIntsClient : public rclcpp::Node
{
public:
  using AddTwoInts = example_interfaces::srv::AddTwoInts;

  AddTwoIntsClient();

  /// サービスが立ち上がっているか、timeout の間だけ確認する。
  ///
  /// 公式の while ループの中身（wait_for_service の呼び出しとログ出力）に相当する。
  /// ループさせるかどうかは呼び出し側（main）の責任にしている。
  bool wait_for_server(std::chrono::nanoseconds timeout);

  /// a + b を計算してもらうリクエストを送る。
  ///
  /// 戻り値はまだ結果が届いていない状態の FutureAndRequestId。
  /// 呼び出し側が rclcpp::spin_until_future_complete() などで完了を待つこと。
  rclcpp::Client<AddTwoInts>::FutureAndRequestId send_request(int64_t a, int64_t b);

private:
  rclcpp::Client<AddTwoInts>::SharedPtr client_;
};
