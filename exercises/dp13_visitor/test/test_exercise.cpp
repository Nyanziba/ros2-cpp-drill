// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "drill/diagnostics.hpp"

namespace
{

/// 同じ木を GoF 版と variant 版の両方で組み立てる。
///
///   [selftest]
///     bat 11800mV OK        （limit 13000）
///     [drive]
///       motor_l fault=0 OK
///       motor_r fault=3 NG
///     temp 65000mV NG       （limit 60000）
std::unique_ptr<CheckGroup> make_gof_tree()
{
  auto root = std::make_unique<CheckGroup>("selftest");
  root->add(std::make_unique<SensorCheck>("bat", 11800, 13000));

  auto drive = std::make_unique<CheckGroup>("drive");
  drive->add(std::make_unique<MotorCheck>("motor_l", 0U));
  drive->add(std::make_unique<MotorCheck>("motor_r", 3U));
  root->add(std::move(drive));

  root->add(std::make_unique<SensorCheck>("temp", 65000, 60000));
  return root;
}

/// 上と同じ木を DiagArena の上に作り、ルートの添字を返す。
std::size_t make_variant_tree(DiagArena & arena)
{
  const std::size_t bat = arena.add(SensorSample{"bat", 11800, 13000});
  const std::size_t motor_l = arena.add(MotorSample{"motor_l", 0U});
  const std::size_t motor_r = arena.add(MotorSample{"motor_r", 3U});
  const std::size_t drive = arena.add(GroupSample{"drive", {motor_l, motor_r}});
  const std::size_t temp = arena.add(SensorSample{"temp", 65000, 60000});
  return arena.add(GroupSample{"selftest", {bat, drive, temp}});
}

const char * const kExpectedReport =
  "[selftest]\n"
  "  bat 11800mV OK\n"
  "  [drive]\n"
  "    motor_l fault=0 OK\n"
  "    motor_r fault=3 NG\n"
  "  temp 65000mV NG\n";

/// 二重ディスパッチの検証用。どの visit が呼ばれたかを順番に記録する。
class RecordingVisitor : public DiagVisitor
{
public:
  void visit(const SensorCheck & node) override
  {
    log.push_back("sensor:" + node.name());
  }

  void visit(const MotorCheck & node) override
  {
    log.push_back("motor:" + node.name());
  }

  void visit(const CheckGroup & node) override
  {
    log.push_back("group:" + node.name());
    for (const std::unique_ptr<DiagNode> & child : node.children()) {
      child->accept(*this);
    }
  }

  std::vector<std::string> log;
};

}  // namespace

TEST(VisitorTest, 集計Visitorが木全体のNGを数える)
{
  const std::unique_ptr<CheckGroup> root = make_gof_tree();

  FailureCountVisitor counter;
  root->accept(counter);

  EXPECT_EQ(counter.checked_count(), 4) << "葉を 4 つ訪問するはずです";
  EXPECT_EQ(counter.failure_count(), 2) << "NG は motor_r と temp の 2 つです";
}

TEST(VisitorTest, 整形Visitorがインデント付きレポートを作る)
{
  const std::unique_ptr<CheckGroup> root = make_gof_tree();

  TextReportVisitor reporter;
  root->accept(reporter);

  EXPECT_EQ(reporter.text(), std::string(kExpectedReport));
}

TEST(VisitorTest, 同じ木に2種類のVisitorを当てられる)
{
  const std::unique_ptr<CheckGroup> root = make_gof_tree();

  FailureCountVisitor counter;
  TextReportVisitor reporter;
  root->accept(counter);
  root->accept(reporter);

  // 要素側は一切変えずに、操作だけを増やせている。
  EXPECT_EQ(counter.failure_count(), 2);
  EXPECT_EQ(reporter.text(), std::string(kExpectedReport));
}

TEST(VisitorTest, 基底ポインタ経由でも派生ごとのvisitが選ばれる)
{
  // accept() が仮想でないと、ここは全部「基底の accept」に落ちて種類が消えます。
  const SensorCheck sensor{"bat", 11800, 13000};
  const MotorCheck motor{"motor_r", 3U};

  const DiagNode * const nodes[] = {&sensor, &motor};

  RecordingVisitor recorder;
  for (const DiagNode * const node : nodes) {
    node->accept(recorder);
  }

  const std::vector<std::string> expected = {"sensor:bat", "motor:motor_r"};
  EXPECT_EQ(recorder.log, expected)
    << "accept() の中で visitor.visit(*this) を呼べていますか（二重ディスパッチ）";
}

TEST(VisitorTest, acceptの訪問順は深さ優先で追加順)
{
  const std::unique_ptr<CheckGroup> root = make_gof_tree();

  RecordingVisitor recorder;
  root->accept(recorder);

  const std::vector<std::string> expected = {
    "group:selftest", "sensor:bat", "group:drive",
    "motor:motor_l", "motor:motor_r", "sensor:temp"};
  EXPECT_EQ(recorder.log, expected);
}

TEST(VisitorTest, 空のグループでも落ちない)
{
  const CheckGroup empty{"empty"};

  FailureCountVisitor counter;
  TextReportVisitor reporter;
  empty.accept(counter);
  empty.accept(reporter);

  EXPECT_EQ(counter.checked_count(), 0);
  EXPECT_EQ(counter.failure_count(), 0);
  EXPECT_EQ(reporter.text(), "[empty]\n");
}

TEST(VisitorTest, variant版が同じ集計結果を返す)
{
  DiagArena arena;
  const std::size_t root = make_variant_tree(arena);

  const std::unique_ptr<CheckGroup> gof_root = make_gof_tree();
  FailureCountVisitor counter;
  gof_root->accept(counter);

  EXPECT_EQ(count_failures(arena, root), 2);
  EXPECT_EQ(count_failures(arena, root), counter.failure_count());
}

TEST(VisitorTest, variant版が同じレポートを返す)
{
  DiagArena arena;
  const std::size_t root = make_variant_tree(arena);

  const std::unique_ptr<CheckGroup> gof_root = make_gof_tree();
  TextReportVisitor reporter;
  gof_root->accept(reporter);

  EXPECT_EQ(make_report(arena, root), std::string(kExpectedReport));
  EXPECT_EQ(make_report(arena, root), reporter.text());
}

TEST(VisitorTest, variant版の葉ひとつでもレポートが作れる)
{
  DiagArena arena;
  const std::size_t only = arena.add(MotorSample{"motor_r", 3U});

  EXPECT_EQ(make_report(arena, only), "motor_r fault=3 NG\n");
  EXPECT_EQ(count_failures(arena, only), 1);
}

TEST(VisitorTest, variant版の型は多態でない)
{
  // GoF 版は vtable を持つ。
  static_assert(std::is_polymorphic_v<SensorCheck>, "GoF 版は多態のはず");
  static_assert(std::is_polymorphic_v<CheckGroup>, "GoF 版は多態のはず");

  // variant 版は仮想関数を 1 つも持たない = vtable ポインタも accept も無い。
  static_assert(!std::is_polymorphic_v<SensorSample>, "variant 版に仮想関数は不要");
  static_assert(!std::is_polymorphic_v<MotorSample>, "variant 版に仮想関数は不要");
  static_assert(!std::is_polymorphic_v<GroupSample>, "variant 版に仮想関数は不要");
  static_assert(!std::is_polymorphic_v<DiagValue>, "std::variant 自体も多態ではない");

  // variant はノードごとのヒープ確保をしない。中身を直接持つので、
  // 最大メンバ + 判別子ぶんの大きさになる。
  static_assert(sizeof(DiagValue) >= sizeof(SensorSample), "variant は中身を直接持つ");

  // 仮想関数が無くても、種類ごとの処理は std::visit で選び分けられている。
  DiagArena arena;
  const std::size_t index = arena.add(SensorSample{"bat", 11800, 13000});
  EXPECT_EQ(arena.size(), 1U);
  EXPECT_EQ(count_failures(arena, index), 0);
  EXPECT_EQ(make_report(arena, index), "bat 11800mV OK\n");
}

TEST(VisitorTest, variant版はget_ifで中身を取り出せる)
{
  // -fno-exceptions のマイコンではこちらを使う（std::get は throw しうる）。
  DiagArena arena;
  const std::size_t index = arena.add(SensorSample{"bat", 11800, 13000});

  const DiagValue & value = arena.at(index);
  const SensorSample * const sensor = std::get_if<SensorSample>(&value);
  ASSERT_NE(sensor, nullptr) << "add() が値を保存できていません";
  EXPECT_EQ(sensor->name, "bat");
  EXPECT_EQ(std::get_if<MotorSample>(&value), nullptr);
}
