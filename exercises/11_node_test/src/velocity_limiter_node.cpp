// このファイルは編集しません（完成済みの実物）。
#include "drill/velocity_limiter_node.hpp"

#include <functional>

#include "drill/velocity_limiter.hpp"

using std::placeholders::_1;

VelocityLimiterNode::VelocityLimiterNode()
: Node("velocity_limiter_node"), previous_(0.0)
{
  this->declare_parameter("max_speed", 1.0);
  this->declare_parameter("max_delta", 0.5);

  publisher_ = this->create_publisher<std_msgs::msg::Float64>("cmd_vel_limited", 10);
  subscription_ = this->create_subscription<std_msgs::msg::Float64>(
    "cmd_vel_raw", 10, std::bind(&VelocityLimiterNode::topic_callback, this, _1));
}

void VelocityLimiterNode::topic_callback(const std_msgs::msg::Float64 & msg)
{
  const double max_speed = this->get_parameter("max_speed").as_double();
  const double max_delta = this->get_parameter("max_delta").as_double();

  const double limited = limit_velocity(msg.data, previous_, max_speed, max_delta);
  previous_ = limited;

  std_msgs::msg::Float64 out;
  out.data = limited;
  publisher_->publish(out);

  RCLCPP_DEBUG(
    this->get_logger(), "cmd_vel: raw=%.3f -> limited=%.3f", msg.data, limited);
}
