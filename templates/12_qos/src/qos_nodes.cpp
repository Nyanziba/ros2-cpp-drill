#include "drill/qos_nodes.hpp"

#include <functional>

// I AM NOT DONE

LatchedPublisher::LatchedPublisher()
: Node("latched_publisher")
{
  // TODO: このノード用の QoS を組み立てること。
  //   - History     : KeepLast(1)
  //   - Durability  : TRANSIENT_LOCAL（あとから来た購読者にも最後の値を配る）
  //   - Reliability : RELIABLE
  //
  //   rclcpp::QoS qos(rclcpp::KeepLast(1));
  //   qos.???();   // durability を transient_local にするメンバ関数は？
  //   qos.???();   // reliability を reliable にするメンバ関数は？
  //
  // TODO: 組み立てた qos を渡して "config" に std_msgs::msg::String を流す
  //       Publisher を作り、publisher_ に入れること。
  //
  // 注意: LatchedSubscriber 側にも「まったく同じ」QoS を渡さないと、
  //       お互いを見つけても実際には繋がりません。
}

void LatchedPublisher::publish(const std::string & value)
{
  // TODO: std_msgs::msg::String を作り、data に value を入れて publish すること。

  // TODO: 公式と同じログも出すこと。書式は  Published config: '<値>'
  //
  // コードの形が知りたければ ./drill hint 12
  (void)value;
}

rclcpp::QoS LatchedPublisher::actual_qos() const
{
  // TODO: publisher_ が実際に採用した QoS を返すこと。
  //       要求した QoS と実効値は必ずしも一致しないため、テストは実効値を見ます。
  //       Publisher にそれを返すメンバ関数があります（get_ で始まります）。
  return rclcpp::QoS(1);
}

LatchedSubscriber::LatchedSubscriber()
: Node("latched_subscriber")
{
  // TODO: LatchedPublisher とまったく同じ QoS（KeepLast(1) + TRANSIENT_LOCAL +
  //       RELIABLE）を組み立て、"config" を購読する Subscription を作って
  //       subscription_ に入れること。コールバックは下の topic_callback。
}

void LatchedSubscriber::topic_callback(const std_msgs::msg::String & msg)
{
  // TODO: last_received_ に本文を入れ、count_ を 1 増やすこと。

  // TODO: 公式と同じログも出すこと。書式は  Received config: '<値>'
  //
  // コードの形が知りたければ ./drill hint 12
  (void)msg;
}

std::string LatchedSubscriber::last_received() const
{
  return last_received_;
}

std::size_t LatchedSubscriber::count() const
{
  return count_;
}
