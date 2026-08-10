// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "drill/diagnostic_tree.hpp"

// --- 型の性質（コンパイル時に検査される） ---------------------------------
// unique_ptr のメンバを持つクラスはコピーできません。ムーブだけできます。
static_assert(
  !std::is_copy_constructible<drill::DiagnosticGroup>::value,
  "DiagnosticGroup がコピー構築できてしまっています");
static_assert(
  !std::is_copy_assignable<drill::DiagnosticGroup>::value,
  "DiagnosticGroup がコピー代入できてしまっています");
static_assert(
  std::is_move_constructible<drill::DiagnosticGroup>::value,
  "DiagnosticGroup がムーブ構築できません");
static_assert(
  std::is_move_assignable<drill::DiagnosticGroup>::value,
  "DiagnosticGroup がムーブ代入できません");
// 基底ポインタで delete するので仮想デストラクタが要ります。
static_assert(
  std::has_virtual_destructor<drill::DiagnosticEntry>::value,
  "DiagnosticEntry の仮想デストラクタがありません");

namespace
{

std::unique_ptr<drill::DiagnosticCheck> make_check(std::string name, bool passes)
{
  return std::make_unique<drill::DiagnosticCheck>(std::move(name), passes);
}

/// robot
///   imu
///     imu_whoami (合格)
///     imu_bias   (不合格)
///   motors
///     motor_l    (合格)
///     motor_r    (合格)
///   battery_voltage (合格)
std::unique_ptr<drill::DiagnosticGroup> make_tree()
{
  auto imu = std::make_unique<drill::DiagnosticGroup>("imu");
  imu->add(make_check("imu_whoami", true));
  imu->add(make_check("imu_bias", false));

  auto motors = std::make_unique<drill::DiagnosticGroup>("motors");
  motors->add(make_check("motor_l", true));
  motors->add(make_check("motor_r", true));

  auto root = std::make_unique<drill::DiagnosticGroup>("robot");
  root->add(std::move(imu));
  root->add(std::move(motors));
  root->add(make_check("battery_voltage", true));
  return root;
}

}  // namespace

TEST(CompositeTest, 葉と節を同じ型のポインタで扱える)
{
  const drill::DiagnosticCheck leaf{"encoder_z", true};
  const auto tree = make_tree();

  // 葉もグループも const DiagnosticEntry * として同じ配列に入る。
  const std::vector<const drill::DiagnosticEntry *> entries = {&leaf, tree.get()};

  ASSERT_EQ(entries.size(), 2u);
  EXPECT_EQ(entries[0]->check_count(), 1u) << "葉の check_count() は 1 です";
  EXPECT_EQ(entries[1]->check_count(), 5u) << "木全体の葉の数は 5 です";

  std::size_t total = 0;
  for (const drill::DiagnosticEntry * entry : entries) {
    total += entry->check_count();
  }
  EXPECT_EQ(total, 6u);
}

TEST(CompositeTest, check_countが再帰的に数える)
{
  const auto tree = make_tree();

  EXPECT_EQ(tree->child_count(), 3u) << "root の直下は imu / motors / battery_voltage の 3 つです";
  EXPECT_EQ(tree->check_count(), 5u)
    << "グループ自身は数えません。葉だけを再帰的に数えます";
}

TEST(CompositeTest, 合否が再帰的に集計される)
{
  const auto tree = make_tree();

  const drill::DiagnosticResult result = tree->run();
  EXPECT_EQ(result.passed, 4u);
  EXPECT_EQ(result.failed, 1u);
  EXPECT_FALSE(result.all_passed());
  EXPECT_EQ(result.passed + result.failed, tree->check_count())
    << "run() で数えた総数と check_count() が一致していません";
}

TEST(CompositeTest, 部分木も単体の診断も同じ呼び方で実行できる)
{
  const drill::DiagnosticCheck leaf{"encoder_z", true};
  const drill::DiagnosticResult leaf_result = leaf.run();
  EXPECT_EQ(leaf_result.passed, 1u);
  EXPECT_EQ(leaf_result.failed, 0u);
  EXPECT_TRUE(leaf_result.all_passed());

  const drill::DiagnosticCheck broken{"imu_bias", false};
  const drill::DiagnosticResult broken_result = broken.run();
  EXPECT_EQ(broken_result.passed, 0u);
  EXPECT_EQ(broken_result.failed, 1u);
  EXPECT_FALSE(broken_result.all_passed());

  drill::DiagnosticGroup motors{"motors"};
  motors.add(make_check("motor_l", true));
  motors.add(make_check("motor_r", true));
  EXPECT_TRUE(motors.run().all_passed());
}

TEST(CompositeTest, フルパスが自分から子の順に積まれる)
{
  const auto tree = make_tree();

  const std::vector<std::string> expected = {
    "/robot",
    "/robot/imu",
    "/robot/imu/imu_whoami",
    "/robot/imu/imu_bias",
    "/robot/motors",
    "/robot/motors/motor_l",
    "/robot/motors/motor_r",
    "/robot/battery_voltage",
  };
  EXPECT_EQ(tree->full_names(), expected);
}

TEST(CompositeTest, 親を破棄すると子も破棄される)
{
  drill::destruction_log().clear();

  {
    drill::DiagnosticGroup root{"robot"};
    auto motors = std::make_unique<drill::DiagnosticGroup>("motors");
    motors->add(make_check("motor_l", true));
    root.add(std::move(motors));
    root.add(make_check("battery_voltage", true));

    ASSERT_EQ(root.check_count(), 2u) << "add() が子を受け取れていません";
    EXPECT_TRUE(drill::destruction_log().empty()) << "まだ何も破棄されていないはずです";
  }

  // 親の unique_ptr が死ねば、木は丸ごと死ぬ。delete を 1 つも書いていない。
  std::vector<std::string> destroyed = drill::destruction_log();
  ASSERT_EQ(destroyed.size(), 4u)
    << "破棄されたのは " << destroyed.size() << " 個です。子が親に所有されていません";
  EXPECT_EQ(destroyed.front(), "robot") << "親のデストラクタ本体が先に走ります";

  std::sort(destroyed.begin(), destroyed.end());
  const std::vector<std::string> expected = {"battery_voltage", "motor_l", "motors", "robot"};
  EXPECT_EQ(destroyed, expected);

  drill::destruction_log().clear();
}

TEST(CompositeTest, グループはコピーできずムーブできる)
{
  // コピー不可・ムーブ可はファイル先頭の static_assert がコンパイル時に見ています。
  drill::DiagnosticGroup source{"motors"};
  source.add(make_check("motor_l", true));
  source.add(make_check("motor_r", false));
  ASSERT_EQ(source.check_count(), 2u) << "add() が子を受け取れていません";

  const drill::DiagnosticGroup moved{std::move(source)};
  EXPECT_EQ(moved.check_count(), 2u) << "ムーブで子が失われています";
  EXPECT_EQ(moved.run().failed, 1u);
}

TEST(CompositeTest, 空のグループは診断ゼロで合格扱い)
{
  const drill::DiagnosticGroup empty{"unused"};

  EXPECT_EQ(empty.child_count(), 0u);
  EXPECT_EQ(empty.check_count(), 0u);
  EXPECT_EQ(empty.run().passed, 0u);
  EXPECT_TRUE(empty.run().all_passed());

  const std::vector<std::string> expected = {"/unused"};
  EXPECT_EQ(empty.full_names(), expected);
}

TEST(CompositeTest, nullptrを追加しても壊れない)
{
  drill::DiagnosticGroup group{"imu"};
  group.add(nullptr);
  group.add(make_check("imu_whoami", true));
  group.add(std::unique_ptr<drill::DiagnosticEntry>{});

  EXPECT_EQ(group.child_count(), 1u) << "nullptr を children_ に入れてはいけません";
  EXPECT_EQ(group.check_count(), 1u);
  EXPECT_TRUE(group.run().all_passed());
}
