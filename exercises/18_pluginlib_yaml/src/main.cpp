// このファイルは編集しません（ros2 run で動かすための main）。
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "drill/filter_pipeline.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto pipeline = std::make_shared<FilterPipeline>(rclcpp::NodeOptions());
  pipeline->configure();
  rclcpp::spin(pipeline);
  rclcpp::shutdown();
  return 0;
}
