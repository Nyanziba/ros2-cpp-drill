// 解答例: 結城本 第22章 Command
//
// 見どころは 3 点です。
//   1. 逆操作が自明な RotateCommand と、実行前の状態を控える GripperCommand の差
//   2. MacroCommand の undo が逆順であること（Composite との合流点）
//   3. undo が要らない経路は std::function、マイコンでは POD + リングバッファ

#include "drill/robot_console.hpp"

#include <utility>

// ---------------------------------------------------------------------------
// 1. 単体のコマンド
// ---------------------------------------------------------------------------

void RotateCommand::execute()
{
  arm_->rotate(delta_deg_);
}

void RotateCommand::undo()
{
  // 相対回転なので逆操作が書けます。状態を保存する必要がありません。
  arm_->rotate(-delta_deg_);
}

std::string RotateCommand::name() const
{
  return "rotate";
}

void GripperCommand::execute()
{
  // 逆操作が定義できないので、実行前の状態を控えます。
  // Command の中に最小の Memento（第18章）を持っている形です。
  previous_ = arm_->gripper_closed();
  arm_->set_gripper(closed_);
}

void GripperCommand::undo()
{
  arm_->set_gripper(previous_);
}

std::string GripperCommand::name() const
{
  return closed_ ? "grip" : "release";
}

// ---------------------------------------------------------------------------
// 2. マクロコマンド（Composite）
// ---------------------------------------------------------------------------

void MacroCommand::add(std::unique_ptr<Command> command)
{
  if (command == nullptr) {
    return;
  }
  commands_.push_back(std::move(command));
}

void MacroCommand::execute()
{
  for (const std::unique_ptr<Command> & command : commands_) {
    command->execute();
  }
}

void MacroCommand::undo()
{
  // 逆順。「A のあと B」を取り消すなら、B を取り消してから A を取り消します。
  for (std::size_t i = commands_.size(); i > 0; --i) {
    commands_[i - 1]->undo();
  }
}

std::string MacroCommand::name() const
{
  return "macro";
}

// ---------------------------------------------------------------------------
// 3. 履歴（Invoker）
// ---------------------------------------------------------------------------

void CommandHistory::run(std::unique_ptr<Command> command)
{
  if (command == nullptr) {
    return;
  }
  command->execute();
  undone_.clear();  // 歴史が分岐したので redo は捨てる
  done_.push_back(std::move(command));
}

bool CommandHistory::undo()
{
  if (done_.empty()) {
    return false;
  }
  done_.back()->undo();
  undone_.push_back(std::move(done_.back()));
  done_.pop_back();
  return true;
}

bool CommandHistory::redo()
{
  if (undone_.empty()) {
    return false;
  }
  undone_.back()->execute();
  done_.push_back(std::move(undone_.back()));
  undone_.pop_back();
  return true;
}

// ---------------------------------------------------------------------------
// 4. std::function 版とマイコン版
// ---------------------------------------------------------------------------

void ActionQueue::push(std::function<void()> action)
{
  if (!action) {
    return;  // 空の std::function を呼ぶと std::bad_function_call
  }
  actions_.push_back(std::move(action));
}

void ActionQueue::run_all()
{
  for (const std::function<void()> & action : actions_) {
    action();
  }
  actions_.clear();
}

bool MotorCommandRing::push(const MotorCommand & command)
{
  const bool was_full = (size_ == kCapacity);

  buffer_[head_] = command;
  head_ = (head_ + 1) % kCapacity;

  if (!was_full) {
    ++size_;
  }
  // 満杯だったときは head_ の位置が最古だったので、そこを上書きした時点で
  // 最古が 1 つ落ちています。size_ は kCapacity のまま。
  return !was_full;
}

bool MotorCommandRing::pop(MotorCommand & out)
{
  if (size_ == 0) {
    return false;
  }
  const std::size_t oldest = (head_ + kCapacity - size_) % kCapacity;
  out = buffer_[oldest];
  --size_;
  return true;
}

void apply(RobotArm & arm, const MotorCommand & command)
{
  switch (command.kind) {
    case MotorCommandKind::kRotate:
      arm.rotate(static_cast<double>(command.argument));
      break;
    case MotorCommandKind::kGrip:
      arm.set_gripper(true);
      break;
    case MotorCommandKind::kRelease:
      arm.set_gripper(false);
      break;
    case MotorCommandKind::kNone:
      break;
  }
}
