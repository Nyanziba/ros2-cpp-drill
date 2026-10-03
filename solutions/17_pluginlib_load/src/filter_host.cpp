#include "drill/filter_host.hpp"

FilterHost::FilterHost(const rclcpp::NodeOptions & options)
: Node("filter_host", options),
  loader_("drill_17_pluginlib_load", "drill::VelocityFilter")
{
}

void FilterHost::configure()
{
  const auto filter_type = declare_parameter("filter_type", std::string(""));
  try {
    auto filter = loader_.createSharedInstance(filter_type);
    filter->initialize(shared_from_this(), kFilterInstanceName);
    filter_ = filter;
  } catch (const pluginlib::PluginlibException & error) {
    std::string available_names;
    for (const auto & name : available_filter_names()) {
      available_names += " " + name;
    }
    RCLCPP_ERROR(
      get_logger(), "Could not load filter_type '%s' (%s). Passing commands through unchanged. "
      "Available names:%s",
      filter_type.c_str(), error.what(), available_names.c_str());
    filter_.reset();
  }
}

geometry_msgs::msg::Twist FilterHost::apply(
  const geometry_msgs::msg::Twist & command, double dt_seconds)
{
  if (!filter_) {
    return command;
  }
  return filter_->filter(command, dt_seconds);
}

std::vector<std::string> FilterHost::available_filter_names()
{
  return loader_.getDeclaredClasses();
}
