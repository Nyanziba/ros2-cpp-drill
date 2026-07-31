// このファイルは編集しません。
//
// 公式チュートリアルではクラスと同じファイルに main() を書きますが、
// テストからクラスを直接使えるように分けています。
//
//   ros2 run drill_06_parameters minimal_param_node
#include <memory>

#include "drill/minimal_param.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MinimalParam>());
  rclcpp::shutdown();
  return 0;
}
