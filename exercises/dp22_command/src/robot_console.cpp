// I AM NOT DONE
//
// 結城本 第22章 Command を C++ で書きます。
//
// 実装するのは 4 グループです。
//   1. RotateCommand / GripperCommand  — 逆操作が自明な場合と、そうでない場合
//   2. MacroCommand                    — 複数のコマンドを 1 つとして扱う（Composite との合流）
//   3. CommandHistory                  — undo / redo
//   4. ActionQueue / MotorCommandRing / apply
//                                      — std::function 版と、マイコン向けの POD 版
//
// 「そもそもクラスにする必要があるか」を毎回考えてください。
// undo が要らないなら std::function<void()> で足ります。

#include "drill/robot_console.hpp"

#include <utility>

// ---------------------------------------------------------------------------
// 1. 単体のコマンド
// ---------------------------------------------------------------------------

void RotateCommand::execute()
{
  // TODO: arm_ を delta_deg_ だけ回してください。
  //       RobotArm::rotate(double) を呼ぶだけです。
  (void)arm_;        // 実装したらこの 2 行は消してください（未使用警告よけ）
  (void)delta_deg_;
}

void RotateCommand::undo()
{
  // TODO: 逆操作を実行してください。
  //       回転は相対値なので、実行前の状態を保存する必要がありません。
  //       -delta_deg_ を渡すだけです。
  //
  //       この「逆操作が書ける」ことが Command で undo する条件です。
  //       書けないときは第18章 Memento のように状態を丸ごと保存します。
}

std::string RotateCommand::name() const
{
  // TODO: "rotate" を返してください。
  return {};
}

void GripperCommand::execute()
{
  // TODO: 2 段です。
  //   1. previous_ に **今の** arm_->gripper_closed() を控える
  //   2. arm_->set_gripper(closed_) を呼ぶ
  //
  // 1 を飛ばすと undo が壊れます。「閉じる」の逆は「開く」ではありません。
  // もともと閉じていたなら、undo 後も閉じたままが正解です。
  (void)arm_;     // 実装したらこの 2 行は消してください（未使用警告よけ）
  (void)closed_;
}

void GripperCommand::undo()
{
  // TODO: 控えておいた previous_ に戻してください。
}

std::string GripperCommand::name() const
{
  // TODO: closed_ が true なら "grip"、false なら "release" を返してください。
  return {};
}

// ---------------------------------------------------------------------------
// 2. マクロコマンド（Composite）
// ---------------------------------------------------------------------------

void MacroCommand::add(std::unique_ptr<Command> command)
{
  // TODO: commands_ の末尾に move して積んでください。
  //       command が nullptr のときは積まないこと（execute() で落ちます）。
  //
  //       引数を値で受けて std::move するのが unique_ptr の受け取り方です。
  //       const std::unique_ptr<Command> & で受けると、そもそもコピーできません。
  (void)command;
}

void MacroCommand::execute()
{
  // TODO: commands_ を**先頭から**順に execute() してください。
}

void MacroCommand::undo()
{
  // TODO: commands_ を**末尾から**逆順に undo() してください。
  //
  //       順番が本題です。「A したあと B した」を取り消すなら、
  //       先に B を取り消してから A を取り消します。
  //       正順で undo すると、依存のある操作で状態が壊れます。
}

std::string MacroCommand::name() const
{
  // TODO: "macro" を返してください。
  return {};
}

// ---------------------------------------------------------------------------
// 3. 履歴（Invoker）
// ---------------------------------------------------------------------------

void CommandHistory::run(std::unique_ptr<Command> command)
{
  // TODO: 3 段です。
  //   1. command が nullptr なら何もしない
  //   2. command->execute() を呼ぶ
  //   3. undone_ を clear() してから、done_ に move して積む
  //
  // 3 の clear() を忘れると、undo したあと別のコマンドを実行してから
  // redo できてしまい、状態が復元できなくなります。
  (void)command;
}

bool CommandHistory::undo()
{
  // TODO: done_ が空なら false。
  //       そうでなければ末尾のコマンドの undo() を呼び、
  //       そのコマンドを undone_ へ move して done_ から pop_back し、true を返す。
  //
  // 注意: done_.back() を move したあとに pop_back すること。順番を逆にすると
  //       すでに解放されたものを触ります。
  return false;
}

bool CommandHistory::redo()
{
  // TODO: undone_ が空なら false。
  //       そうでなければ末尾のコマンドの execute() を呼び、
  //       done_ へ戻して true を返す。
  //
  // ここでは undone_ を clear() しません。run() だけが捨てます。
  return false;
}

// ---------------------------------------------------------------------------
// 4. std::function 版とマイコン版
// ---------------------------------------------------------------------------

void ActionQueue::push(std::function<void()> action)
{
  // TODO: actions_ の末尾に move して積んでください。
  //       action が空（未設定の std::function）なら積まないこと。
  //       空の std::function を呼ぶと std::bad_function_call を投げます。
  //       if (action) { ... } で判定できます。
  (void)action;
}

void ActionQueue::run_all()
{
  // TODO: 積んだ順に全部呼び、そのあと actions_ を clear() してください。
  //
  //       これが「undo の無い Command」です。クラスを 1 つも作らずに、
  //       実行を後回しにするという Command の目的だけを果たしています。
}

bool MotorCommandRing::push(const MotorCommand & command)
{
  // TODO: 固定長リングバッファに積んでください。std::vector は使いません。
  //
  //   - 満杯（size_ == kCapacity）なら、最も古いものを 1 つ落とします。
  //     head_ が「次に書く位置」なので、満杯のときは head_ が最古の位置でもあります。
  //     上書きしたうえで size_ は kCapacity のまま、戻り値は false。
  //   - 満杯でなければ buffer_[head_] に書いて ++size_、戻り値は true。
  //   - どちらの場合も head_ = (head_ + 1) % kCapacity で進めます。
  (void)command;
  (void)buffer_;  // 実装したらこの 2 行は消してください（未使用警告よけ）
  (void)head_;
  return false;
}

bool MotorCommandRing::pop(MotorCommand & out)
{
  // TODO: 最も古いものを out に書いて取り出してください。
  //
  //   - 空なら out を触らず false。
  //   - 最古の位置は (head_ + kCapacity - size_) % kCapacity です。
  //     head_ から size_ 個ぶん戻った場所。負にならないよう kCapacity を足してから % を取ります。
  //   - 取り出したら --size_。head_ は動かしません。
  (void)out;
  return false;
}

void apply(RobotArm & arm, const MotorCommand & command)
{
  // TODO: command.kind で switch して arm を操作してください。
  //
  //   kRotate  → arm.rotate(command.argument)   ※ double への変換は明示的に書くこと
  //   kGrip    → arm.set_gripper(true)
  //   kRelease → arm.set_gripper(false)
  //   kNone    → 何もしない
  //
  // これが「virtual の代わりの switch」です。vtable も動的確保も要りません。
  (void)arm;
  (void)command;
}
