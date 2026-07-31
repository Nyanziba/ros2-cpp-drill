// このファイルは編集しません。
//
// 公式チュートリアルではクラスと同じファイルに main() を書きますが、
// テストからクラスを直接使えるように分けています（01・04と同じ方針）。
//
//   ros2 run drill_03_custom_interface num_node
#include <memory>

#include "drill/num_node.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<NumNode>());
  rclcpp::shutdown();
  return 0;
}
