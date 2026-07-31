#include "drill/fibonacci_action_server.hpp"

#include <memory>
#include <thread>

using namespace std::chrono_literals;

// I AM NOT DONE

FibonacciActionServer::FibonacciActionServer()
: Node("fibonacci_action_server")
{
  using namespace std::placeholders;

  // TODO: アクションサーバを作り、action_server_ に入れること。
  //       使う API は rclcpp_action::create_server<Fibonacci>。
  //       第1引数は this、第2引数はアクション名 "fibonacci"、
  //       そのあとに下の handle_goal / handle_cancel / handle_accepted を
  //       std::bind で（この順で）渡します。引数の数がそれぞれ違うので注意。
  //
  // コードの形が知りたければ ./drill hint 10
}

rclcpp_action::GoalResponse FibonacciActionServer::handle_goal(
  const rclcpp_action::GoalUUID & uuid,
  std::shared_ptr<const Fibonacci::Goal> goal)
{
  (void)uuid;
  (void)goal;

  // TODO: 公式と同じログを出すこと。
  //       書式は  Received goal request with order <order>

  // TODO: 「目標を受理して、すぐ実行する」を表す値を返すこと（今は REJECT を返しています）。
  //       rclcpp_action::GoalResponse に何が定義されているか見てみましょう。
  return rclcpp_action::GoalResponse::REJECT;
}

rclcpp_action::CancelResponse FibonacciActionServer::handle_cancel(
  const std::shared_ptr<GoalHandleFibonacci> goal_handle)
{
  (void)goal_handle;

  // TODO: 公式と同じログを出すこと。書式は  Received request to cancel goal

  // TODO: 「キャンセルを受理する」を表す値を返すこと（今は REJECT を返しています）。
  return rclcpp_action::CancelResponse::REJECT;
}

void FibonacciActionServer::handle_accepted(
  const std::shared_ptr<GoalHandleFibonacci> goal_handle)
{
  (void)goal_handle;

  // TODO: 下の execute() を実行し始めること。
  //
  //       ただしこの関数は「すぐ返る」ことが絶対条件です。ここで execute() を
  //       直接呼ぶと、Executor のスレッドが計算の間ずっと塞がれ、他のコールバック
  //       （キャンセル要求すら）処理できなくなります。
  //       公式と同じく別スレッドを立てて投げ、この関数はすぐ抜けること。
  //       立てたスレッドは detach します。
}

void FibonacciActionServer::execute(const std::shared_ptr<GoalHandleFibonacci> goal_handle)
{
  (void)goal_handle;

  // TODO: 公式の execute() と同じ流れを書くこと。
  //
  //   1. "Executing goal" をログに出す。
  //   2. ループの周期を作る。公式は rclcpp::Rate で 1 秒周期だが、
  //      この課題ではテストを速く終わらせるため 20ms 周期にすること。
  //   3. goal_handle から目標を取り出し、order を読む。
  //   4. feedback（Fibonacci::Feedback）と result（Fibonacci::Result）を用意し、
  //      数列を {0, 1} で始める。feedback には partial_sequence がある。
  //   5. i = 1 から i < order の間、次を繰り返す。
  //        - キャンセル要求が来ていたら（goal_handle に判定するメンバ関数がある）、
  //          そこまでの数列を result に入れてキャンセル完了を通知し、
  //          "Goal canceled" をログに出して return する。
  //        - 数列の末尾に「直前の 2 項の和」を追加する。
  //        - 途中経過を通知する（goal_handle に publish するメンバ関数がある）。
  //        - 周期分だけ待つ。
  //   6. 最後まで終えたら、数列を result に入れて成功を通知し、
  //      "Goal succeeded" をログに出す。
  //
  // コードの形が知りたければ ./drill hint 10
}
