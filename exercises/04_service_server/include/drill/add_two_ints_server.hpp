// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>

#include <example_interfaces/srv/add_two_ints.hpp>
#include <rclcpp/rclcpp.hpp>

/// 公式チュートリアル
/// 「Writing a simple service and client (C++)」のサーバ側。
///
/// 公式は main() の中で rclcpp::Node::make_shared("add_two_ints_server") と
/// 自由関数 add を書きますが、テストからノードを直接生成して検証できるように
/// クラスにまとめています（公式の examples_rclcpp_minimal_service と同じ形）。
/// main() を別ファイル（src/server_main.cpp）に分けている点も含め、
/// これが公式との唯一の逸脱です。ロジック自体は公式と同一です。
class AddTwoIntsServer : public rclcpp::Node
{
public:
  using AddTwoInts = example_interfaces::srv::AddTwoInts;

  AddTwoIntsServer();

private:
  void add(
    const std::shared_ptr<AddTwoInts::Request> request,
    std::shared_ptr<AddTwoInts::Response> response);

  rclcpp::Service<AddTwoInts>::SharedPtr service_;
};
