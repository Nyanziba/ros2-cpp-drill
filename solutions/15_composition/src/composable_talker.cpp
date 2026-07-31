#include "drill/composable_talker.hpp"

using namespace std::chrono_literals;

ComposableTalker::ComposableTalker(const rclcpp::NodeOptions & options)
: Node("composable_talker", options), count_(0)
{
  publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);
  timer_ = this->create_wall_timer(
    200ms, std::bind(&ComposableTalker::timer_callback, this));
}

void ComposableTalker::timer_callback()
{
  auto message = std_msgs::msg::String();
  message.data = "composable hello " + std::to_string(count_++);
  RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
  publisher_->publish(message);
}

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(ComposableTalker)
