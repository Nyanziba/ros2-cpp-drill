// I AM NOT DONE
//
// 結城本 第19章 State を C++ で書きます。
// 同じ 1 つの遷移規則を、3 通りの手段で実装して比べます。
//
//   手段1: enum + switch          （状態は「値」。vtable もヒープもゼロ）
//   手段2: State クラス（GoF 版） （状態は「派生クラス」。static 実体で持てばヒープはゼロ）
//   手段3: std::variant + visit   （状態は「直和型」。状態ごとのデータを持てて、ヒープはゼロ）
//
// 遷移規則は include/drill/state_machine.hpp の MachineState のコメントにあります。

#include "drill/state_machine.hpp"

#include <type_traits>

// ---------------------------------------------------------------------------
// 手段1: enum + switch
// ---------------------------------------------------------------------------

MachineState EnumStateMachine::next_state(MachineState current, MachineEvent event)
{
  // TODO: 遷移表どおりに遷移先を返してください。
  //
  //   - EmergencyStop はどの状態からでも Faulted へ。ただし既に Faulted なら current のまま
  //   - Stopped + PowerOn -> Idle
  //   - Idle    + Start   -> Running
  //   - Running + Stop    -> Idle
  //   - Faulted + Reset   -> Stopped
  //   - それ以外は current をそのまま返す（＝入力は無視される）
  //
  // switch を書くときは default: を書かずに全 enum 値を列挙してください。
  // そうすると、あとで状態を足したときにコンパイラが「書き漏れ」を警告してくれます。
  static_cast<void>(event);
  return current;
}

bool EnumStateMachine::handle(MachineEvent event)
{
  // TODO:
  //   1. next_state() で遷移先を求める
  //   2. 遷移先が今と同じなら、何もせず false を返す（アクションも走らせない）
  //   3. 違うなら log_exit(log_, 今の状態) → 状態を差し替え → log_enter(log_, 新しい状態)
  //      の順に実行して true を返す
  //
  // 「退場が先、入場が後」です。逆にすると、実機ではモータが回ったまま次の状態に入ります。
  static_cast<void>(event);
  static_cast<void>(log_);
  return false;
}

// ---------------------------------------------------------------------------
// 手段2: State クラス（GoF 版）
// ---------------------------------------------------------------------------

namespace
{

/// 4 つの状態クラス。どれもメンバ変数を 1 つも持ちません。
/// だから実体は state_object() の中に static で 1 個ずつ置けば足ります。
class StoppedStateObject : public State
{
public:
  MachineState id() const override { return MachineState::Stopped; }

  const State * handle(MachineEvent event) const override
  {
    // TODO: EmergencyStop なら Faulted、PowerOn なら Idle の実体を返してください。
    //       それ以外は「遷移しない」の意味で this を返します。
    //       遷移先は state_object(MachineState::Idle) のように取ります。
    static_cast<void>(event);
    return this;
  }
};

class IdleStateObject : public State
{
public:
  MachineState id() const override { return MachineState::Idle; }

  const State * handle(MachineEvent event) const override
  {
    // TODO: EmergencyStop なら Faulted、Start なら Running。それ以外は this。
    static_cast<void>(event);
    return this;
  }
};

class RunningStateObject : public State
{
public:
  MachineState id() const override { return MachineState::Running; }

  const State * handle(MachineEvent event) const override
  {
    // TODO: EmergencyStop なら Faulted、Stop なら Idle。それ以外は this。
    static_cast<void>(event);
    return this;
  }

  // TODO: on_exit(TransitionLog & log) を override してください。
  //       基底の State::on_exit(log) を呼んでから、log.record("motor:stop") を足します。
  //       走行状態から抜けるときは必ずモータを止める、という実機の要求がここに入ります。
};

class FaultedStateObject : public State
{
public:
  MachineState id() const override { return MachineState::Faulted; }

  const State * handle(MachineEvent event) const override
  {
    // TODO: Reset のときだけ Stopped。それ以外は（EmergencyStop も含めて）this。
    static_cast<void>(event);
    return this;
  }

  // TODO: on_enter(TransitionLog & log) を override してください。
  //       基底の State::on_enter(log) を呼んでから、log.record("brake:engage") を足します。
};

}  // namespace

const State * state_object(MachineState id)
{
  // 関数内 static。使われる直前に 1 度だけ初期化されるので、
  // 翻訳単位をまたいだ初期化順序の問題が起きません（第5章 Singleton と同じ理由）。
  static const StoppedStateObject stopped;
  static const IdleStateObject idle;
  static const RunningStateObject running;
  static const FaultedStateObject faulted;

  // TODO: id に対応する実体のアドレスを返してください。
  //       state_object() は何度呼んでも同じアドレスを返さなければいけません。
  static_cast<void>(idle);
  static_cast<void>(running);
  static_cast<void>(faulted);
  static_cast<void>(id);
  return &stopped;
}

bool ClassStateMachine::handle(MachineEvent event)
{
  // TODO:
  //   1. const State * next = current_->handle(event); で遷移先を「受け取る」
  //   2. next == current_ なら false（遷移しない）
  //   3. current_->on_exit(log_) → current_ = next → current_->on_enter(log_) → true
  //
  // 【なぜこの順で書くのか】
  // Java 版の State は handle() の中で context.setState(this に代わる状態) を呼びます。
  // C++ でその形をまねて、状態オブジェクトを unique_ptr で持って handle() の中で
  // reset() すると、handle() の残りの行は**既に破棄された this** の上で走ります。
  // 遷移先を戻り値で返す形なら、差し替えるのは呼び出し側なので構造的に起きません。
  static_cast<void>(event);
  static_cast<void>(log_);
  return false;
}

// ---------------------------------------------------------------------------
// 手段3: std::variant + std::visit
// ---------------------------------------------------------------------------

MachineState id_of(const StateVariant & state)
{
  // TODO: std::visit と if constexpr で、どの型が入っているかを MachineState に写してください。
  //
  //   return std::visit([](const auto & concrete) {
  //     using T = std::decay_t<decltype(concrete)>;
  //     if constexpr (std::is_same_v<T, StoppedState>) { return MachineState::Stopped; }
  //     else if constexpr ( ... ) { ... }
  //   }, state);
  //
  // std::holds_alternative で 4 回書いても動きますが、状態を足したときに
  // 書き漏れても気づけません。visit なら「全部の型を扱ったか」をコンパイラが見ます。
  static_cast<void>(state);
  return MachineState::Stopped;
}

std::optional<StateVariant> VariantStateMachine::next_state(
  const StateVariant & current, MachineEvent event)
{
  // TODO: 遷移先を返してください。遷移しないなら std::nullopt。
  //
  //   - EmergencyStop はどの状態からでも FaultedState{MachineEvent::EmergencyStop} へ
  //     （既に Faulted なら nullopt）
  //   - Idle + Start は RunningState{kCruiseDutyPercent} へ。
  //     enum 版と違い、**状態が固有のデータを持てる**のがこの手段の取り柄です
  //   - それ以外は enum 版と同じ表
  static_cast<void>(current);
  static_cast<void>(event);
  return std::nullopt;
}

bool VariantStateMachine::handle(MachineEvent event)
{
  // TODO: enum 版の handle() と同じ流れです。
  //   next_state() で遷移先を受け取り、nullopt なら false。
  //   そうでなければ log_exit → 差し替え → log_enter の順で true。
  static_cast<void>(event);
  static_cast<void>(log_);
  return false;
}
