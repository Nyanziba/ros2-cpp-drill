// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "drill/robot_console.hpp"

namespace
{

std::unique_ptr<Command> rotate(RobotArm & arm, double delta_deg)
{
  return std::unique_ptr<Command>(new RotateCommand(arm, delta_deg));
}

std::unique_ptr<Command> gripper(RobotArm & arm, bool closed)
{
  return std::unique_ptr<Command>(new GripperCommand(arm, closed));
}

}  // namespace

TEST(CommandTest, コマンドは積んだ順に実行される)
{
  RobotArm arm;
  CommandHistory history;

  history.run(rotate(arm, 30.0));
  history.run(gripper(arm, true));
  history.run(rotate(arm, -10.0));

  const std::vector<std::string> expected = {"rotate 30", "grip", "rotate -10"};
  EXPECT_EQ(arm.log(), expected) << "実行の順序が積んだ順になっていません";

  EXPECT_DOUBLE_EQ(arm.angle_deg(), 20.0);
  EXPECT_TRUE(arm.gripper_closed());
  EXPECT_EQ(history.undo_depth(), 3u);
  EXPECT_EQ(history.redo_depth(), 0u);
}

TEST(CommandTest, undoは1つずつ逆順に戻る)
{
  RobotArm arm;
  CommandHistory history;

  history.run(rotate(arm, 30.0));
  history.run(rotate(arm, 5.0));
  ASSERT_DOUBLE_EQ(arm.angle_deg(), 35.0);
  arm.clear_log();

  EXPECT_TRUE(history.undo());
  EXPECT_DOUBLE_EQ(arm.angle_deg(), 30.0) << "最後に実行したコマンドから取り消してください";
  EXPECT_EQ(history.undo_depth(), 1u);
  EXPECT_EQ(history.redo_depth(), 1u);

  EXPECT_TRUE(history.undo());
  EXPECT_DOUBLE_EQ(arm.angle_deg(), 0.0);
  EXPECT_EQ(history.undo_depth(), 0u);
  EXPECT_EQ(history.redo_depth(), 2u);

  EXPECT_FALSE(history.undo()) << "履歴が空なら undo は false を返します";

  const std::vector<std::string> expected = {"rotate -5", "rotate -30"};
  EXPECT_EQ(arm.log(), expected) << "undo が逆順（後に実行したものが先）になっていません";
}

TEST(CommandTest, redoで再実行できる)
{
  RobotArm arm;
  CommandHistory history;

  history.run(rotate(arm, 30.0));
  history.run(rotate(arm, 5.0));

  ASSERT_TRUE(history.undo());
  ASSERT_TRUE(history.undo());
  ASSERT_DOUBLE_EQ(arm.angle_deg(), 0.0);
  arm.clear_log();

  EXPECT_TRUE(history.redo());
  EXPECT_DOUBLE_EQ(arm.angle_deg(), 30.0) << "redo は取り消した順の逆から戻します";

  EXPECT_TRUE(history.redo());
  EXPECT_DOUBLE_EQ(arm.angle_deg(), 35.0);

  EXPECT_FALSE(history.redo()) << "redo するものが無ければ false";
  EXPECT_EQ(history.undo_depth(), 2u);
  EXPECT_EQ(history.redo_depth(), 0u);

  const std::vector<std::string> expected = {"rotate 30", "rotate 5"};
  EXPECT_EQ(arm.log(), expected);
}

TEST(CommandTest, 新しいコマンドを実行するとredoの履歴は捨てられる)
{
  RobotArm arm;
  CommandHistory history;

  history.run(rotate(arm, 30.0));
  ASSERT_TRUE(history.undo());
  ASSERT_EQ(history.redo_depth(), 1u);

  history.run(rotate(arm, 7.0));

  EXPECT_EQ(history.redo_depth(), 0u)
    << "run() のたびに redo 履歴を捨ててください。分岐した歴史に redo すると状態が壊れます";
  EXPECT_FALSE(history.redo());
  EXPECT_DOUBLE_EQ(arm.angle_deg(), 7.0);
}

TEST(CommandTest, 逆操作が自明でない操作は実行前の状態を保存する)
{
  RobotArm arm;
  arm.set_gripper(true);  // もともと閉じている
  arm.clear_log();

  GripperCommand close_again(arm, true);
  close_again.execute();
  EXPECT_TRUE(close_again.previous())
    << "execute() が実行前の状態を控えていません";
  EXPECT_TRUE(arm.gripper_closed());

  close_again.undo();
  EXPECT_TRUE(arm.gripper_closed())
    << "「閉じる」の逆は「開く」ではありません。実行前が閉じていたなら閉じたままです";

  // 逆に、開いている状態から閉じたなら undo で開く。
  arm.set_gripper(false);
  GripperCommand close(arm, true);
  close.execute();
  EXPECT_FALSE(close.previous());
  EXPECT_TRUE(arm.gripper_closed());
  close.undo();
  EXPECT_FALSE(arm.gripper_closed());
}

TEST(CommandTest, マクロコマンドは1つのコマンドとして扱える)
{
  RobotArm arm;

  auto macro = std::unique_ptr<MacroCommand>(new MacroCommand());
  macro->add(rotate(arm, 90.0));
  macro->add(gripper(arm, true));
  macro->add(rotate(arm, -20.0));
  EXPECT_EQ(macro->size(), 3u);
  EXPECT_EQ(macro->name(), "macro");

  // MacroCommand は Command でもある（Composite）。
  static_assert(
    std::is_base_of<Command, MacroCommand>::value,
    "MacroCommand は Command を実装している必要があります");

  CommandHistory history;
  history.run(std::move(macro));

  EXPECT_EQ(history.undo_depth(), 1u)
    << "マクロは 3 個ではなく 1 個のコマンドとして積まれます";

  const std::vector<std::string> after_execute = {"rotate 90", "grip", "rotate -20"};
  EXPECT_EQ(arm.log(), after_execute);
  EXPECT_DOUBLE_EQ(arm.angle_deg(), 70.0);

  arm.clear_log();

  // undo は 1 回で全部戻る。しかも中身は逆順。
  EXPECT_TRUE(history.undo());
  const std::vector<std::string> after_undo = {"rotate 20", "release", "rotate -90"};
  EXPECT_EQ(arm.log(), after_undo)
    << "マクロの undo は末尾のコマンドから逆順に取り消してください";

  EXPECT_DOUBLE_EQ(arm.angle_deg(), 0.0);
  EXPECT_FALSE(arm.gripper_closed());
  EXPECT_EQ(history.undo_depth(), 0u);
}

TEST(CommandTest, マクロの中にマクロを入れられる)
{
  RobotArm arm;

  auto inner = std::unique_ptr<MacroCommand>(new MacroCommand());
  inner->add(rotate(arm, 10.0));
  inner->add(rotate(arm, 20.0));

  MacroCommand outer;
  outer.add(rotate(arm, 1.0));
  outer.add(std::move(inner));
  EXPECT_EQ(outer.size(), 2u);

  outer.execute();
  EXPECT_DOUBLE_EQ(arm.angle_deg(), 31.0);

  const std::vector<std::string> expected = {"rotate 1", "rotate 10", "rotate 20"};
  EXPECT_EQ(arm.log(), expected);

  arm.clear_log();
  outer.undo();
  EXPECT_DOUBLE_EQ(arm.angle_deg(), 0.0);

  const std::vector<std::string> undone = {"rotate -20", "rotate -10", "rotate -1"};
  EXPECT_EQ(arm.log(), undone);
}

TEST(CommandTest, std_function版でも同じ実行結果になる)
{
  RobotArm class_arm;
  RobotArm function_arm;

  // クラス版。
  CommandHistory history;
  history.run(rotate(class_arm, 30.0));
  history.run(gripper(class_arm, true));
  history.run(rotate(class_arm, -10.0));

  // std::function 版。クラスは 1 つも作っていません。
  ActionQueue queue;
  EXPECT_TRUE(queue.empty());
  queue.push([&function_arm]() { function_arm.rotate(30.0); });
  queue.push([&function_arm]() { function_arm.set_gripper(true); });
  queue.push([&function_arm]() { function_arm.rotate(-10.0); });
  EXPECT_EQ(queue.size(), 3u) << "積んだだけでは実行されません";

  // 積んだ時点ではまだ何も起きていない。これが Command の目的そのものです。
  EXPECT_DOUBLE_EQ(function_arm.angle_deg(), 0.0);

  queue.run_all();

  EXPECT_EQ(function_arm.log(), class_arm.log())
    << "std::function 版とクラス版で実行結果が一致しません";
  EXPECT_DOUBLE_EQ(function_arm.angle_deg(), class_arm.angle_deg());
  EXPECT_TRUE(queue.empty()) << "run_all() のあとキューは空になります";

  // 空の std::function は積まない（呼ぶと std::bad_function_call）。
  queue.push(std::function<void()>{});
  EXPECT_EQ(queue.size(), 0u);
}

TEST(CommandTest, マイコン版のコマンドはPODで割り込みから積める)
{
  // 動的確保も仮想関数も無いこと。ISR から触るのでこれが条件です。
  static_assert(
    std::is_trivially_copyable<MotorCommand>::value,
    "MotorCommand は trivially copyable でなければなりません");
  static_assert(
    !std::is_polymorphic<MotorCommand>::value,
    "MotorCommand に vtable があってはいけません");
  static_assert(
    sizeof(MotorCommand) <= 4,
    "MotorCommand が大きすぎます。enum + 引数だけにしてください");

  RobotArm arm;
  MotorCommandRing ring;
  EXPECT_TRUE(ring.empty());

  // ISR 側に相当する部分。
  EXPECT_TRUE(ring.push(MotorCommand{MotorCommandKind::kRotate, 45}));
  EXPECT_TRUE(ring.push(MotorCommand{MotorCommandKind::kGrip, 0}));
  EXPECT_TRUE(ring.push(MotorCommand{MotorCommandKind::kRotate, -5}));
  EXPECT_EQ(ring.size(), 3u);

  // メインループ側に相当する部分。
  MotorCommand command;
  while (ring.pop(command)) {
    apply(arm, command);
  }

  EXPECT_TRUE(ring.empty());
  EXPECT_DOUBLE_EQ(arm.angle_deg(), 40.0);
  EXPECT_TRUE(arm.gripper_closed());

  const std::vector<std::string> expected = {"rotate 45", "grip", "rotate -5"};
  EXPECT_EQ(arm.log(), expected) << "リングバッファは積んだ順に取り出します（FIFO）";

  EXPECT_FALSE(ring.pop(command)) << "空のときは false";
}

TEST(CommandTest, リングバッファは容量を超えると古いコマンドから落ちる)
{
  MotorCommandRing ring;

  for (std::size_t i = 0; i < MotorCommandRing::kCapacity; ++i) {
    const std::int16_t argument = static_cast<std::int16_t>(i);
    EXPECT_TRUE(ring.push(MotorCommand{MotorCommandKind::kRotate, argument}))
      << "満杯になるまでは何も落ちません（i = " << i << "）";
  }
  EXPECT_EQ(ring.size(), MotorCommandRing::kCapacity);

  // ここから先は最古のものを押し出す。
  EXPECT_FALSE(ring.push(MotorCommand{MotorCommandKind::kRotate, 100}))
    << "満杯で押し出したときは false を返します";
  EXPECT_FALSE(ring.push(MotorCommand{MotorCommandKind::kRotate, 101}));

  EXPECT_EQ(ring.size(), MotorCommandRing::kCapacity)
    << "容量を超えて増えています。リングバッファになっていません";

  // 残っているのは古い方から 2 個が落ちた残り。
  MotorCommand command;
  for (std::size_t i = 2; i < MotorCommandRing::kCapacity; ++i) {
    ASSERT_TRUE(ring.pop(command));
    EXPECT_EQ(command.argument, static_cast<std::int16_t>(i))
      << "落とすのは最も古いものです（i = " << i << "）";
  }
  ASSERT_TRUE(ring.pop(command));
  EXPECT_EQ(command.argument, 100);
  ASSERT_TRUE(ring.pop(command));
  EXPECT_EQ(command.argument, 101);
  EXPECT_TRUE(ring.empty());
}
