// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include "drill_i18n.hpp"

#include <string>
#include <type_traits>
#include <vector>

#include "drill/state_machine.hpp"

namespace
{

using Events = std::vector<MachineEvent>;
using Log = std::vector<std::string>;
using States = std::vector<MachineState>;

/// 全 4 状態・全 5 イベント。
constexpr MachineState kAllStates[] = {
  MachineState::Stopped, MachineState::Idle, MachineState::Running, MachineState::Faulted};

constexpr MachineEvent kAllEvents[] = {
  MachineEvent::PowerOn, MachineEvent::Start, MachineEvent::Stop, MachineEvent::EmergencyStop,
  MachineEvent::Reset};

/// 仕様そのもの。テストが参照する唯一の正解表。
MachineState expected_next(MachineState current, MachineEvent event)
{
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
      return (event == MachineEvent::Reset) ? MachineState::Stopped : current;
  }
  return current;
}

/// Stopped から始めて、目的の状態まで正規のイベントで進める。
Events path_to(MachineState target)
{
  switch (target) {
    case MachineState::Stopped:
      return {};
    case MachineState::Idle:
      return {MachineEvent::PowerOn};
    case MachineState::Running:
      return {MachineEvent::PowerOn, MachineEvent::Start};
    case MachineState::Faulted:
      return {MachineEvent::EmergencyStop};
  }
  return {};
}

/// 3 実装の共通の呼び出し方。どれも state() / handle() を同じ形で持っています。
template <typename Machine>
struct RunResult
{
  States states;
  Log log;
  std::vector<bool> changed;
};

template <typename Machine>
RunResult<Machine> execute(const Events & events)
{
  TransitionLog log;
  Machine machine{log};

  RunResult<Machine> result;
  for (const MachineEvent event : events) {
    result.changed.push_back(machine.handle(event));
    result.states.push_back(machine.state());
  }
  result.log = log.entries();
  return result;
}

/// 実機を一通り動かしたときのイベント列（無視されるはずの入力も混ぜてある）。
const Events kScenario = {
  MachineEvent::Start,          // Stopped で Start は無効 → 無視
  MachineEvent::PowerOn,        // Stopped -> Idle
  MachineEvent::PowerOn,        // Idle で PowerOn は無効 → 無視
  MachineEvent::Stop,           // Idle で Stop は無効 → 無視
  MachineEvent::Start,          // Idle -> Running
  MachineEvent::Stop,           // Running -> Idle
  MachineEvent::Start,          // Idle -> Running
  MachineEvent::EmergencyStop,  // Running -> Faulted
  MachineEvent::EmergencyStop,  // Faulted で再度の非常停止 → 無視
  MachineEvent::PowerOn,        // Faulted で PowerOn は無効 → 無視
  MachineEvent::Start,          // Faulted で Start は無効 → 無視
  MachineEvent::Reset,          // Faulted -> Stopped
  MachineEvent::PowerOn,        // Stopped -> Idle
};

}  // namespace

// ---------------------------------------------------------------------------
// 遷移規則
// ---------------------------------------------------------------------------

TEST(StateTest, EnumTransitionTableMatchesSpec)
{
  for (const MachineState current : kAllStates) {
    for (const MachineEvent event : kAllEvents) {
      EXPECT_EQ(EnumStateMachine::next_state(current, event), expected_next(current, event))
        << drill::localized("状態 ", "state ") << to_string(current) << drill::localized(" でイベント ", " with event ") << to_string(event);
    }
  }
}

TEST(StateTest, InvalidInputIsIgnoredAndStateStays)
{
  for (const MachineState start : kAllStates) {
    for (const MachineEvent event : kAllEvents) {
      TransitionLog log;
      EnumStateMachine machine{log};
      for (const MachineEvent step : path_to(start)) {
        machine.handle(step);
      }
      ASSERT_EQ(machine.state(), start);
      log.clear();

      const bool changed = machine.handle(event);
      const MachineState expected = expected_next(start, event);

      EXPECT_EQ(machine.state(), expected);
      EXPECT_EQ(changed, expected != start)
        << drill::localized("handle() の戻り値は「実際に遷移したか」です: ", "handle() must return whether a transition really happened: ") << to_string(start) << " / "
        << to_string(event);
      if (!changed) {
        EXPECT_TRUE(log.entries().empty())
          << drill::localized("遷移しなかったのに入場/退場アクションが走っています", "Enter/exit actions ran although no transition happened");
      }
    }
  }
}

TEST(StateTest, FaultedCanOnlyBeLeftByManualReset)
{
  TransitionLog log;
  EnumStateMachine machine{log};
  machine.handle(MachineEvent::EmergencyStop);
  ASSERT_EQ(machine.state(), MachineState::Faulted);

  for (const MachineEvent event : kAllEvents) {
    if (event == MachineEvent::Reset) {
      continue;
    }
    EXPECT_FALSE(machine.handle(event)) << to_string(event) << drill::localized(" で Faulted を抜けています", " left Faulted");
    EXPECT_EQ(machine.state(), MachineState::Faulted);
  }

  EXPECT_TRUE(machine.handle(MachineEvent::Reset));
  EXPECT_EQ(machine.state(), MachineState::Stopped);
}

TEST(StateTest, EmergencyStopWorksFromAnyState)
{
  for (const MachineState start : kAllStates) {
    if (start == MachineState::Faulted) {
      continue;
    }
    TransitionLog log;
    EnumStateMachine machine{log};
    for (const MachineEvent step : path_to(start)) {
      machine.handle(step);
    }
    ASSERT_EQ(machine.state(), start);

    EXPECT_TRUE(machine.handle(MachineEvent::EmergencyStop));
    EXPECT_EQ(machine.state(), MachineState::Faulted);
  }
}

// ---------------------------------------------------------------------------
// 入場・退場アクション
// ---------------------------------------------------------------------------

TEST(StateTest, ExitComesBeforeEnter)
{
  TransitionLog log;
  EnumStateMachine machine{log};
  machine.handle(MachineEvent::PowerOn);

  const Log expected = {"exit:Stopped", "enter:Idle"};
  EXPECT_EQ(log.entries(), expected);
}

TEST(StateTest, LeavingRunningStopsMotorBeforeLeavingState)
{
  TransitionLog log;
  EnumStateMachine machine{log};
  machine.handle(MachineEvent::PowerOn);
  machine.handle(MachineEvent::Start);
  ASSERT_EQ(machine.state(), MachineState::Running);
  log.clear();

  machine.handle(MachineEvent::EmergencyStop);

  const Log expected = {"exit:Running", "motor:stop", "enter:Faulted", "brake:engage"};
  EXPECT_EQ(log.entries(), expected);
}

TEST(StateTest, StateClassVersionHasSameEnterExitActions)
{
  TransitionLog log;
  ClassStateMachine machine{log};
  machine.handle(MachineEvent::PowerOn);
  machine.handle(MachineEvent::Start);
  ASSERT_EQ(machine.state(), MachineState::Running);
  log.clear();

  machine.handle(MachineEvent::EmergencyStop);

  const Log expected = {"exit:Running", "motor:stop", "enter:Faulted", "brake:engage"};
  EXPECT_EQ(log.entries(), expected)
    << drill::localized("RunningStateObject::on_exit / FaultedStateObject::on_enter を override してください", "Override RunningStateObject::on_exit and FaultedStateObject::on_enter");
}

// ---------------------------------------------------------------------------
// 遷移中に状態を差し替えても壊れないこと
// ---------------------------------------------------------------------------

TEST(StateTest, StateObjectIsUniqueAndKeepsAddress)
{
  // 自分自身を差し替える手段がそもそも無い、ということを型でも確認します。
  static_assert(
    std::is_same_v<decltype(&State::handle), const State * (State::*)(MachineEvent) const>,
    "State::handle は Context を受け取らず、遷移先を戻り値で返すだけにしてください / State::handle must not take a Context. It should only return the next state");

  for (const MachineState id : kAllStates) {
    const State * const first = state_object(id);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first, state_object(id)) << drill::localized("呼ぶたびに別の実体を作っています", "A new object is created on every call");
    EXPECT_EQ(first->id(), id);
  }
}

TEST(StateTest, OriginalStateObjectStaysAliveAfterTransition)
{
  TransitionLog log;
  ClassStateMachine machine{log};
  EXPECT_EQ(machine.current(), state_object(MachineState::Stopped));

  machine.handle(MachineEvent::PowerOn);
  EXPECT_EQ(machine.current(), state_object(MachineState::Idle));

  machine.handle(MachineEvent::Start);
  ASSERT_EQ(machine.state(), MachineState::Running);

  const State * const running = machine.current();
  ASSERT_EQ(running, state_object(MachineState::Running));

  machine.handle(MachineEvent::EmergencyStop);
  EXPECT_EQ(machine.state(), MachineState::Faulted);

  // 遷移元のオブジェクトが破棄されていたら、ここは未定義動作になります。
  // static 実体 + 「差し替えるのは呼び出し側」なら安全に触れます。
  EXPECT_EQ(running->id(), MachineState::Running);
  EXPECT_EQ(running, state_object(MachineState::Running));

  // 遷移元の handle() を今もう一度呼んでも矛盾しない（状態を持たないから）。
  EXPECT_EQ(running->handle(MachineEvent::Stop), state_object(MachineState::Idle));
}

// ---------------------------------------------------------------------------
// std::variant 版
// ---------------------------------------------------------------------------

TEST(StateTest, VariantVersionIsNonPolymorphicAndUsesNoHeap)
{
  static_assert(!std::is_polymorphic_v<StoppedState>, "variant の状態に vtable は要りません / The states of the variant need no vtable");
  static_assert(!std::is_polymorphic_v<IdleState>, "variant の状態に vtable は要りません / The states of the variant need no vtable");
  static_assert(!std::is_polymorphic_v<RunningState>, "variant の状態に vtable は要りません / The states of the variant need no vtable");
  static_assert(!std::is_polymorphic_v<FaultedState>, "variant の状態に vtable は要りません / The states of the variant need no vtable");
  static_assert(!std::is_polymorphic_v<VariantStateMachine>, "Context にも vtable は要りません / The Context needs no vtable either");
  static_assert(std::is_trivially_destructible_v<StateVariant>, "デストラクタも要りません / No destructor is needed either");
  static_assert(sizeof(StateVariant) <= 8, "状態はすべて直和型の中に収まります / All states fit inside the sum type");

  // 型だけでなく、id_of() が std::visit で正しく振り分けられているかも見ます。
  EXPECT_EQ(id_of(StateVariant{IdleState{}}), MachineState::Idle);
  EXPECT_EQ(id_of(StateVariant{RunningState{}}), MachineState::Running);
}

TEST(StateTest, VariantVersionHoldsPerStateData)
{
  TransitionLog log;
  VariantStateMachine machine{log};
  machine.handle(MachineEvent::PowerOn);
  machine.handle(MachineEvent::Start);
  ASSERT_EQ(machine.state(), MachineState::Running);

  ASSERT_TRUE(std::holds_alternative<RunningState>(machine.raw()));
  EXPECT_EQ(std::get<RunningState>(machine.raw()).duty_percent, kCruiseDutyPercent);

  machine.handle(MachineEvent::EmergencyStop);
  ASSERT_TRUE(std::holds_alternative<FaultedState>(machine.raw()));
  EXPECT_EQ(std::get<FaultedState>(machine.raw()).cause, MachineEvent::EmergencyStop);
}

TEST(StateTest, VariantNextStateIsNulloptWithoutTransition)
{
  const StateVariant faulted{FaultedState{MachineEvent::EmergencyStop}};
  EXPECT_FALSE(VariantStateMachine::next_state(faulted, MachineEvent::Start).has_value());
  EXPECT_FALSE(VariantStateMachine::next_state(faulted, MachineEvent::EmergencyStop).has_value());
  EXPECT_TRUE(VariantStateMachine::next_state(faulted, MachineEvent::Reset).has_value());

  EXPECT_EQ(id_of(StateVariant{StoppedState{}}), MachineState::Stopped);
  EXPECT_EQ(id_of(StateVariant{IdleState{}}), MachineState::Idle);
  EXPECT_EQ(id_of(StateVariant{RunningState{}}), MachineState::Running);
  EXPECT_EQ(id_of(faulted), MachineState::Faulted);
}

// ---------------------------------------------------------------------------
// 3 実装の一致
// ---------------------------------------------------------------------------

TEST(StateTest, ThreeImplementationsGiveSameTransitionsAndLog)
{
  const RunResult<EnumStateMachine> by_enum = execute<EnumStateMachine>(kScenario);
  const RunResult<ClassStateMachine> by_class = execute<ClassStateMachine>(kScenario);
  const RunResult<VariantStateMachine> by_variant = execute<VariantStateMachine>(kScenario);

  EXPECT_EQ(by_enum.states, by_class.states) << drill::localized("enum 版と State クラス版で遷移が違います", "The enum version and the State class version transition differently");
  EXPECT_EQ(by_enum.states, by_variant.states) << drill::localized("enum 版と variant 版で遷移が違います", "The enum version and the variant version transition differently");

  EXPECT_EQ(by_enum.changed, by_class.changed);
  EXPECT_EQ(by_enum.changed, by_variant.changed);

  EXPECT_EQ(by_enum.log, by_class.log) << drill::localized("入退場アクションの並びが違います", "The order of enter/exit actions is different");
  EXPECT_EQ(by_enum.log, by_variant.log) << drill::localized("入退場アクションの並びが違います", "The order of enter/exit actions is different");

  // 期待値そのものも固定しておきます（3 つとも同じように間違えている場合の保険）。
  const States expected_states = {
    MachineState::Stopped, MachineState::Idle,    MachineState::Idle,
    MachineState::Idle,    MachineState::Running, MachineState::Idle,
    MachineState::Running, MachineState::Faulted, MachineState::Faulted,
    MachineState::Faulted, MachineState::Faulted, MachineState::Stopped,
    MachineState::Idle};
  EXPECT_EQ(by_enum.states, expected_states);

  const Log expected_log = {
    "exit:Stopped",  "enter:Idle",   "exit:Idle",     "enter:Running", "exit:Running",
    "motor:stop",    "enter:Idle",   "exit:Idle",     "enter:Running", "exit:Running",
    "motor:stop",    "enter:Faulted", "brake:engage", "exit:Faulted",  "enter:Stopped",
    "exit:Stopped",  "enter:Idle"};
  EXPECT_EQ(by_enum.log, expected_log);
}
