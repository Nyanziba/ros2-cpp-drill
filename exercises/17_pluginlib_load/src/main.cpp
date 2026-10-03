// このファイルは編集しません（ros2 run で動かすための main）。
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "drill/filter_host.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto host = std::make_shared<FilterHost>(rclcpp::NodeOptions());
  host->configure();
  rclcpp::spin(host);
  rclcpp::shutdown();
  return 0;
}
