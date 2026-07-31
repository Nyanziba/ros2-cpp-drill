// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>

#include <action_tutorials_interfaces/action/fibonacci.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>

/// 公式チュートリアル
/// 「Writing an action server and client (C++)」のサーバ側。
///
/// 公式との違いは次の 2 点だけです。ロジック自体は公式と同一です。
///   - main() を別ファイル（src/fibonacci_action_server_main.cpp）に分けている点
///     （テストからクラスを直接使えるようにするため）
///   - execute() 内のループ周期を、公式の 1 秒（rclcpp::Rate loop_rate(1)）から
///     20ms に変えている点（テストを速く終わらせるため）
class FibonacciActionServer : public rclcpp::Node
{
public:
  using Fibonacci = action_tutorials_interfaces::action::Fibonacci;
  using GoalHandleFibonacci = rclcpp_action::ServerGoalHandle<Fibonacci>;

  FibonacciActionServer();

private:
  /// 目標を受けるか断るかを決める（ACCEPT_AND_EXECUTE / REJECT）。
  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID & uuid,
    std::shared_ptr<const Fibonacci::Goal> goal);

  /// キャンセル要求を受けるか断るかを決める（ACCEPT / REJECT）。
  rclcpp_action::CancelResponse handle_cancel(
    const std::shared_ptr<GoalHandleFibonacci> goal_handle);

  /// 受理された目標の実行を始める。
  ///
  /// ここは「すぐ返る」ことが大事。重い処理（execute の呼び出し）を
  /// このスレッドで直接やってはいけない。公式どおり別スレッドに投げること。
  void handle_accepted(const std::shared_ptr<GoalHandleFibonacci> goal_handle);

  /// 実際にフィボナッチ数列を計算し、途中経過を publish_feedback、
  /// 最後に succeed（またはキャンセル時は canceled）で結果を返す。
  void execute(const std::shared_ptr<GoalHandleFibonacci> goal_handle);

  rclcpp_action::Server<Fibonacci>::SharedPtr action_server_;
};
