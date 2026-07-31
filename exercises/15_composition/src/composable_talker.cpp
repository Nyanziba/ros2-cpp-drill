#include "drill/composable_talker.hpp"

using namespace std::chrono_literals;

// I AM NOT DONE

ComposableTalker::ComposableTalker(const rclcpp::NodeOptions & options)
: Node("composable_talker"), count_(0)
{
  // TODO: 上の初期化を直し、受け取った options を Node にそのまま渡すこと。
  //       渡さないと、--ros-args でのノード名変更やパラメータ指定、
  //       プロセス内通信の設定が一切効かないノードになります
  //       （常に既定名 "composable_talker" のまま動いてしまう）。

  // TODO: "topic" に std_msgs::msg::String を流す Publisher を作り、
  //       publisher_ に入れること（QoS の depth は 10）。

  // TODO: 200ms 周期のタイマを作り、timer_ に入れること。
  //       呼び出すのは下の timer_callback。
}

void ComposableTalker::timer_callback()
{
  // TODO: std_msgs::msg::String を作り、data を "composable hello " と count_ を
  //       つなげた文字列にして publish すること。count_ は publish のたびに 1 増やす。
  //
  // TODO: ログも出すこと。書式は  Publishing: '<本文>'
  //
  // コードの形が知りたければ ./drill hint 15
}

// TODO: このファイルの末尾で、このクラスをコンポーネントとして登録すること。
//       登録がないと component_container はこのクラスの存在に気づけず、
//       ros2 component load でロードできません。この課題の核心はここです。
//
//       必要なもの: rclcpp_components/register_node_macro.hpp の include と、
//       RCLCPP_COMPONENTS_REGISTER_NODE という名前のマクロ。
//       （./drill hint 15 に正確な書き方があります）
