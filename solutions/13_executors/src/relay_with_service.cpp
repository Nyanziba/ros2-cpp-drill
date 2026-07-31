#include "drill/relay_with_service.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

RelayWithService::RelayWithService()
: Node("relay_with_service")
{
  subscription_group_ = this->create_callback_group(
    rclcpp::CallbackGroupType::MutuallyExclusive);
  client_group_ = this->create_callback_group(
    rclcpp::CallbackGroupType::MutuallyExclusive);

  publisher_ = this->create_publisher<std_msgs::msg::Int32>("sum", 10);

  rclcpp::SubscriptionOptions options;
  options.callback_group = subscription_group_;
  subscription_ = this->create_subscription<std_msgs::msg::Int32>(
    "trigger", 10,
    std::bind(&RelayWithService::trigger_callback, this, _1),
    options);

  client_ = this->create_client<AddTwoInts>(
    "add_two_ints", rclcpp::ServicesQoS(), client_group_);
}

rclcpp::CallbackGroup::SharedPtr RelayWithService::subscription_group() const
{
  return subscription_group_;
}

rclcpp::CallbackGroup::SharedPtr RelayWithService::client_group() const
{
  return client_group_;
}

void RelayWithService::trigger_callback(const std_msgs::msg::Int32 & msg)
{
  auto request = std::make_shared<AddTwoInts::Request>();
  request->a = msg.data;
  request->b = msg.data;

  auto future = client_->async_send_request(request);

  if (future.wait_for(2s) != std::future_status::ready) {
    RCLCPP_ERROR(this->get_logger(), "add_two_ints の応答がタイムアウトしました");
    return;
  }

  std_msgs::msg::Int32 out;
  out.data = future.get()->sum;
  publisher_->publish(out);
}
