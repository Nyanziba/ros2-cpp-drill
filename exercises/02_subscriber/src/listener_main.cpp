// このファイルは編集しません。
//
// 公式チュートリアルではクラスと同じファイルに main() を書きますが、
// テストからクラスを直接使えるように分けています。
//
//   ros2 run drill_02_subscriber listener
#include <memory>

#include "drill/minimal_subscriber.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MinimalSubscriber>());
  rclcpp::shutdown();
  return 0;
}
