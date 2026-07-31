#include "drill/minimal_param.hpp"

using namespace std::chrono_literals;

MinimalParam::MinimalParam()
: Node("minimal_param_node")
{
  auto param_desc = rcl_interfaces::msg::ParameterDescriptor{};
  param_desc.description = "This parameter is mine!";
  this->declare_parameter("my_parameter", "world", param_desc);

  timer_ = this->create_wall_timer(
    1000ms, std::bind(&MinimalParam::timer_callback, this));
}

void MinimalParam::timer_callback()
{
  std::string my_param = this->get_parameter("my_parameter").as_string();

  RCLCPP_INFO(this->get_logger(), "Hello %s!", my_param.c_str());

  std::vector<rclcpp::Parameter> all_new_parameters{
    rclcpp::Parameter("my_parameter", "world")};
  this->set_parameters(all_new_parameters);
}
