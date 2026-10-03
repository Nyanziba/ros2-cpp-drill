// このファイルは編集しません（採点用）。
//
// テストはプラグインのライブラリにリンクしない。pluginlib::ClassLoader が、
// plugins.xml を頼りに実行時に読み込む（受講者の書き出しが正しいかを、本物の経路で見る）。
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <pluginlib/class_loader.hpp>

#include "drill/velocity_filter.hpp"
#include "drill_harness.hpp"
#include "drill_i18n.hpp"

using DrillTest = drill::DrillTest;
using Twist = geometry_msgs::msg::Twist;

namespace
{

constexpr char kPackageName[] = "drill_16_pluginlib_create";
constexpr char kBaseClassName[] = "drill::VelocityFilter";
constexpr char kClampName[] = "drill/ClampFilter";
constexpr char kRateLimitName[] = "drill/RateLimitFilter";
constexpr char kDeadbandName[] = "drill/DeadbandFilter";
constexpr double kTolerance = 1e-9;

using FilterLoader = pluginlib::ClassLoader<drill::VelocityFilter>;

Twist make_twist(double linear_x, double angular_z)
{
  Twist twist;
  twist.linear.x = linear_x;
  twist.angular.z = angular_z;
  return twist;
}

std::string join_lines(const std::vector<std::string> & names)
{
  if (names.empty()) {
    return std::string("\n      ") + drill::localized("（1 件もありません）", "(none)");
  }
  std::string joined;
  for (const auto & name : names) {
    joined += "\n      " + name;
  }
  return joined;
}

/// ClassLoader を作る。plugins.xml が壊れていると例外になるので、失敗として報告する。
std::unique_ptr<FilterLoader> open_loader()
{
  try {
    return std::make_unique<FilterLoader>(kPackageName, kBaseClassName);
  } catch (const std::exception & error) {
    ADD_FAILURE()
      << drill::localized(
      "ClassLoader を作れませんでした。plugins.xml の書き方を確かめてください: ",
      "Could not create the ClassLoader. Check how plugins.xml is written: ")
      << error.what();
    return nullptr;
  }
}

/// 名前でプラグインを作り、initialize まで済ませる。失敗は失敗として報告して nullptr を返す。
std::shared_ptr<drill::VelocityFilter> create_filter(
  FilterLoader & loader, const std::string & plugin_name, const rclcpp::Node::SharedPtr & node,
  const std::string & instance_name)
{
  try {
    auto filter = loader.createSharedInstance(plugin_name);
    filter->initialize(node, instance_name);
    return filter;
  } catch (const std::exception & error) {
    ADD_FAILURE()
      << drill::localized("プラグイン \"", "Could not create the plugin \"")
      << plugin_name
      << drill::localized(
      "\" を作れませんでした。\n"
      "  - plugins.xml の <class> に name=\"...\" type=\"...\" base_class_type=\"drill::VelocityFilter\" を書きましたか？\n"
      "  - <library path=\"...\"> は CMake の add_library の名前と合っていますか？\n"
      "  - filters.cpp の末尾に PLUGINLIB_EXPORT_CLASS を書きましたか？\n"
      "  pluginlib のメッセージ: ",
      "\".\n"
      "  - Did you write name=\"...\" type=\"...\" base_class_type=\"drill::VelocityFilter\" in the <class> of plugins.xml?\n"
      "  - Does <library path=\"...\"> match the name given to add_library in CMake?\n"
      "  - Did you write PLUGINLIB_EXPORT_CLASS at the end of filters.cpp?\n"
      "  pluginlib's message: ")
      << error.what();
    return nullptr;
  }
}

rclcpp::Node::SharedPtr make_node(const std::vector<rclcpp::Parameter> & overrides = {})
{
  return rclcpp::Node::make_shared(
    drill::unique_name("filter_test"), rclcpp::NodeOptions().parameter_overrides(overrides));
}

}  // namespace

// 観点1: plugins.xml の宣言が pluginlib に届いているか。
//
// cmake/export_plugins.cmake の pluginlib_export_plugin_description_file と、plugins.xml の <class> の両方が
// そろって初めて、ClassLoader の「宣言されたクラス」の一覧に名前が出る。
TEST_F(DrillTest, ThreePluginsAreDeclared)
{
  auto loader = open_loader();
  ASSERT_NE(loader, nullptr);

  const auto declared = loader->getDeclaredClasses();
  for (const char * expected : {kClampName, kRateLimitName, kDeadbandName}) {
    EXPECT_NE(std::find(declared.begin(), declared.end(), expected), declared.end())
      << "\""
      << expected
      << drill::localized(
      "\" が、宣言されたプラグインの一覧にありません。\n"
      "  - plugins.xml の <class name=\"...\"> に書きましたか？\n"
      "  - cmake/export_plugins.cmake で pluginlib_export_plugin_description_file(${PROJECT_NAME} plugins.xml) を呼びましたか？\n"
      "  今 ClassLoader から見えている名前:",
      "\" is not in the list of declared plugins.\n"
      "  - Did you write it in <class name=\"...\"> of plugins.xml?\n"
      "  - Did you call pluginlib_export_plugin_description_file(${PROJECT_NAME} plugins.xml) in cmake/export_plugins.cmake?\n"
      "  Names the ClassLoader can see now:")
      << join_lines(declared);
  }
}

// 観点2: 実際に作れるか（ライブラリが見つかり、PLUGINLIB_EXPORT_CLASS が書かれているか）。
//
// 宣言が正しくても、ライブラリの path が違う、PLUGINLIB_EXPORT_CLASS が無い、
// のどちらかなら createSharedInstance が PluginlibException を投げる。
TEST_F(DrillTest, ThreePluginsCanBeCreated)
{
  auto loader = open_loader();
  ASSERT_NE(loader, nullptr);
  auto node = make_node();

  // ClassLoader（loader）が先、インスタンスが後。破棄は逆順になるので安全。
  EXPECT_NE(create_filter(*loader, kClampName, node, "clamp"), nullptr);
  EXPECT_NE(create_filter(*loader, kRateLimitName, node, "rate_limit"), nullptr);
  EXPECT_NE(create_filter(*loader, kDeadbandName, node, "deadband"), nullptr);
}

// 観点3: ClampFilter::filter の中身（既定の上限は linear 1.0 / angular 2.0）。
TEST_F(DrillTest, ClampFilterCutsAbsoluteValueAtLimits)
{
  auto loader = open_loader();
  ASSERT_NE(loader, nullptr);
  auto node = make_node();
  auto filter = create_filter(*loader, kClampName, node, "clamp");
  ASSERT_NE(filter, nullptr);

  const auto over = filter->filter(make_twist(3.0, 5.0), 0.1);
  EXPECT_NEAR(over.linear.x, 1.0, kTolerance)
    << drill::localized(
    "linear.x = 3.0 を 1.0 に切れていません。実際の値: ",
    "linear.x = 3.0 was not cut to 1.0. Actual value: ")
    << over.linear.x;
  EXPECT_NEAR(over.angular.z, 2.0, kTolerance)
    << drill::localized(
    "angular.z = 5.0 を 2.0 に切れていません。実際の値: ",
    "angular.z = 5.0 was not cut to 2.0. Actual value: ")
    << over.angular.z;

  const auto under = filter->filter(make_twist(-3.0, -5.0), 0.1);
  EXPECT_NEAR(under.linear.x, -1.0, kTolerance)
    << drill::localized(
    "負の向き（linear.x = -3.0）も -1.0 に切ってください。実際の値: ",
    "Cut the negative direction too (linear.x = -3.0 should become -1.0). Actual value: ")
    << under.linear.x;
  EXPECT_NEAR(under.angular.z, -2.0, kTolerance)
    << drill::localized(
    "負の向き（angular.z = -5.0）も -2.0 に切ってください。実際の値: ",
    "Cut the negative direction too (angular.z = -5.0 should become -2.0). Actual value: ")
    << under.angular.z;

  const auto inside = filter->filter(make_twist(0.4, -0.7), 0.1);
  EXPECT_NEAR(inside.linear.x, 0.4, kTolerance)
    << drill::localized(
    "上限の内側の値は、そのまま通してください。",
    "Values inside the limits must pass through unchanged.");
  EXPECT_NEAR(inside.angular.z, -0.7, kTolerance)
    << drill::localized(
    "上限の内側の値は、そのまま通してください。",
    "Values inside the limits must pass through unchanged.");
}

// 観点4: パラメータ "<name>.max_linear" を、initialize に渡した名前で読んでいるか。
//
// initialize は用意してある。ここは「名前を変えても、フィルタの中身が
// 保存した上限を使っているか」を見る（メンバにとった上限を filter() で使うこと）。
TEST_F(DrillTest, ClampFilterUsesParametersUnderInstanceName)
{
  auto loader = open_loader();
  ASSERT_NE(loader, nullptr);
  auto node = make_node(
    {rclcpp::Parameter("limit.max_linear", 0.3), rclcpp::Parameter("limit.max_angular", 0.4)});
  auto filter = create_filter(*loader, kClampName, node, "limit");
  ASSERT_NE(filter, nullptr);

  const auto result = filter->filter(make_twist(1.0, -1.0), 0.1);
  EXPECT_NEAR(result.linear.x, 0.3, kTolerance)
    << drill::localized(
    "limit.max_linear = 0.3 のとき、linear.x = 1.0 は 0.3 になるはずです。実際の値: ",
    "With limit.max_linear = 0.3, linear.x = 1.0 should become 0.3. Actual value: ")
    << result.linear.x
    << drill::localized(
    "\n  filter() で、固定の 1.0 ではなく max_linear_ を使っていますか？",
    "\n  In filter(), do you use max_linear_ instead of a fixed 1.0?");
  EXPECT_NEAR(result.angular.z, -0.4, kTolerance)
    << drill::localized(
    "limit.max_angular = 0.4 のとき、angular.z = -1.0 は -0.4 になるはずです。実際の値: ",
    "With limit.max_angular = 0.4, angular.z = -1.0 should become -0.4. Actual value: ")
    << result.angular.z;
}

// 観点5: 渡してある残りの 2 つも、ClassLoader 越しに動くか（書き出し・宣言の確認を兼ねる）。
TEST_F(DrillTest, RateLimitAndDeadbandWorkThroughClassLoader)
{
  auto loader = open_loader();
  ASSERT_NE(loader, nullptr);
  auto node = make_node();
  auto rate_limit = create_filter(*loader, kRateLimitName, node, "rate_limit");
  auto deadband = create_filter(*loader, kDeadbandName, node, "deadband");
  ASSERT_NE(rate_limit, nullptr);
  ASSERT_NE(deadband, nullptr);

  // 既定の加速度は linear 0.5 / angular 1.0。dt = 0.1 秒なら 1 回に 0.05 / 0.1 まで。
  const auto first = rate_limit->filter(make_twist(1.0, 1.0), 0.1);
  const auto second = rate_limit->filter(make_twist(1.0, 1.0), 0.1);
  EXPECT_NEAR(first.linear.x, 0.05, kTolerance);
  EXPECT_NEAR(second.linear.x, 0.10, kTolerance);
  EXPECT_NEAR(second.angular.z, 0.20, kTolerance);

  // 既定のしきい値は linear 0.05 / angular 0.1。
  const auto small = deadband->filter(make_twist(0.01, 0.05), 0.1);
  EXPECT_NEAR(small.linear.x, 0.0, kTolerance);
  EXPECT_NEAR(small.angular.z, 0.0, kTolerance);
  const auto large = deadband->filter(make_twist(0.5, 0.5), 0.1);
  EXPECT_NEAR(large.linear.x, 0.5, kTolerance);
  EXPECT_NEAR(large.angular.z, 0.5, kTolerance);
}
