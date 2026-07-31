#include "drill/add_two_ints_server.hpp"

using std::placeholders::_1;
using std::placeholders::_2;

AddTwoIntsServer::AddTwoIntsServer()
: Node("add_two_ints_server")
{
  service_ = this->create_service<AddTwoInts>(
    "add_two_ints", std::bind(&AddTwoIntsServer::add, this, _1, _2));
}

void AddTwoIntsServer::add(
  const std::shared_ptr<AddTwoInts::Request> request,
  std::shared_ptr<AddTwoInts::Response> response)
{
  response->sum = request->a + request->b;
  RCLCPP_INFO(
    this->get_logger(), "Incoming request\na: %ld b: %ld",
    request->a, request->b);
  RCLCPP_INFO(
    this->get_logger(), "sending back response: [%ld]",
    (long int)response->sum);
}
