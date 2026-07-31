// このファイルは編集しません。
//
//   ros2 run drill_14_zero_copy zero_copy_demo
//
// ZeroCopyTalker と ZeroCopyListener を同一プロセス内に作り、
// use_intra_process_comms(true) を有効にして SingleThreadedExecutor で spin する。
// ログに出る "Published message with address" と "Received message with
// address" が毎回同じ値になっていれば、コピーが発生していない証拠。
#include <chrono>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "drill/zero_copy_nodes.hpp"

using namespace std::chrono_literals;

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  // プロセス内通信を有効にした NodeOptions。両方のノードに渡す。
  const auto options = rclcpp::NodeOptions().use_intra_process_comms(true);
  auto talker = std::make_shared<ZeroCopyTalker>(options);
  auto listener = std::make_shared<ZeroCopyListener>(options);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(talker);
  executor.add_node(listener);

  // 500ms ごとに publish_once() を呼ぶ。5 回 publish したら止める。
  auto remaining = std::make_shared<int>(5);
  rclcpp::TimerBase::SharedPtr timer;
  timer = talker->create_wall_timer(
    500ms,
    [talker, remaining, &executor]() {
      talker->publish_once();
      if (--(*remaining) <= 0) {
        executor.cancel();
      }
    });

  executor.spin();

  rclcpp::shutdown();
  return 0;
}
