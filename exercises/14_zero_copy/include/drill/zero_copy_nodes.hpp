// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

// なぜ unique_ptr で渡すとコピーが消えるのか
// ------------------------------------------
// publisher_->publish(message) は「値」を渡すため、rclcpp はまずメッセージを
// コピーしてから中間バッファ（IntraProcessManager）に積みます。
// 一方 publisher_->publish(std::move(msg)) は unique_ptr の「所有権」を
// そのまま中間バッファへ渡すだけなので、コピーは一切発生しません。
// プロセス内通信（NodeOptions().use_intra_process_comms(true)）が有効で、
// 購読側も ConstSharedPtr（または UniquePtr）でメッセージを受け取る場合、
// 送信側が確保したのと同じアドレスのメモリがそのまま購読側に届きます。
// 大きな画像や点群ではこの 1 回のコピーの有無が性能に直結するため、
// 「値で publish するか、unique_ptr を move して publish するか」は
// rclcpp を使いこなす上で重要な選択になります。

/// トピック "zero_copy" に std_msgs::msg::String を publish するノード。
///
/// publish_once() を呼ぶたびに、そのとき作ったメッセージ自身のアドレスを
/// data に書き込んでから publish する。プロセス内通信が効いていれば、
/// 購読側に届くメッセージのアドレスも同じになるはず。
class ZeroCopyTalker : public rclcpp::Node
{
public:
  explicit ZeroCopyTalker(const rclcpp::NodeOptions & options);

  /// 1 回 publish する。main のタイマやテストから呼ばれる。
  void publish_once();

  /// 直前に publish したメッセージのアドレス。
  std::uintptr_t last_published_address() const;

private:
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  std::uintptr_t last_published_address_{0};
};

/// トピック "zero_copy" を購読するノード。
///
/// コールバックの引数を std_msgs::msg::String::ConstSharedPtr で受けることで、
/// プロセス内通信が有効なときに届いたメッセージをコピーせずに扱える。
class ZeroCopyListener : public rclcpp::Node
{
public:
  explicit ZeroCopyListener(const rclcpp::NodeOptions & options);

  /// 直前に受信したメッセージのアドレス。
  std::uintptr_t last_received_address() const;

  /// 直前に受信したメッセージの data（アドレスの 16 進文字列）。
  std::string last_received_payload() const;

  /// これまでに受信した件数。
  std::size_t count() const;

private:
  void topic_callback(std_msgs::msg::String::ConstSharedPtr msg);

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
  std::uintptr_t last_received_address_{0};
  std::string last_received_payload_;
  std::size_t count_{0};
};
