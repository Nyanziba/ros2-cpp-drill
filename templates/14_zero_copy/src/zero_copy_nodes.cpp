#include "drill/zero_copy_nodes.hpp"

#include <sstream>
#include <utility>

// I AM NOT DONE

ZeroCopyTalker::ZeroCopyTalker(const rclcpp::NodeOptions & options)
: Node("zero_copy_talker", options)
{
  // TODO: "zero_copy" に std_msgs::msg::String を流す Publisher を作り、
  //       publisher_ に入れること（QoS の depth は 10）。使う API は create_publisher。
}

void ZeroCopyTalker::publish_once()
{
  // TODO: メッセージを std::unique_ptr で作ること（std::make_unique を使う）。
  //       ここが「値」や shared_ptr だと、この課題の主題であるゼロコピーになりません。

  // TODO: data に「そのメッセージ自身のアドレス」を 16 進文字列で入れること。
  //       アドレスは reinterpret_cast<std::uintptr_t>(...) で数値にし、
  //       std::ostringstream と std::hex で文字列にできます。

  // TODO: 同じアドレスを last_published_address_ にも記録し、ログに出すこと。
  //       書式は  Published message with address: 0x<アドレス>

  // TODO: publish すること。ただし**所有権を渡す形**で渡すこと（std::move）。
  //       値で渡すとここで 1 回コピーが起き、送信側と受信側のアドレスが
  //       一致しなくなります（＝ゼロコピーにならない）。
  //
  // コードの形が知りたければ ./drill hint 14
}

std::uintptr_t ZeroCopyTalker::last_published_address() const
{
  return last_published_address_;
}

ZeroCopyListener::ZeroCopyListener(const rclcpp::NodeOptions & options)
: Node("zero_copy_listener", options)
{
  // TODO: "zero_copy" を購読する Subscription を作り、subscription_ に入れること
  //       （QoS の depth は 10、コールバックは下の topic_callback）。
}

void ZeroCopyListener::topic_callback(std_msgs::msg::String::ConstSharedPtr msg)
{
  // TODO: 受け取ったメッセージのアドレス（msg.get() を uintptr_t にしたもの）を
  //       last_received_address_ に、本文を last_received_payload_ に記録し、
  //       count_ を 1 増やすこと。
  //
  //       引数の型は ConstSharedPtr で宣言済みです。変更しないでください。
  //       値や const 参照で受け直すと、せっかくコピーなしで届いたメッセージを
  //       ここでコピーしてしまいます。

  // TODO: 公式と同じログも出すこと。
  //       書式は  Received message with address: 0x<アドレス>
  //
  // コードの形が知りたければ ./drill hint 14
  (void)msg;
}

std::uintptr_t ZeroCopyListener::last_received_address() const
{
  return last_received_address_;
}

std::string ZeroCopyListener::last_received_payload() const
{
  return last_received_payload_;
}

std::size_t ZeroCopyListener::count() const
{
  return count_;
}
