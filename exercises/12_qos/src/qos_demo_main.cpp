// このファイルは編集しません。
//
//   ros2 run drill_12_qos qos_demo
//
// publisher を先に作って 1 回だけ publish し、2 秒待ってから subscriber を
// 作る。QoS が TRANSIENT_LOCAL になっていれば、subscriber は起動が
// publish より後でも "Published config" の内容を受け取れる（ログで確認できる）。
#include <chrono>
#include <memory>
#include <thread>

#include "drill/qos_nodes.hpp"

using namespace std::chrono_literals;

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  auto publisher_node = std::make_shared<LatchedPublisher>();
  publisher_node->publish("hello from qos_demo");

  RCLCPP_INFO(
    publisher_node->get_logger(),
    "2 秒待ってから subscriber を起動します（TRANSIENT_LOCAL の効果を見るため）...");
  rclcpp::spin_some(publisher_node);
  std::this_thread::sleep_for(2s);

  auto subscriber_node = std::make_shared<LatchedSubscriber>();

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(publisher_node);
  executor.add_node(subscriber_node);
  executor.spin();

  rclcpp::shutdown();
  return 0;
}
