// このファイルは編集しません。
//
// 公式チュートリアルではクラスと同じファイルに main() を書きますが、
// テストからクラスを直接使えるように分けています。
//
//   ros2 run drill_07_param_events parameter_event_handler
#include <memory>

#include "drill/node_with_parameters.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<NodeWithParameters>());
  rclcpp::shutdown();
  return 0;
}
