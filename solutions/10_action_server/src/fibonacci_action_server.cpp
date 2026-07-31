#include "drill/fibonacci_action_server.hpp"

#include <memory>
#include <thread>

using namespace std::chrono_literals;

FibonacciActionServer::FibonacciActionServer()
: Node("fibonacci_action_server")
{
  using namespace std::placeholders;

  action_server_ = rclcpp_action::create_server<Fibonacci>(
    this,
    "fibonacci",
    std::bind(&FibonacciActionServer::handle_goal, this, _1, _2),
    std::bind(&FibonacciActionServer::handle_cancel, this, _1),
    std::bind(&FibonacciActionServer::handle_accepted, this, _1));
}

rclcpp_action::GoalResponse FibonacciActionServer::handle_goal(
  const rclcpp_action::GoalUUID & uuid,
  std::shared_ptr<const Fibonacci::Goal> goal)
{
  RCLCPP_INFO(this->get_logger(), "Received goal request with order %d", goal->order);
  (void)uuid;
  return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
}

rclcpp_action::CancelResponse FibonacciActionServer::handle_cancel(
  const std::shared_ptr<GoalHandleFibonacci> goal_handle)
{
  RCLCPP_INFO(this->get_logger(), "Received request to cancel goal");
  (void)goal_handle;
  return rclcpp_action::CancelResponse::ACCEPT;
}

void FibonacciActionServer::handle_accepted(
  const std::shared_ptr<GoalHandleFibonacci> goal_handle)
{
  using namespace std::placeholders;
  // this needs to return quickly to avoid blocking the executor,
  // so spin up a new thread
  std::thread{std::bind(&FibonacciActionServer::execute, this, _1), goal_handle}.detach();
}

void FibonacciActionServer::execute(const std::shared_ptr<GoalHandleFibonacci> goal_handle)
{
  RCLCPP_INFO(this->get_logger(), "Executing goal");
  // 公式は 1 秒周期（rclcpp::Rate loop_rate(1)）だが、この課題では
  // テストを速く終わらせるため 20ms 周期にしている（唯一のロジック上の逸脱）。
  rclcpp::Rate loop_rate(20ms);
  const auto goal = goal_handle->get_goal();
  auto feedback = std::make_shared<Fibonacci::Feedback>();
  auto & sequence = feedback->partial_sequence;
  sequence.push_back(0);
  sequence.push_back(1);
  auto result = std::make_shared<Fibonacci::Result>();

  for (int i = 1; (i < goal->order) && rclcpp::ok(); ++i) {
    // Check if there is a cancel request
    if (goal_handle->is_canceling()) {
      result->sequence = sequence;
      goal_handle->canceled(result);
      RCLCPP_INFO(this->get_logger(), "Goal canceled");
      return;
    }
    // Update sequence
    sequence.push_back(sequence[i] + sequence[i - 1]);
    // Publish feedback
    goal_handle->publish_feedback(feedback);
    RCLCPP_INFO(this->get_logger(), "Publish feedback");

    loop_rate.sleep();
  }

  // Check if goal is done
  if (rclcpp::ok()) {
    result->sequence = sequence;
    goal_handle->succeed(result);
    RCLCPP_INFO(this->get_logger(), "Goal succeeded");
  }
}
