#include "drill/add_two_ints_server.hpp"

using std::placeholders::_1;
using std::placeholders::_2;

// I AM NOT DONE

AddTwoIntsServer::AddTwoIntsServer()
: Node("add_two_ints_server")
{
  // TODO: "add_two_ints" という名前で AddTwoInts サービスのサーバを作り、
  //       service_ に入れること。使う API は create_service で、ハンドラには
  //       下の add を std::bind で結びつける（引数が 2 つあるので _1, _2 が必要）。
}

void AddTwoIntsServer::add(
  const std::shared_ptr<AddTwoInts::Request> request,
  std::shared_ptr<AddTwoInts::Response> response)
{
  (void)request;
  (void)response;

  // TODO: リクエストの a と b を足した結果を、レスポンスの sum に入れること。
  //       値を return するのではなく response に書き込むのがサービスの作法です。

  // TODO: 公式と同じログを 2 つ出すこと（使うマクロは RCLCPP_INFO）。
  //       1 つめの書式:  Incoming request\na: <a> b: <b>
  //       2 つめの書式:  sending back response: [<sum>]
  //
  // コードの形が知りたければ ./drill hint 04
}
