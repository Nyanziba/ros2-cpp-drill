// 解答例。
//
// 同じ遷移規則を 3 通りに書きます。3 つとも同じログを出します。

#include "drill/state_machine.hpp"

#include <type_traits>

// ---------------------------------------------------------------------------
// 手段1: enum + switch
// ---------------------------------------------------------------------------

MachineState EnumStateMachine::next_state(MachineState current, MachineEvent event)
{
  // 非常停止はどの状態からでも入れる。ただし既に Faulted なら「変わらない」。
  if (event == MachineEvent::EmergencyStop) {
    return MachineState::Faulted;
  }

  switch (current) {
    case MachineState::Stopped:
      return (event == MachineEvent::PowerOn) ? MachineState::Idle : current;

    case MachineState::Idle:
      return (event == MachineEvent::Start) ? MachineState::Running : current;

    case MachineState::Running:
      return (event == MachineEvent::Stop) ? MachineState::Idle : current;

    case MachineState::Faulted:
      // 異常からは手動リセットでしか出られない。
      return (event == MachineEvent::Reset) ? MachineState::Stopped : current;
  }
  return current;
}

bool EnumStateMachine::handle(MachineEvent event)
{
  const MachineState next = next_state(state_, event);
  if (next == state_) {
    return false;
  }

  log_exit(log_, state_);
  state_ = next;
  log_enter(log_, state_);
  return true;
}

// ---------------------------------------------------------------------------
// 手段2: State クラス（GoF 版）
// ---------------------------------------------------------------------------

namespace
{

/// 4 つの状態クラス。どれもメンバを持たないので、実体は static に 1 個ずつで足ります。
class StoppedStateObject : public State
{
public:
  MachineState id() const override { return MachineState::Stopped; }

  const State * handle(MachineEvent event) const override
  {
    if (event == MachineEvent::EmergencyStop) {
      return state_object(MachineState::Faulted);
    }
    if (event == MachineEvent::PowerOn) {
      return state_object(MachineState::Idle);
    }
    return this;
  }
};

class IdleStateObject : public State
{
public:
  MachineState id() const override { return MachineState::Idle; }

  const State * handle(MachineEvent event) const override
  {
    if (event == MachineEvent::EmergencyStop) {
      return state_object(MachineState::Faulted);
    }
    if (event == MachineEvent::Start) {
      return state_object(MachineState::Running);
    }
    return this;
  }
};

class RunningStateObject : public State
{
public:
  MachineState id() const override { return MachineState::Running; }

  const State * handle(MachineEvent event) const override
  {
    if (event == MachineEvent::EmergencyStop) {
      return state_object(MachineState::Faulted);
    }
    if (event == MachineEvent::Stop) {
      return state_object(MachineState::Idle);
    }
    return this;
  }

  /// 走行から抜けるときは必ずモータを止める。
  void on_exit(TransitionLog & log) const override
  {
    State::on_exit(log);
    log.record("motor:stop");
  }
};

class FaultedStateObject : public State
{
public:
  MachineState id() const override { return MachineState::Faulted; }

  const State * handle(MachineEvent event) const override
  {
    // 手動リセット以外は受け付けない。EmergencyStop も含めて無視。
    if (event == MachineEvent::Reset) {
      return state_object(MachineState::Stopped);
    }
    return this;
  }

  void on_enter(TransitionLog & log) const override
  {
    State::on_enter(log);
    log.record("brake:engage");
  }
};

}  // namespace

const State * state_object(MachineState id)
{
  // 関数内 static。初期化順序の問題が起きません（第5章 Singleton と同じ理由）。
  static const StoppedStateObject stopped;
  static const IdleStateObject idle;
  static const RunningStateObject running;
  static const FaultedStateObject faulted;

  switch (id) {
    case MachineState::Stopped:
      return &stopped;
    case MachineState::Idle:
      return &idle;
    case MachineState::Running:
      return &running;
    case MachineState::Faulted:
      return &faulted;
  }
  return &stopped;
}

bool ClassStateMachine::handle(MachineEvent event)
{
  // 遷移先を「受け取ってから」差し替える。
  // ここで current_->handle(event) の中から current_ を書き換えていたら、
  // 続きの on_exit() は既に消えたオブジェクトのメンバ関数になります。
  const State * const next = current_->handle(event);
  if (next == current_) {
    return false;
  }

  current_->on_exit(log_);
  current_ = next;
  current_->on_enter(log_);
  return true;
}

// ---------------------------------------------------------------------------
// 手段3: std::variant + std::visit
// ---------------------------------------------------------------------------

MachineState id_of(const StateVariant & state)
{
  return std::visit(
    [](const auto & concrete) {
      using T = std::decay_t<decltype(concrete)>;
      if constexpr (std::is_same_v<T, StoppedState>) {
        return MachineState::Stopped;
      } else if constexpr (std::is_same_v<T, IdleState>) {
        return MachineState::Idle;
      } else if constexpr (std::is_same_v<T, RunningState>) {
        return MachineState::Running;
      } else {
        return MachineState::Faulted;
      }
    },
    state);
}

std::optional<StateVariant> VariantStateMachine::next_state(
  const StateVariant & current, MachineEvent event)
{
  if (event == MachineEvent::EmergencyStop) {
    if (id_of(current) == MachineState::Faulted) {
      return std::nullopt;
    }
    return StateVariant{FaultedState{MachineEvent::EmergencyStop}};
  }

  return std::visit(
    [event](const auto & concrete) -> std::optional<StateVariant> {
      using T = std::decay_t<decltype(concrete)>;
      if constexpr (std::is_same_v<T, StoppedState>) {
        if (event == MachineEvent::PowerOn) {
          return StateVariant{IdleState{}};
        }
      } else if constexpr (std::is_same_v<T, IdleState>) {
        if (event == MachineEvent::Start) {
          return StateVariant{RunningState{kCruiseDutyPercent}};
        }
      } else if constexpr (std::is_same_v<T, RunningState>) {
        if (event == MachineEvent::Stop) {
          return StateVariant{IdleState{}};
        }
      } else {
        if (event == MachineEvent::Reset) {
          return StateVariant{StoppedState{}};
        }
      }
      return std::nullopt;
    },
    current);
}

bool VariantStateMachine::handle(MachineEvent event)
{
  const std::optional<StateVariant> next = next_state(state_, event);
  if (!next.has_value()) {
    return false;
  }

  log_exit(log_, id_of(state_));
  state_ = *next;
  log_enter(log_, id_of(state_));
  return true;
}
