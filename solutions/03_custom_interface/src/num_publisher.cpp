#include "drill/num_node.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;

NumNode::NumNode()
: Node("num_node"), count_(0)
{
  publisher_ = this->create_publisher<Num>("num", 10);
  timer_ = this->create_wall_timer(
    500ms, std::bind(&NumNode::timer_callback, this));

  service_ = this->create_service<AddThreeInts>(
    "add_three_ints", std::bind(&NumNode::add_three_ints, this, _1, _2));
}

void NumNode::timer_callback()
{
  auto message = Num();
  message.num = count_++;
  RCLCPP_INFO(this->get_logger(), "Publishing: '%ld'", (long int)message.num);
  publisher_->publish(message);
}

void NumNode::add_three_ints(
  const std::shared_ptr<AddThreeInts::Request> request,
  std::shared_ptr<AddThreeInts::Response> response)
{
  response->sum = request->a + request->b + request->c;
  RCLCPP_INFO(
    this->get_logger(), "Incoming request\na: %ld b: %ld c: %ld",
    request->a, request->b, request->c);
  RCLCPP_INFO(
    this->get_logger(), "sending back response: [%ld]",
    (long int)response->sum);
}
