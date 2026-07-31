// このファイルは編集しません（採点用）。
#include <atomic>
#include <chrono>
#include <memory>
#include <vector>

#include "drill/fibonacci_action_server.hpp"
#include "drill_harness.hpp"

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

TEST_F(DrillTest, fibonacciアクションサーバを公開している)
{
  auto server = std::make_shared<FibonacciActionServer>();
  Probe probe;

  ASSERT_TRUE(wait_for_server(server, probe))
    << "\"fibonacci\" アクションサーバが3秒待っても見つかりませんでした。\n"
    << "  - コンストラクタで rclcpp_action::create_server<Fibonacci>(this, \"fibonacci\", ...)"
    << " の戻り値を action_server_ に入れましたか？";
}

TEST_F(DrillTest, order5の目標を送るとフィボナッチ数列を返す)
{
  auto server = std::make_shared<FibonacciActionServer>();
  Probe probe;

  ASSERT_TRUE(wait_for_server(server, probe))
    << "\"fibonacci\" アクションサーバが見つかりませんでした。";

  Fibonacci::Goal goal_msg;
  goal_msg.order = 5;

  auto goal_handle_future = probe.client->async_send_goal(goal_msg);
  ASSERT_TRUE(
    drill::spin_until_multithreaded(
      {server, probe.node},
      [&goal_handle_future]() {
        return goal_handle_future.wait_for(0s) == std::future_status::ready;
      }, 3s))
    << "目標の送信に3秒待っても応答がありませんでした。\n"
    << "  - handle_goal で ACCEPT_AND_EXECUTE を返していますか？";

  auto goal_handle = goal_handle_future.get();
  ASSERT_NE(goal_handle, nullptr)
    << "目標が拒否されました（goal_handle が null）。handle_goal の戻り値を確認してください。";

  auto result_future = probe.client->async_get_result(goal_handle);
  ASSERT_TRUE(
    drill::spin_until_multithreaded(
      {server, probe.node},
      [&result_future]() {
        return result_future.wait_for(0s) == std::future_status::ready;
      }, 4s))
    << "実行結果が4秒待っても返ってきませんでした。\n"
    << "  - handle_accepted で execute を別スレッドに投げていますか？\n"
    << "  - execute の最後で goal_handle->succeed(result) を呼んでいますか？";

  auto wrapped_result = result_future.get();
  ASSERT_EQ(wrapped_result.code, rclcpp_action::ResultCode::SUCCEEDED)
    << "結果コードが SUCCEEDED になっていません。execute() の最後まで到達していますか？";

  const std::vector<int32_t> expected = {0, 1, 1, 2, 3, 5};
  EXPECT_EQ(wrapped_result.result->sequence, expected)
    << "sequence が {0, 1, 1, 2, 3, 5} になっていません。\n"
    << "  sequence.push_back(sequence[i] + sequence[i - 1]); を i = 1 から"
    << " order - 1 まで繰り返していますか？";
}

TEST_F(DrillTest, 実行中にfeedbackが1回以上届く)
{
  auto server = std::make_shared<FibonacciActionServer>();
  Probe probe;

  ASSERT_TRUE(wait_for_server(server, probe))
    << "\"fibonacci\" アクションサーバが見つかりませんでした。";

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
    << "目標の送信に応答がありませんでした。";

  auto goal_handle = goal_handle_future.get();
  ASSERT_NE(goal_handle, nullptr) << "目標が拒否されました。";

  auto result_future = probe.client->async_get_result(goal_handle);
  ASSERT_TRUE(
    drill::spin_until_multithreaded(
      {server, probe.node},
      [&result_future]() {
        return result_future.wait_for(0s) == std::future_status::ready;
      }, 4s))
    << "実行結果が返ってきませんでした。";

  EXPECT_GE(feedback_count.load(), 1)
    << "feedback が1回も届きませんでした。\n"
    << "  - execute の中で goal_handle->publish_feedback(feedback) を呼んでいますか？";
}

TEST_F(DrillTest, キャンセル要求を受理する)
{
  auto server = std::make_shared<FibonacciActionServer>();
  Probe probe;

  ASSERT_TRUE(wait_for_server(server, probe))
    << "\"fibonacci\" アクションサーバが見つかりませんでした。";

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
    << "目標の送信に応答がありませんでした。";

  auto goal_handle = goal_handle_future.get();
  ASSERT_NE(goal_handle, nullptr) << "目標が拒否されました。";

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
    << "5秒待ってもキャンセル後の結果が返ってきませんでした。\n"
    << "  - execute の中で goal_handle->publish_feedback(feedback) を呼んでいますか？\n"
    << "  - handle_cancel で ACCEPT を返していますか？\n"
    << "  - execute のループの中で goal_handle->is_canceling() を確認していますか？";

  auto cancel_response = cancel_future.get();
  EXPECT_EQ(cancel_response->return_code, CancelResponse::ERROR_NONE)
    << "キャンセル要求が受理されませんでした（return_code="
    << static_cast<int>(cancel_response->return_code) << "）。\n"
    << "  - handle_cancel で ACCEPT を返していますか？";

  auto wrapped_result = result_future.get();
  EXPECT_EQ(wrapped_result.code, rclcpp_action::ResultCode::CANCELED)
    << "結果コードが CANCELED になっていません。\n"
    << "  - is_canceling() が true のとき"
    << " result->sequence = sequence; goal_handle->canceled(result); を呼んでいますか？";
}
