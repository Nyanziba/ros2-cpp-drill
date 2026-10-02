// このファイルは編集しません（採点用）。
#include <atomic>
#include <chrono>
#include <memory>
#include <vector>

#include "drill/fibonacci_action_server.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using Fibonacci = FibonacciActionServer::Fibonacci;
using namespace std::chrono_literals;

namespace
{

/// probe ノードから "fibonacci" を呼ぶためのアクションクライアント一式。
struct Probe
{
  rclcpp::Node::SharedPtr node;
  rclcpp_action::Client<Fibonacci>::SharedPtr client;

  Probe()
  : node(rclcpp::Node::make_shared(drill::unique_name("probe")))
  {
    client = rclcpp_action::create_client<Fibonacci>(node, "fibonacci");
  }
};

/// server と probe を spin しながらアクションサーバの存在を待つ。
bool wait_for_server(const rclcpp::Node::SharedPtr & server, Probe & probe)
{
  return drill::spin_until(
    {server, probe.node},
    [&probe]() {return probe.client->wait_for_action_server(0s);}, 3s);
}

}  // namespace

TEST_F(DrillTest, ExposesFibonacciActionServer)
{
  auto server = std::make_shared<FibonacciActionServer>();
  Probe probe;

  ASSERT_TRUE(wait_for_server(server, probe))
    << drill::localized(
    "\"fibonacci\" アクションサーバが3秒待っても見つかりませんでした。\n"
    "  - コンストラクタで rclcpp_action::create_server<Fibonacci>(this, \"fibonacci\", ...)"
    " の戻り値を action_server_ に入れましたか？",
    "The \"fibonacci\" action server was not found after waiting 3 seconds.\n"
    "  - Did you assign the return value of rclcpp_action::create_server<Fibonacci>"
    "(this, \"fibonacci\", ...) to action_server_ in the constructor?");
}

TEST_F(DrillTest, Order5GoalReturnsFibonacciSequence)
{
  auto server = std::make_shared<FibonacciActionServer>();
  Probe probe;

  ASSERT_TRUE(wait_for_server(server, probe))
    << drill::localized(
    "\"fibonacci\" アクションサーバが見つかりませんでした。",
    "The \"fibonacci\" action server was not found.");

  Fibonacci::Goal goal_msg;
  goal_msg.order = 5;

  auto goal_handle_future = probe.client->async_send_goal(goal_msg);
  ASSERT_TRUE(
    drill::spin_until_multithreaded(
      {server, probe.node},
      [&goal_handle_future]() {
        return goal_handle_future.wait_for(0s) == std::future_status::ready;
      }, 3s))
    << drill::localized(
    "目標の送信に3秒待っても応答がありませんでした。\n"
    "  - handle_goal で ACCEPT_AND_EXECUTE を返していますか？",
    "No response to sending the goal after waiting 3 seconds.\n"
    "  - Does handle_goal return ACCEPT_AND_EXECUTE?");

  auto goal_handle = goal_handle_future.get();
  ASSERT_NE(goal_handle, nullptr)
    << drill::localized(
    "目標が拒否されました（goal_handle が null）。handle_goal の戻り値を確認してください。",
    "The goal was rejected (goal_handle is null). Check the return value of handle_goal.");

  auto result_future = probe.client->async_get_result(goal_handle);
  ASSERT_TRUE(
    drill::spin_until_multithreaded(
      {server, probe.node},
      [&result_future]() {
        return result_future.wait_for(0s) == std::future_status::ready;
      }, 4s))
    << drill::localized(
    "実行結果が4秒待っても返ってきませんでした。\n"
    "  - handle_accepted で execute を別スレッドに投げていますか？\n"
    "  - execute の最後で goal_handle->succeed(result) を呼んでいますか？",
    "The result did not come back after waiting 4 seconds.\n"
    "  - Does handle_accepted run execute on a separate thread?\n"
    "  - Does execute call goal_handle->succeed(result) at the end?");

  auto wrapped_result = result_future.get();
  ASSERT_EQ(wrapped_result.code, rclcpp_action::ResultCode::SUCCEEDED)
    << drill::localized(
    "結果コードが SUCCEEDED になっていません。execute() の最後まで到達していますか？",
    "The result code is not SUCCEEDED. Does execute() reach its end?");

  const std::vector<int32_t> expected = {0, 1, 1, 2, 3, 5};
  EXPECT_EQ(wrapped_result.result->sequence, expected)
    << drill::localized(
    "sequence が {0, 1, 1, 2, 3, 5} になっていません。\n"
    "  sequence.push_back(sequence[i] + sequence[i - 1]); を i = 1 から"
    " order - 1 まで繰り返していますか？",
    "sequence is not {0, 1, 1, 2, 3, 5}.\n"
    "  Do you repeat sequence.push_back(sequence[i] + sequence[i - 1]); "
    "from i = 1 to order - 1?");
}

TEST_F(DrillTest, DeliversFeedbackAtLeastOnceWhileRunning)
{
  auto server = std::make_shared<FibonacciActionServer>();
  Probe probe;

  ASSERT_TRUE(wait_for_server(server, probe))
    << drill::localized(
    "\"fibonacci\" アクションサーバが見つかりませんでした。",
    "The \"fibonacci\" action server was not found.");

  Fibonacci::Goal goal_msg;
  goal_msg.order = 5;

  std::atomic<int> feedback_count{0};
  rclcpp_action::Client<Fibonacci>::SendGoalOptions options;
  options.feedback_callback =
    [&feedback_count](
    rclcpp_action::ClientGoalHandle<Fibonacci>::SharedPtr,
    const std::shared_ptr<const Fibonacci::Feedback>) {
      feedback_count++;
    };

  auto goal_handle_future = probe.client->async_send_goal(goal_msg, options);
  ASSERT_TRUE(
    drill::spin_until_multithreaded(
      {server, probe.node},
      [&goal_handle_future]() {
        return goal_handle_future.wait_for(0s) == std::future_status::ready;
      }, 3s))
    << drill::localized("目標の送信に応答がありませんでした。", "No response to sending the goal.");

  auto goal_handle = goal_handle_future.get();
  ASSERT_NE(goal_handle, nullptr)
    << drill::localized("目標が拒否されました。", "The goal was rejected.");

  auto result_future = probe.client->async_get_result(goal_handle);
  ASSERT_TRUE(
    drill::spin_until_multithreaded(
      {server, probe.node},
      [&result_future]() {
        return result_future.wait_for(0s) == std::future_status::ready;
      }, 4s))
    << drill::localized("実行結果が返ってきませんでした。", "The result did not come back.");

  EXPECT_GE(feedback_count.load(), 1)
    << drill::localized(
    "feedback が1回も届きませんでした。\n"
    "  - execute の中で goal_handle->publish_feedback(feedback) を呼んでいますか？",
    "No feedback arrived.\n"
    "  - Does execute call goal_handle->publish_feedback(feedback)?");
}

TEST_F(DrillTest, AcceptsCancelRequest)
{
  auto server = std::make_shared<FibonacciActionServer>();
  Probe probe;

  ASSERT_TRUE(wait_for_server(server, probe))
    << drill::localized(
    "\"fibonacci\" アクションサーバが見つかりませんでした。",
    "The \"fibonacci\" action server was not found.");

  Fibonacci::Goal goal_msg;
  goal_msg.order = 50;  // 20ms周期 x 49 ステップ。すぐには終わらない長さにしておく。

  std::atomic<int> feedback_count{0};
  rclcpp_action::Client<Fibonacci>::SendGoalOptions options;
  options.feedback_callback =
    [&feedback_count](
    rclcpp_action::ClientGoalHandle<Fibonacci>::SharedPtr,
    const std::shared_ptr<const Fibonacci::Feedback>) {
      feedback_count++;
    };

  auto goal_handle_future = probe.client->async_send_goal(goal_msg, options);
  ASSERT_TRUE(
    drill::spin_until_multithreaded(
      {server, probe.node},
      [&goal_handle_future]() {
        return goal_handle_future.wait_for(0s) == std::future_status::ready;
      }, 3s))
    << drill::localized("目標の送信に応答がありませんでした。", "No response to sending the goal.");

  auto goal_handle = goal_handle_future.get();
  ASSERT_NE(goal_handle, nullptr)
    << drill::localized("目標が拒否されました。", "The goal was rejected.");

  // feedback が1回でも届いたらキャンセルを送り、その結果を待つ。
  // Executor を何度も作り直すと rclcpp 内部の状態が不安定になることがあるため、
  // 1回の spin セッションの中で「feedback を待つ → キャンセルする → 結果を待つ」
  // を tick で状態遷移させる。
  using CancelResponse = rclcpp_action::Client<Fibonacci>::CancelResponse;
  std::shared_future<CancelResponse::SharedPtr> cancel_future;
  std::shared_future<rclcpp_action::ClientGoalHandle<Fibonacci>::WrappedResult> result_future;
  bool cancel_sent = false;

  auto tick = [&]() {
      if (!cancel_sent && feedback_count.load() >= 1) {
        cancel_future = probe.client->async_cancel_goal(goal_handle);
        result_future = probe.client->async_get_result(goal_handle);
        cancel_sent = true;
      }
    };
  auto cond = [&]() {
      return cancel_sent &&
             cancel_future.wait_for(0s) == std::future_status::ready &&
             result_future.wait_for(0s) == std::future_status::ready;
    };

  ASSERT_TRUE(drill::spin_until_multithreaded({server, probe.node}, cond, 5s, tick))
    << drill::localized(
    "5秒待ってもキャンセル後の結果が返ってきませんでした。\n"
    "  - execute の中で goal_handle->publish_feedback(feedback) を呼んでいますか？\n"
    "  - handle_cancel で ACCEPT を返していますか？\n"
    "  - execute のループの中で goal_handle->is_canceling() を確認していますか？",
    "The result after the cancel did not come back after waiting 5 seconds.\n"
    "  - Does execute call goal_handle->publish_feedback(feedback)?\n"
    "  - Does handle_cancel return ACCEPT?\n"
    "  - Does the loop in execute check goal_handle->is_canceling()?");

  auto cancel_response = cancel_future.get();
  EXPECT_EQ(cancel_response->return_code, CancelResponse::ERROR_NONE)
    << drill::localized(
    "キャンセル要求が受理されませんでした（return_code=",
    "The cancel request was not accepted (return_code=")
    << static_cast<int>(cancel_response->return_code)
    << drill::localized(
    "）。\n"
    "  - handle_cancel で ACCEPT を返していますか？",
    ").\n"
    "  - Does handle_cancel return ACCEPT?");

  auto wrapped_result = result_future.get();
  EXPECT_EQ(wrapped_result.code, rclcpp_action::ResultCode::CANCELED)
    << drill::localized(
    "結果コードが CANCELED になっていません。\n"
    "  - is_canceling() が true のとき"
    " result->sequence = sequence; goal_handle->canceled(result); を呼んでいますか？",
    "The result code is not CANCELED.\n"
    "  - When is_canceling() is true, do you call"
    " result->sequence = sequence; goal_handle->canceled(result); ?");
}
