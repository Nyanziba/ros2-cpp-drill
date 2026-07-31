#include "drill/add_two_ints_client.hpp"

AddTwoIntsClient::AddTwoIntsClient()
: Node("add_two_ints_client")
{
  client_ = this->create_client<AddTwoInts>("add_two_ints");
}

bool AddTwoIntsClient::wait_for_server(std::chrono::nanoseconds timeout)
{
  if (client_->wait_for_service(timeout)) {
    return true;
  }

  if (!rclcpp::ok()) {
    RCLCPP_ERROR(this->get_logger(), "Interrupted while waiting for the service. Exiting.");
    return false;
  }

  RCLCPP_INFO(this->get_logger(), "service not available, waiting again...");
  return false;
}

rclcpp::Client<AddTwoIntsClient::AddTwoInts>::FutureAndRequestId
AddTwoIntsClient::send_request(int64_t a, int64_t b)
{
  auto request = std::make_shared<AddTwoInts::Request>();
  request->a = a;
  request->b = b;
  return client_->async_send_request(request);
}
