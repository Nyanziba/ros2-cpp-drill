#include "drill/node_with_parameters.hpp"

// I AM NOT DONE

NodeWithParameters::NodeWithParameters()
: Node("node_with_parameters")
{
  // TODO: 整数パラメータ "an_int_param" を既定値 0 で宣言すること。
  //       使う API は declare_parameter。

  // TODO: rclcpp::ParameterEventHandler を作って param_subscriber_ に入れること。
  //       これが /parameter_events を購読して、パラメータ変更を教えてくれる部品です。

  // TODO: "an_int_param" の変更を監視するコールバックを登録すること。
  //       使う API は param_subscriber_->add_parameter_callback。
  //       コールバックは rclcpp::Parameter を 1 つ受け取るラムダで、中で 2 つのことをする:
  //         1. 公式と同じログを出す。書式は
  //              cb: Received an update to parameter "<名前>" of type <型名>: "<値>"
  //            （名前・型名・値は rclcpp::Parameter の get_name / get_type_name / as_int から取る）
  //         2. この練習帳のテスト用に、受け取った値を latest_value_ に入れる
  //
  //       考えること: add_parameter_callback() は戻り値を返します。それをどうすべきか？
  //       捨てるとコールバックはその場で解除され、二度と呼ばれません。
  //       置き場所はこのクラスにもう用意されています（ヘッダを見てください）。
  //
  // コードの形が知りたければ ./drill hint 07
}

int64_t NodeWithParameters::latest_value() const
{
  return latest_value_;
}
