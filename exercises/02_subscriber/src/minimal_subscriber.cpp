#include "drill/minimal_subscriber.hpp"

using std::placeholders::_1;

// I AM NOT DONE

MinimalSubscriber::MinimalSubscriber()
: Node("minimal_subscriber")
{
  // TODO: "topic" から std_msgs::msg::String を受け取る Subscription を作り、
  //       subscription_ に入れること。QoS の depth は 10。
  //       使う API は create_subscription で、コールバックには下の topic_callback を
  //       std::bind で結びつける（引数が 1 つあるので _1 が必要）。
  //
  // 戻り値を捨てるとどうなるかを考えてみてください。
}

void MinimalSubscriber::topic_callback(const std_msgs::msg::String & msg) const
{
  // TODO: 受け取った内容を公式と同じ書式でログに出すこと。
  //       書式は  I heard: '<受信した文字列>'  で、使うマクロは RCLCPP_INFO。
  //
  // コードの形が知りたければ ./drill hint 02
  (void)msg;
}
