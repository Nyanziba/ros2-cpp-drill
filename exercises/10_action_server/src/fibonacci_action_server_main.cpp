// このファイルは編集しません。
//
// 公式チュートリアルではクラスと同じファイルに main() を書きますが、
// テストからクラスを直接使えるように分けています。
//
//   ros2 run drill_10_action_server fibonacci_action_server
#include <memory>

#include "drill/fibonacci_action_server.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<FibonacciActionServer>());
  rclcpp::shutdown();
  return 0;
}
