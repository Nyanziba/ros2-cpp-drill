// このファイルは編集しません。
//
// 公式チュートリアルではクラスと同じファイルに main() を書きますが、
// テストからクラスを直接使えるように分けています。
//
//   ros2 run drill_05_service_client client 20 22
#include <cstdlib>
#include <memory>

#include "drill/add_two_ints_client.hpp"

using namespace std::chrono_literals;

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  if (argc != 3) {
    RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "usage: client X Y");
    rclcpp::shutdown();
    return 1;
  }

  auto node = std::make_shared<AddTwoIntsClient>();

  while (!node->wait_for_server(1s)) {
    if (!rclcpp::ok()) {
      rclcpp::shutdown();
      return 0;
    }
  }

  auto result = node->send_request(std::atoll(argv[1]), std::atoll(argv[2]));

  if (rclcpp::spin_until_future_complete(node, result) == rclcpp::FutureReturnCode::SUCCESS) {
    RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Sum: %ld", result.get()->sum);
  } else {
    RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Failed to call service add_two_ints");
  }

  rclcpp::shutdown();
  return 0;
}
