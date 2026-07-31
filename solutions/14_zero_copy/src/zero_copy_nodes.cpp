#include "drill/zero_copy_nodes.hpp"

#include <sstream>
#include <utility>

ZeroCopyTalker::ZeroCopyTalker(const rclcpp::NodeOptions & options)
: Node("zero_copy_talker", options)
{
  publisher_ = this->create_publisher<std_msgs::msg::String>("zero_copy", 10);
}

void ZeroCopyTalker::publish_once()
{
  auto msg = std::make_unique<std_msgs::msg::String>();

  std::ostringstream ss;
  ss << std::hex << reinterpret_cast<std::uintptr_t>(msg.get());
  msg->data = ss.str();

  last_published_address_ = reinterpret_cast<std::uintptr_t>(msg.get());
  RCLCPP_INFO(
    this->get_logger(), "Published message with address: 0x%lx", last_published_address_);

  publisher_->publish(std::move(msg));
}

std::uintptr_t ZeroCopyTalker::last_published_address() const
{
  return last_published_address_;
}

ZeroCopyListener::ZeroCopyListener(const rclcpp::NodeOptions & options)
: Node("zero_copy_listener", options)
{
  subscription_ = this->create_subscription<std_msgs::msg::String>(
    "zero_copy", 10,
    std::bind(&ZeroCopyListener::topic_callback, this, std::placeholders::_1));
}

void ZeroCopyListener::topic_callback(std_msgs::msg::String::ConstSharedPtr msg)
{
  last_received_address_ = reinterpret_cast<std::uintptr_t>(msg.get());
  last_received_payload_ = msg->data;
  ++count_;

  RCLCPP_INFO(
    this->get_logger(), "Received message with address: 0x%lx", last_received_address_);
}

std::uintptr_t ZeroCopyListener::last_received_address() const
{
  return last_received_address_;
}

std::string ZeroCopyListener::last_received_payload() const
{
  return last_received_payload_;
}

std::size_t ZeroCopyListener::count() const
{
  return count_;
}
