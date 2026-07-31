#include "drill/qos_nodes.hpp"

#include <functional>

LatchedPublisher::LatchedPublisher()
: Node("latched_publisher")
{
  rclcpp::QoS qos(rclcpp::KeepLast(1));
  qos.transient_local();
  qos.reliable();

  publisher_ = this->create_publisher<std_msgs::msg::String>("config", qos);
}

void LatchedPublisher::publish(const std::string & value)
{
  std_msgs::msg::String message;
  message.data = value;
  publisher_->publish(message);
  RCLCPP_INFO(this->get_logger(), "Published config: '%s'", value.c_str());
}

rclcpp::QoS LatchedPublisher::actual_qos() const
{
  return publisher_->get_actual_qos();
}

LatchedSubscriber::LatchedSubscriber()
: Node("latched_subscriber")
{
  rclcpp::QoS qos(rclcpp::KeepLast(1));
  qos.transient_local();
  qos.reliable();

  subscription_ = this->create_subscription<std_msgs::msg::String>(
    "config", qos,
    std::bind(&LatchedSubscriber::topic_callback, this, std::placeholders::_1));
}

void LatchedSubscriber::topic_callback(const std_msgs::msg::String & msg)
{
  last_received_ = msg.data;
  ++count_;
  RCLCPP_INFO(this->get_logger(), "Received config: '%s'", msg.data.c_str());
}

std::string LatchedSubscriber::last_received() const
{
  return last_received_;
}

std::size_t LatchedSubscriber::count() const
{
  return count_;
}
