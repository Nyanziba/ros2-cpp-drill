// このファイルは編集しません。
//
//   ros2 run drill_11_node_test velocity_limiter_node
#include <memory>

#include "drill/velocity_limiter_node.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VelocityLimiterNode>());
  rclcpp::shutdown();
  return 0;
}
