// このファイルは編集しません。
//
// この課題の要が MultiThreadedExecutor です。SingleThreadedExecutor で回すと、
// トリガーのコールバックが応答を待っている間、応答を届けるコールバックが
// 実行される機会そのものがなくなり、デッドロックします（コールバックグループを
// 分けていても、Executor が 1 スレッドしか持っていなければ「同時に実行できる
// コールバックの数」が 1 のままだからです）。コールバックグループを分けるのは
// 「どのコールバックが同時に実行されてよいか」を Executor に教えるためであり、
// 実際に複数のコールバックを並行して動かすには、それを実行できるだけの
// スレッドを持つ MultiThreadedExecutor が必要です。
//
//   ros2 run drill_13_executors relay_with_service
#include <memory>

#include "drill/relay_with_service.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<RelayWithService>();

  // SingleThreadedExecutor ではこの課題は動かない。
  rclcpp::executors::MultiThreadedExecutor exec;
  exec.add_node(node);
  exec.spin();

  rclcpp::shutdown();
  return 0;
}
