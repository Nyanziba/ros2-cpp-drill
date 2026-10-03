// このファイルは編集しません（採点用）。
//
// テストはプラグインのライブラリにリンクしない（FilterHost のライブラリにだけリンクする）。
// プラグインは、FilterHost の中の ClassLoader が plugins.xml を頼りに実行時に読み込む。
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "drill/filter_host.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using Twist = geometry_msgs::msg::Twist;

namespace
{

constexpr char kClampName[] = "drill/ClampFilter";
constexpr char kRateLimitName[] = "drill/RateLimitFilter";
constexpr char kDeadbandName[] = "drill/DeadbandFilter";
constexpr char kMissingName[] = "drill/NoSuchFilter";
constexpr double kTolerance = 1e-9;

Twist make_twist(double linear_x, double angular_z)
{
  Twist twist;
  twist.linear.x = linear_x;
  twist.angular.z = angular_z;
  return twist;
}

/// filter_type を指定して FilterHost を作り、configure() まで済ませる。
std::shared_ptr<FilterHost> make_host(
  const std::string & filter_type, const std::vector<rclcpp::Parameter> & extra_parameters = {})
{
  std::vector<rclcpp::Parameter> overrides = extra_parameters;
  if (!filter_type.empty()) {
    overrides.emplace_back("filter_type", filter_type);
  }
  auto host = std::make_shared<FilterHost>(rclcpp::NodeOptions().parameter_overrides(overrides));
  host->configure();
  return host;
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

/// フィルタを読み込んだ FilterHost を作って使い、壊れるまでの間に標準エラーに出た内容を返す。
std::string destroy_host_holding_filter_and_capture_stderr()
{
  StderrCapture capture;
  {
    auto host = make_host(kClampName);
    host->apply(make_twist(2.0, 3.0), 0.1);
  }
  return capture.finish();
}

}  // namespace

// 観点1: filter_type の名前でプラグインが選ばれ、"filter.<項目>" のパラメータで設定されるか。
TEST_F(DrillTest, SelectsFilterByFilterTypeParameter)
{
  auto clamp_host = make_host(kClampName, {rclcpp::Parameter("filter.max_linear", 0.5)});
  const auto clamped = clamp_host->apply(make_twist(2.0, 3.0), 0.1);
  EXPECT_NEAR(clamped.linear.x, 0.5, kTolerance)
    << drill::localized(
    "filter_type = drill/ClampFilter、filter.max_linear = 0.5 のとき、linear.x = 2.0 は 0.5 になるはずです。"
    "実際の値: ",
    "With filter_type = drill/ClampFilter and filter.max_linear = 0.5, linear.x = 2.0 should become 0.5. "
    "Actual value: ")
    << clamped.linear.x
    << drill::localized(
    "\n  - createSharedInstance の後で initialize(shared_from_this(), kFilterInstanceName) を呼んでいますか？"
    "\n  - 作ったフィルタを filter_ に入れ、apply で filter_->filter(...) を呼んでいますか？",
    "\n  - Do you call initialize(shared_from_this(), kFilterInstanceName) after createSharedInstance?"
    "\n  - Do you store the created filter in filter_ and call filter_->filter(...) in apply?");
  EXPECT_NEAR(clamped.angular.z, 2.0, kTolerance)
    << drill::localized(
    "filter.max_angular は既定の 2.0 のはずです。実際の値: ",
    "filter.max_angular should be its default of 2.0. Actual value: ")
    << clamped.angular.z;

  auto deadband_host = make_host(kDeadbandName);
  const auto dead = deadband_host->apply(make_twist(0.01, 0.05), 0.1);
  EXPECT_NEAR(dead.linear.x, 0.0, kTolerance)
    << drill::localized(
    "filter_type = drill/DeadbandFilter なのに、小さい指令が 0 になっていません。"
    "filter_type の名前を読んで、そのプラグインを作っていますか？",
    "With filter_type = drill/DeadbandFilter, the small command was not zeroed. "
    "Do you read the name from filter_type and create that plugin?");

  auto rate_host = make_host(kRateLimitName);
  const auto first = rate_host->apply(make_twist(1.0, 0.0), 0.1);
  const auto second = rate_host->apply(make_twist(1.0, 0.0), 0.1);
  EXPECT_NEAR(first.linear.x, 0.05, kTolerance);
  EXPECT_NEAR(second.linear.x, 0.10, kTolerance)
    << drill::localized(
    "RateLimitFilter は前回の出力を覚えています。apply のたびに新しく作り直していませんか？",
    "RateLimitFilter remembers its previous output. Are you recreating it on every apply?");
}

// 観点2: 名前が見つからないとき、落ちずに「通過」にフォールバックし、分かるメッセージを出すか。
TEST_F(DrillTest, FallsBackToPassThroughWhenNameIsUnknown)
{
  drill::LogCapture log;
  std::shared_ptr<FilterHost> host;
  ASSERT_NO_THROW(host = make_host(kMissingName))
    << drill::localized(
    "存在しない名前で configure() が例外を投げました。PluginlibException を try / catch で受けましたか？",
    "configure() threw an exception for a name that does not exist. "
    "Did you catch the PluginlibException with try / catch?");

  const auto command = make_twist(2.0, -3.0);
  const auto result = host->apply(command, 0.1);
  EXPECT_NEAR(result.linear.x, 2.0, kTolerance)
    << drill::localized(
    "通過になっていません。読み込めなかったときは、フィルタを空にして command をそのまま返してください。",
    "It is not passing through. When loading fails, leave the filter empty and return command as it is.");
  EXPECT_NEAR(result.angular.z, -3.0, kTolerance);

  EXPECT_TRUE(log.contains(kMissingName))
    << drill::localized(
    "ログに、読み込めなかった名前 \"drill/NoSuchFilter\" が出ていません。RCLCPP_ERROR に名前と e.what() を出してください。"
    "捕まえたログ:",
    "The log does not contain the name that could not be loaded, \"drill/NoSuchFilter\". "
    "Put the name and e.what() in an RCLCPP_ERROR. Captured log:")
    << log.dump();
}

// 観点3: filter_type を指定しなかったときも、落ちずに通過するか（既定は空）。
TEST_F(DrillTest, PassesThroughWhenFilterTypeIsNotGiven)
{
  std::shared_ptr<FilterHost> host;
  ASSERT_NO_THROW(host = make_host(""))
    << drill::localized(
    "filter_type を指定しないと configure() が例外を投げました。"
    "既定値を空文字列にして、読み込めなければ通過にしてください。",
    "configure() threw when filter_type was not given. "
    "Make the default an empty string and pass through when loading fails.");
  const auto result = host->apply(make_twist(2.0, 3.0), 0.1);
  EXPECT_NEAR(result.linear.x, 2.0, kTolerance);
  EXPECT_NEAR(result.angular.z, 3.0, kTolerance);
}

// 観点4: 使える名前の一覧が返るか（ClassLoader の getDeclaredClasses）。
TEST_F(DrillTest, ListsAvailableFilterNames)
{
  auto host = make_host(kMissingName);
  const auto names = host->available_filter_names();
  for (const char * expected : {kClampName, kRateLimitName, kDeadbandName}) {
    EXPECT_NE(std::find(names.begin(), names.end(), expected), names.end())
      << "\"" << expected
      << drill::localized(
      "\" が available_filter_names() の結果にありません。"
      "loader_.getDeclaredClasses() をそのまま返していますか？",
      "\" is not in the result of available_filter_names(). "
      "Do you return loader_.getDeclaredClasses() as it is?");
  }
}

// 観点5: ClassLoader をフィルタより先に宣言しているか（壊れるときに警告が出ないか）。
//
// メンバは宣言の逆順に壊れる。フィルタを ClassLoader より先に宣言すると、ClassLoader が先に壊れ、
// class_loader が標準エラーに "SEVERE WARNING!!! Attempting to unload library while objects created
// by this loader exist in the heap!" を出す（ライブラリは外されないが、未定義の動作になりうる）。
// 警告が出ていないことを、標準エラーを捕まえて確かめる。
TEST_F(DrillTest, HostWithLiveFilterIsDestroyedWithoutClassLoaderWarning)
{
  const std::string error_output = destroy_host_holding_filter_and_capture_stderr();
  EXPECT_EQ(error_output.find("SEVERE WARNING"), std::string::npos)
    << drill::localized(
    "フィルタを持ったまま FilterHost を壊したら、class_loader が警告を出しました。\n"
    "  ClassLoader（loader_）を、フィルタ（filter_）より先に宣言してください。\n"
    "  メンバは宣言の逆順に壊れるので、こうすると ClassLoader が最後に壊れます。\n"
    "  逆だと、フィルタが残っているのに ClassLoader が先に壊れ、未定義の動作になりえます。\n"
    "  出た警告:\n",
    "class_loader printed a warning when a FilterHost holding a filter was destroyed.\n"
    "  Declare the ClassLoader (loader_) before the filter (filter_).\n"
    "  Members are destroyed in reverse order, so this way the ClassLoader is destroyed last.\n"
    "  The other way round, the ClassLoader is destroyed while the filter is still alive, "
    "which can be undefined behavior.\n"
    "  The warning:\n")
    << error_output;
}
