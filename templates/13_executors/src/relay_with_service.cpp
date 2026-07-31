#include "drill/relay_with_service.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

// I AM NOT DONE

RelayWithService::RelayWithService()
: Node("relay_with_service")
{
  // TODO 1: コールバックグループを 2 つ作り、subscription_group_ と client_group_ に
  //         入れること。種類はどちらも MutuallyExclusive でよい
  //         （効くのは「別のグループに属していること」であって、種類ではない）。
  //         使う API は create_callback_group。

  // TODO 2: "sum" に std_msgs::msg::Int32 を流す Publisher を作り、
  //         publisher_ に入れること（QoS の depth は 10）。

  // TODO 3: "trigger"（std_msgs::msg::Int32、depth 10）を購読する Subscription を作り、
  //         subscription_ に入れること。コールバックは下の trigger_callback。
  //         このとき rclcpp::SubscriptionOptions を使って、この購読を
  //         subscription_group_ に置くこと（options に代入するメンバがあります）。

  // TODO 4: "add_two_ints" のクライアントを作り、client_ に入れること。
  //         create_client には QoS とコールバックグループを渡す引数があります。
  //         ここで client_group_ を渡し、購読とは別のグループに置くこと。
  //         これを忘れると、下の trigger_callback が永久に返らなくなります。
  //
  // コードの形が知りたければ ./drill hint 13
}

rclcpp::CallbackGroup::SharedPtr RelayWithService::subscription_group() const
{
  return subscription_group_;
}

rclcpp::CallbackGroup::SharedPtr RelayWithService::client_group() const
{
  return client_group_;
}

void RelayWithService::trigger_callback(const std_msgs::msg::Int32 & msg)
{
  (void)msg;

  // TODO 5: AddTwoInts::Request を作り、a にも b にも受け取った値を入れて
  //         非同期でリクエストを送ること（戻り値はローカル変数で受けてよい）。

  // TODO 6: ここで応答を待つこと。
  //
  //         ただし rclcpp::spin_until_future_complete() は使えません。
  //         この関数は Executor から呼ばれているコールバックの中であり、
  //         そこからさらに spin するのは、自分が乗っている Executor を
  //         自分でもう一度回そうとする行為です。
  //         代わりに future 自身の待ち方（wait_for）で
  //         std::future_status::ready になるまで待つこと。
  //         待ちきれなかったら RCLCPP_ERROR を出して return する。

  // TODO 7: 応答の sum を std_msgs::msg::Int32 に詰めて "sum" に publish すること。
  //
  // コードの形が知りたければ ./drill hint 13
}
