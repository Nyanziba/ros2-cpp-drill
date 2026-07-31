#include "drill/minimal_publisher.hpp"

using namespace std::chrono_literals;

// I AM NOT DONE

MinimalPublisher::MinimalPublisher()
: Node("minimal_publisher"), count_(0)
{
  // TODO: "topic" に std_msgs::msg::String を流す Publisher を作り、publisher_ に入れること。
  //       QoS の depth は 10。使う API は create_publisher。

  // TODO: 500ms 周期のタイマを作り、timer_ に入れること。
  //       呼び出すのは下の timer_callback。使う API は create_wall_timer で、
  //       メンバ関数を渡すには std::bind が要る。
}

void MinimalPublisher::timer_callback()
{
  // TODO: std_msgs::msg::String を作り、data を "Hello, world! " と count_ をつなげた
  //       文字列にして publish すること。count_ は publish のたびに 1 増やす
  //       （1 通目が "Hello, world! 0" になるように）。
  //
  // TODO: 公式と同じログも出すこと。書式は  Publishing: '<本文>'  で、使うマクロは RCLCPP_INFO。
  //
  // コードの形が知りたければ ./drill hint 01
}
