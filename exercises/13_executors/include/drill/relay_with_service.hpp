// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>

#include <example_interfaces/srv/add_two_ints.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>

/// "trigger"（std_msgs::msg::Int32）で受けた値 v を "add_two_ints" サービスに
/// a = v, b = v として投げ、その応答をコールバックの中で同期的に待ってから
/// 得られた sum を "sum"（std_msgs::msg::Int32）に publish するノード。
///
/// なぜコールバックグループを 2 つに分ける必要があるのか:
/// Executor は 1 つの MutuallyExclusive グループから同時に 1 つのコールバックしか
/// 実行しない。もしサブスクライバとクライアントが同じグループにいると、
/// トリガーのコールバックが応答を待っている間、そのグループの他のコールバック
/// （＝応答を届けるはずのクライアントの完了コールバック）は実行機会を得られない。
/// つまり自分自身が待っている相手を、自分自身がブロックしてしまう。
/// グループを分けて MultiThreadedExecutor で回せば、片方が待っている間にも
/// もう片方のスレッドが応答処理を進められる。
class RelayWithService : public rclcpp::Node
{
public:
  using AddTwoInts = example_interfaces::srv::AddTwoInts;

  RelayWithService();

  /// テスト用アクセサ: サブスクライバが属するコールバックグループ。
  rclcpp::CallbackGroup::SharedPtr subscription_group() const;

  /// テスト用アクセサ: サービスクライアントが属するコールバックグループ。
  rclcpp::CallbackGroup::SharedPtr client_group() const;

private:
  void trigger_callback(const std_msgs::msg::Int32 & msg);

  rclcpp::CallbackGroup::SharedPtr subscription_group_;
  rclcpp::CallbackGroup::SharedPtr client_group_;

  rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr subscription_;
  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr publisher_;
  rclcpp::Client<AddTwoInts>::SharedPtr client_;
};
