// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

/// 公式チュートリアル
/// 「Writing a Composable Node (C++)」の Talker をコンポーネント化したもの。
///
/// 課題01 の MinimalPublisher とやることは同じ（"topic" に String を publish する）
/// だが、守るべき約束が 1 つ増える。それがコンストラクタの引数 `NodeOptions` だ。
///
/// なぜ NodeOptions を受け取って Node にそのまま渡すのか
/// ------------------------------------------------------
/// `rclcpp_components` の component_container は、このクラスを直接 `new` するのではなく
/// `RCLCPP_COMPONENTS_REGISTER_NODE` が生成したファクトリ経由で `NodeOptions` を
/// 組み立ててから生成する。その `NodeOptions` には、実行時に決まる次のような
/// 設定が積まれている。
///
///   - `--ros-args -r __node:=foo` のようなノード名・トピック名のリマップ
///   - `--ros-args -p param:=value` で渡すパラメータ
///   - `use_intra_process_comms(true)`（課題14 のゼロコピー通信）。
///     コンポーネントは同一プロセスに複数ノードを載せられる仕組みなので、
///     これが効くかどうかは NodeOptions 経由でしか制御できない。
///   - コンテキスト（`rclcpp::Context`）やコールバックグループなど、
///     コンテナが複数ノードをまとめて面倒を見るための情報。
///
/// コンストラクタが options を受け取っても `Node("composable_talker")` と
/// 決め打ちで Node を作ってしまうと、これらの設定は一切反映されない
/// （＝リマップもパラメータもプロセス内通信も効かないノードになる）。
/// `Node("composable_talker", options)` と、必ず options を渡すこと。
class ComposableTalker : public rclcpp::Node
{
public:
  explicit ComposableTalker(const rclcpp::NodeOptions & options);

private:
  void timer_callback();

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  std::size_t count_;
};
