// このファイルは編集しません（採点用）。
//
// YAML は test/ に置いてあり、NodeOptions の --ros-args --params-file で読ませる
// （ros2 run や launch で --params-file を渡すのと同じ経路）。
// ノード名は YAML の最上位のキー "filter_host" と一致している必要がある。
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "drill/filter_pipeline.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using Twist = geometry_msgs::msg::Twist;
using namespace std::chrono_literals;

namespace
{

constexpr double kTolerance = 1e-9;

Twist make_twist(double linear_x, double angular_z)
{
  Twist twist;
  twist.linear.x = linear_x;
  twist.angular.z = angular_z;
  return twist;
}

rclcpp::NodeOptions options_from_yaml(const std::string & file_name)
{
  return rclcpp::NodeOptions().arguments(
    {"--ros-args", "--params-file", std::string(DRILL_TEST_DIRECTORY) + "/" + file_name});
}

std::shared_ptr<FilterPipeline> make_pipeline(const rclcpp::NodeOptions & options)
{
  auto pipeline = std::make_shared<FilterPipeline>(options);
  pipeline->configure();
  return pipeline;
}

/// 標準エラー出力（fd 2）を、生きている間だけファイルに取り込む。
///
/// class_loader の警告は、rclcpp のログではなく標準エラーに直接出る（LogCapture では捕まらない）。
class StderrCapture
{
public:
  StderrCapture()
  : file_(std::tmpfile())
  {
    std::fflush(stderr);
    saved_descriptor_ = dup(STDERR_FILENO);
    dup2(fileno(file_), STDERR_FILENO);
  }

  ~StderrCapture()
  {
    restore();
    std::fclose(file_);
  }

  StderrCapture(const StderrCapture &) = delete;
  StderrCapture & operator=(const StderrCapture &) = delete;

  /// 標準エラーを元に戻し、それまでに出た内容を返す。
  std::string finish()
  {
    restore();
    std::string text;
    std::rewind(file_);
    char buffer[512];
    std::size_t length = 0;
    while ((length = std::fread(buffer, 1, sizeof(buffer), file_)) > 0) {
      text.append(buffer, length);
    }
    return text;
  }

private:
  void restore()
  {
    if (saved_descriptor_ >= 0) {
      std::fflush(stderr);
      dup2(saved_descriptor_, STDERR_FILENO);
      close(saved_descriptor_);
      saved_descriptor_ = -1;
    }
  }

  std::FILE * file_;
  int saved_descriptor_{-1};
};

/// YAML を読んで組んだパイプラインを使い、壊れるまでの間に標準エラーに出た内容を返す。
std::string destroy_pipeline_holding_filters_and_capture_stderr()
{
  StderrCapture capture;
  {
    auto pipeline = make_pipeline(options_from_yaml("filters.yaml"));
    pipeline->apply(make_twist(1.0, 1.0), 0.1);
  }
  return capture.finish();
}

}  // namespace

// 観点1: filters の並びの順に通しているか。
//
// 同じ 2 段（deadband: しきい値 0.05 / clamp: 上限 0.04）を、順番だけ入れ替えた 2 つの YAML で比べる。
//   deadband -> clamp : 1.0 は deadband を通り（そのまま）、clamp で 0.04 になる
//   clamp -> deadband : 1.0 は clamp で 0.04 になり、deadband で（0.04 < 0.05 なので）0 になる
TEST_F(DrillTest, AppliesFiltersInListedOrder)
{
  {
    auto pipeline = make_pipeline(options_from_yaml("pipeline_deadband_then_clamp.yaml"));
    EXPECT_EQ(pipeline->filter_count(), 2u)
      << drill::localized(
      "filters に 2 つ書いてあるのに、組み上がった段の数が違います。filters を宣言して、全部読んでいますか？",
      "filters lists 2 stages, but the number of stages built differs. "
      "Do you declare filters and read all of it?");
    const auto result = pipeline->apply(make_twist(1.0, 0.0), 0.1);
    EXPECT_NEAR(result.linear.x, 0.04, kTolerance)
      << drill::localized(
      "filters: [deadband, clamp] のとき、linear.x = 1.0 は 0.04 になるはずです。実際の値: ",
      "With filters: [deadband, clamp], linear.x = 1.0 should become 0.04. Actual value: ")
      << result.linear.x;
  }
  {
    auto pipeline = make_pipeline(options_from_yaml("pipeline_clamp_then_deadband.yaml"));
    const auto result = pipeline->apply(make_twist(1.0, 0.0), 0.1);
    EXPECT_NEAR(result.linear.x, 0.0, kTolerance)
      << drill::localized(
      "filters: [clamp, deadband] のとき、linear.x = 1.0 は 0.0 になるはずです。実際の値: ",
      "With filters: [clamp, deadband], linear.x = 1.0 should become 0.0. Actual value: ")
      << result.linear.x
      << drill::localized(
      "\n  filters の並びの順に push_back し、その順に filter() を呼んでいますか？",
      "\n  Do you push_back in the order of filters and call filter() in that order?");
  }
}

// 観点2: <name>.plugin で名前を引き、各プラグインに自分の <name>.<項目> が渡るか。
//
// test/filters.yaml は、既定値と違う値を一部だけ指定している。
//   clamp.max_linear = 0.5（既定 1.0）、rate_limit.max_linear_acceleration = 1.0（既定 0.5）。
//   指定していない値（clamp.max_angular = 2.0、rate_limit.max_angular_acceleration = 1.0）は既定のまま。
// dt = 0.2 秒、入力 (1.0, 3.0): deadband を通り → clamp で (0.5, 2.0) → rate_limit で
// 1 回に動ける量は linear 1.0 x 0.2 = 0.2、angular 1.0 x 0.2 = 0.2 なので (0.2, 0.2)。
TEST_F(DrillTest, PassesEachStageItsOwnParameters)
{
  auto pipeline = make_pipeline(options_from_yaml("filters.yaml"));
  ASSERT_EQ(pipeline->filter_count(), 3u)
    << drill::localized(
    "filters: [deadband, clamp, rate_limit] なのに、組み上がった段が 3 つではありません。"
    "各段の <name>.plugin を宣言して読み、createSharedInstance に渡していますか？",
    "filters is [deadband, clamp, rate_limit], but 3 stages were not built. "
    "Do you declare and read <name>.plugin for each stage and pass it to createSharedInstance?");

  const auto result = pipeline->apply(make_twist(1.0, 3.0), 0.2);
  EXPECT_NEAR(result.linear.x, 0.2, kTolerance)
    << drill::localized(
    "linear.x は 0.2 になるはずです（clamp 0.5 -> rate_limit 1.0 x 0.2）。実際の値: ",
    "linear.x should be 0.2 (clamp 0.5 -> rate_limit 1.0 x 0.2). Actual value: ")
    << result.linear.x
    << drill::localized(
    "\n  initialize(shared_from_this(), name) の name に、段の名前（\"clamp\" など）を渡していますか？"
    "\n  プラグインは \"<name>.max_linear\" のように、渡された name を頭につけて宣言します。",
    "\n  Do you pass the stage name (such as \"clamp\") as name in initialize(shared_from_this(), name)?"
    "\n  A plugin declares parameters like \"<name>.max_linear\", with the name it was given in front.");
  EXPECT_NEAR(result.angular.z, 0.2, kTolerance)
    << drill::localized(
    "angular.z は 0.2 になるはずです。実際の値: ",
    "angular.z should be 0.2. Actual value: ")
    << result.angular.z;
}

// 観点3: 未知の plugin 名のとき、落ちずに分かるメッセージを出し、その段だけ飛ばすか。
TEST_F(DrillTest, SkipsStageWithUnknownPluginAndKeepsTheRest)
{
  drill::LogCapture log;
  std::shared_ptr<FilterPipeline> pipeline;
  ASSERT_NO_THROW(pipeline = make_pipeline(options_from_yaml("pipeline_unknown_plugin.yaml")))
    << drill::localized(
    "存在しない plugin 名で configure() が例外を投げました。"
    "PluginlibException を段ごとの try / catch で受けましたか？",
    "configure() threw for a plugin name that does not exist. "
    "Did you catch the PluginlibException with try / catch for each stage?");

  EXPECT_EQ(pipeline->filter_count(), 2u)
    << drill::localized(
    "filters: [deadband, typo, clamp] のうち typo だけ読めないので、組み上がる段は 2 つのはずです。"
    "読めなかった段を列に入れていませんか？ 例外で止まって残りを飛ばしていませんか？",
    "Only typo cannot be loaded out of filters: [deadband, typo, clamp], so 2 stages should be built. "
    "Are you putting the stage that failed into the list? Do you stop at the exception and skip the rest?");

  const auto result = pipeline->apply(make_twist(1.0, 0.0), 0.1);
  EXPECT_NEAR(result.linear.x, 0.5, kTolerance)
    << drill::localized(
    "残った deadband と clamp は動くはずです（linear.x = 1.0 は 0.5 になる）。実際の値: ",
    "The remaining deadband and clamp should work (linear.x = 1.0 becomes 0.5). Actual value: ")
    << result.linear.x;

  EXPECT_TRUE(log.contains("drill/NoSuchFilter") && log.contains("typo"))
    << drill::localized(
    "ログに、段の名前 \"typo\" と plugin 名 \"drill/NoSuchFilter\" の両方が出ていません。"
    "RCLCPP_ERROR に、段の名前・plugin 名・e.what() を出してください。捕まえたログ:",
    "The log does not contain both the stage name \"typo\" and the plugin name \"drill/NoSuchFilter\". "
    "Put the stage name, the plugin name and e.what() in an RCLCPP_ERROR. Captured log:")
    << log.dump();
}

// 観点4: filters が無い（YAML も無い）ときは、段が 0 で、通過するか。
TEST_F(DrillTest, PassesThroughWhenFiltersIsEmpty)
{
  auto pipeline = make_pipeline(rclcpp::NodeOptions());
  EXPECT_EQ(pipeline->filter_count(), 0u);
  const auto result = pipeline->apply(make_twist(2.0, -3.0), 0.1);
  EXPECT_NEAR(result.linear.x, 2.0, kTolerance);
  EXPECT_NEAR(result.angular.z, -3.0, kTolerance);
}

// 観点5: cmd_vel_in を購読し、フィルタを通して cmd_vel_out に出すか。
//
// test/pipeline_deadband_then_clamp.yaml: deadband 0.05 -> clamp 0.04。
// 1.0 を送れば 0.04、0.01 を送れば 0.0 が出てくる。
TEST_F(DrillTest, PublishesFilteredCommandToCmdVelOut)
{
  auto pipeline = make_pipeline(options_from_yaml("pipeline_deadband_then_clamp.yaml"));
  auto probe = rclcpp::Node::make_shared(drill::unique_name("probe"));
  auto publisher = probe->create_publisher<Twist>("cmd_vel_in", 10);
  std::vector<Twist> received;
  auto subscription = probe->create_subscription<Twist>(
    "cmd_vel_out", 10, [&received](Twist::ConstSharedPtr message) {received.push_back(*message);});

  // discovery に時間がかかるので、届くまで送り続ける。
  const bool is_received = drill::spin_until(
    {pipeline, probe}, [&received]() {return !received.empty();}, 8s,
    [&publisher]() {publisher->publish(make_twist(1.0, 0.0));});

  ASSERT_TRUE(is_received)
    << drill::localized(
    "cmd_vel_in に送っても、cmd_vel_out に 8 秒待っても届きませんでした。\n"
    "  - cmd_vel_in を購読し、コールバックで apply して cmd_vel_out に publish していますか？\n"
    "  - 購読者と出し手を、メンバ（subscription_ / publisher_）に保持していますか？",
    "Nothing arrived on cmd_vel_out within 8 seconds of sending to cmd_vel_in.\n"
    "  - Do you subscribe to cmd_vel_in, run apply in the callback, and publish to cmd_vel_out?\n"
    "  - Do you keep the subscription and the publisher in members (subscription_ / publisher_)?");
  EXPECT_NEAR(received.front().linear.x, 0.04, kTolerance)
    << drill::localized(
    "cmd_vel_out の linear.x は 0.04（フィルタを通した値）のはずです。実際の値: ",
    "linear.x on cmd_vel_out should be 0.04 (the filtered value). Actual value: ")
    << received.front().linear.x;
}

// 観点6: ClassLoader をフィルタの列より先に宣言しているか（壊れるときに警告が出ないか）。
//
// メンバは宣言の逆順に壊れる。ClassLoader が先に壊れると、class_loader が標準エラーに
// "SEVERE WARNING!!! Attempting to unload library while objects created by this loader exist in the heap!"
// を出す（ライブラリは外されないが、未定義の動作になりうる）。標準エラーを捕まえて、警告が無いことを確かめる。
TEST_F(DrillTest, PipelineWithLiveFiltersIsDestroyedWithoutClassLoaderWarning)
{
  const std::string error_output = destroy_pipeline_holding_filters_and_capture_stderr();
  EXPECT_EQ(error_output.find("SEVERE WARNING"), std::string::npos)
    << drill::localized(
    "フィルタを持ったまま FilterPipeline を壊したら、class_loader が警告を出しました。\n"
    "  ClassLoader（loader_）を、フィルタの列（filters_）より先に宣言してください。\n"
    "  メンバは宣言の逆順に壊れるので、こうすると ClassLoader が最後に壊れます。\n"
    "  逆だと、フィルタが残っているのに ClassLoader が先に壊れ、未定義の動作になりえます。\n"
    "  出た警告:\n",
    "class_loader printed a warning when a FilterPipeline holding filters was destroyed.\n"
    "  Declare the ClassLoader (loader_) before the list of filters (filters_).\n"
    "  Members are destroyed in reverse order, so this way the ClassLoader is destroyed last.\n"
    "  The other way round, the ClassLoader is destroyed while the filters are still alive, "
    "which can be undefined behavior.\n"
    "  The warning:\n")
    << error_output;
}
