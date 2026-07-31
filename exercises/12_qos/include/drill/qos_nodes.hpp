// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

// QoS の 4 つの主要ポリシー
// ------------------------
// History      : Publisher/Subscription が内部にいくつメッセージを保持するかの
//                「数え方」。KEEP_LAST（直近 depth 件だけ残す）と
//                KEEP_ALL（リソースが許す限り全部残す）がある。
// Depth        : History が KEEP_LAST のときのキューの長さ。
//                古いメッセージから捨てられる。
// Reliability  : 届け方の保証。RELIABLE は再送してでも届ける（TCP に近い）。
//                BEST_EFFORT は届かなくても再送しない（UDP に近い。センサ向き）。
// Durability   : 「あとから来た購読者」に過去のメッセージを渡すかどうか。
//                VOLATILE は購読開始後に publish された分しか届かない（既定）。
//                TRANSIENT_LOCAL は Publisher 側が最後の値を保持しておき、
//                あとから subscribe してきた相手にも配り直す。
//
// publisher と subscription の QoS は「合っていないと繋がらない」契約です。
// 例えば片方が TRANSIENT_LOCAL でもう片方が VOLATILE だと discovery はできても
// 通信できません（non-cooperative sets とも呼ばれます）。

/// トピック "config" に std_msgs::msg::String を publish するノード。
///
/// QoS は KeepLast(1) + TRANSIENT_LOCAL + RELIABLE。
/// 「最後に publish した値を、あとから起動した購読者にも配る」ための設定。
class LatchedPublisher : public rclcpp::Node
{
public:
  LatchedPublisher();

  /// value を data に入れて publish する。
  void publish(const std::string & value);

  /// publisher_ が実際に採用した QoS（テスト用）。
  rclcpp::QoS actual_qos() const;

private:
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
};

/// トピック "config" を購読するノード。
///
/// LatchedPublisher と同じ QoS（KeepLast(1) + TRANSIENT_LOCAL + RELIABLE）で
/// 購読することで、起動が publish より後でも過去の値を受け取れる。
class LatchedSubscriber : public rclcpp::Node
{
public:
  LatchedSubscriber();

  /// 直近に受信した値。
  std::string last_received() const;

  /// これまでに受信した件数。
  std::size_t count() const;

private:
  void topic_callback(const std_msgs::msg::String & msg);

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
  std::string last_received_;
  std::size_t count_{0};
};
