#include "drill/filter_pipeline.hpp"

FilterPipeline::FilterPipeline(const rclcpp::NodeOptions & options)
: Node("filter_host", options),
  loader_("drill_18_pluginlib_yaml", "drill::VelocityFilter")
{
}

void FilterPipeline::configure()
{
  const auto stage_names = declare_parameter("filters", std::vector<std::string>{});
  for (const auto & stage_name : stage_names) {
    const auto plugin_name = declare_parameter(stage_name + ".plugin", std::string(""));
    try {
      auto filter = loader_.createSharedInstance(plugin_name);
      filter->initialize(shared_from_this(), stage_name);
      filters_.push_back(filter);
    } catch (const pluginlib::PluginlibException & error) {
      RCLCPP_ERROR(
        get_logger(), "Skipping stage '%s': could not load plugin '%s' (%s)",
        stage_name.c_str(), plugin_name.c_str(), error.what());
    }
  }

  publisher_ = create_publisher<geometry_msgs::msg::Twist>("cmd_vel_out", 10);
  subscription_ = create_subscription<geometry_msgs::msg::Twist>(
    "cmd_vel_in", 10,
    [this](geometry_msgs::msg::Twist::ConstSharedPtr message) {
      publisher_->publish(apply(*message, kCommandPeriodSeconds));
    });
}

geometry_msgs::msg::Twist FilterPipeline::apply(
  const geometry_msgs::msg::Twist & command, double dt_seconds)
{
  auto result = command;
  for (const auto & filter : filters_) {
    result = filter->filter(result, dt_seconds);
  }
  return result;
}

std::size_t FilterPipeline::filter_count() const
{
  return filters_.size();
}
