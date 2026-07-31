#include "drill/num_node.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;

// I AM NOT DONE

NumNode::NumNode()
: Node("num_node"), count_(0)
{
  // TODO: "num" という名前で Num を流す Publisher を作り、publisher_ に入れること。
  //       QoS の depth は 10。使う API は create_publisher。

  // TODO: 500ms 周期のタイマを作り、timer_ に入れること。
  //       呼び出すのは下の timer_callback。使う API は create_wall_timer で、
  //       メンバ関数を渡すには std::bind が要る。

  // TODO: "add_three_ints" という名前で AddThreeInts サービスのサーバを作り、
  //       service_ に入れること。使う API は create_service で、ハンドラには
  //       下の add_three_ints を std::bind で結びつける（引数が2つあるので
  //       _1, _2 が要る）。
}

void NumNode::timer_callback()
{
  // TODO: Num を作り、num フィールドに count_ を入れて publish すること。
  //       count_ は publish のたびに1増やす（1通目の num が 0 になるように）。
  //
  // TODO: 何を publish したか分かるログも出すこと。使うマクロは RCLCPP_INFO。
  //
  // コードの形が知りたければ ./drill hint 03
}

void NumNode::add_three_ints(
  const std::shared_ptr<AddThreeInts::Request> request,
  std::shared_ptr<AddThreeInts::Response> response)
{
  (void)request;
  (void)response;

  // TODO: リクエストの a, b, c を足した結果を、レスポンスの sum に入れること。
  //       値を return するのではなく response に書き込むのがサービスの作法です
  //       （04課題の AddTwoInts と同じ）。
  //
  // TODO: ログも2つ出すこと（04課題と同じ書式で、フィールドが1つ増えます）。
  //       1つめの書式:  Incoming request\na: <a> b: <b> c: <c>
  //       2つめの書式:  sending back response: [<sum>]
  //
  // コードの形が知りたければ ./drill hint 03
}
