// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

/// ロボットの動作状態。
///
/// 遷移規則（この 1 枚が仕様のすべてです）:
///   Stopped --PowerOn--> Idle
///   Idle    --Start----> Running
///   Running --Stop-----> Idle
///   どの状態からでも EmergencyStop --> Faulted
///   Faulted --Reset----> Stopped
///   上に無い組み合わせは「無視」。状態は変わらず、入場/退場アクションも走りません。
enum class MachineState : std::uint8_t
{
  Stopped,
  Idle,
  Running,
  Faulted,
};

/// 状態機械への入力。
enum class MachineEvent : std::uint8_t
{
  PowerOn,
  Start,
  Stop,
  EmergencyStop,
  Reset,
};

inline const char * to_string(MachineState state)
{
  switch (state) {
    case MachineState::Stopped:
      return "Stopped";
    case MachineState::Idle:
      return "Idle";
    case MachineState::Running:
      return "Running";
    case MachineState::Faulted:
      return "Faulted";
  }
  return "?";
}

inline const char * to_string(MachineEvent event)
{
  switch (event) {
    case MachineEvent::PowerOn:
      return "PowerOn";
    case MachineEvent::Start:
      return "Start";
    case MachineEvent::Stop:
      return "Stop";
    case MachineEvent::EmergencyStop:
      return "EmergencyStop";
    case MachineEvent::Reset:
      return "Reset";
  }
  return "?";
}

/// 走行時の PWM デューティ。Running 状態が固有に持つデータです。
inline constexpr std::uint8_t kCruiseDutyPercent = 60;

/// 何が起きたかを順番に記録するだけの入れ物。
/// 実機ではこれがモータ出力や UART ログに化けます。
class TransitionLog
{
public:
  void record(std::string entry) { entries_.push_back(std::move(entry)); }

  const std::vector<std::string> & entries() const { return entries_; }

  void clear() { entries_.clear(); }

private:
  std::vector<std::string> entries_;
};

/// 退場アクション。3 つの実装すべてがこれと同じ並びを出すこと。
///
///   "exit:<状態名>" を記録する。
///   Running から出るときだけ、続けて "motor:stop" を記録する
///   （モータを止めてから状態を抜ける。実機では必須）。
inline void log_exit(TransitionLog & log, MachineState from)
{
  log.record(std::string("exit:") + to_string(from));
  if (from == MachineState::Running) {
    log.record("motor:stop");
  }
}

/// 入場アクション。
///
///   "enter:<状態名>" を記録する。
///   Faulted に入るときだけ、続けて "brake:engage" を記録する。
inline void log_enter(TransitionLog & log, MachineState to)
{
  log.record(std::string("enter:") + to_string(to));
  if (to == MachineState::Faulted) {
    log.record("brake:engage");
  }
}

// ---------------------------------------------------------------------------
// 手段1: enum + switch
// ---------------------------------------------------------------------------

/// 状態を「値」で持つ版。vtable もヒープ確保もありません。
/// ロボットの状態機械はたいていこれで足ります。
class EnumStateMachine
{
public:
  explicit EnumStateMachine(TransitionLog & log)
  : log_(log)
  {
  }

  MachineState state() const { return state_; }

  /// 遷移表を引くだけの純粋関数。副作用が無いので単体で試せます。
  /// 遷移しない入力に対しては current をそのまま返してください。
  static MachineState next_state(MachineState current, MachineEvent event);

  /// イベントを処理する。実際に状態が変わったら true。
  /// 変わるときだけ log_exit → log_enter の順に呼ぶこと。
  bool handle(MachineEvent event);

private:
  TransitionLog & log_;
  MachineState state_ = MachineState::Stopped;
};

// ---------------------------------------------------------------------------
// 手段2: State クラス（GoF 版）
// ---------------------------------------------------------------------------

/// GoF の State。
///
/// 【この章で最も重要な設計】
/// handle() は Context（状態機械）を一切受け取りません。**遷移先を戻り値で返すだけ**です。
/// Java 版のように handle() の中で context.setState(...) を呼ぶ形にすると、
/// C++ では「自分が破棄されたあとに自分のメンバ関数の続きが走る」ことがあります。
/// 戻り値で返す形なら、差し替えるのは呼び出し側なので構造的に起こりません。
///
/// もう 1 点。この State は**状態を 1 バイトも持ちません**。
/// だから 4 つの実体を static に 1 個ずつ置けば足ります（ヒープ確保ゼロ）。
class State
{
public:
  virtual ~State() = default;

  State() = default;
  State(const State &) = delete;
  State & operator=(const State &) = delete;

  virtual MachineState id() const = 0;

  /// 遷移先の State を返す。遷移しないなら this を返すこと。
  /// Context には触れません。
  virtual const State * handle(MachineEvent event) const = 0;

  /// 入場/退場アクション。既定は名前を記録するだけ。
  /// 状態固有の後始末がある派生（Running / Faulted）だけが override します。
  virtual void on_enter(TransitionLog & log) const
  {
    log.record(std::string("enter:") + to_string(id()));
  }

  virtual void on_exit(TransitionLog & log) const
  {
    log.record(std::string("exit:") + to_string(id()));
  }
};

/// 状態 id に対応する唯一の State 実体を返す。
/// 何度呼んでも同じアドレスが返ります（static 実体だから）。
const State * state_object(MachineState id);

/// State クラス版の Context。
class ClassStateMachine
{
public:
  explicit ClassStateMachine(TransitionLog & log)
  : log_(log)
  {
  }

  MachineState state() const { return current_->id(); }

  /// 今の状態オブジェクト。state_object() が返すものと同一のはずです。
  const State * current() const { return current_; }

  /// 遷移先を current_->handle() から**受け取ってから**差し替えること。
  bool handle(MachineEvent event);

private:
  TransitionLog & log_;
  const State * current_ = state_object(MachineState::Stopped);
};

// ---------------------------------------------------------------------------
// 手段3: std::variant + std::visit
// ---------------------------------------------------------------------------

/// 状態ごとのデータを持てて、しかもヒープ確保ゼロ・vtable ゼロ。
/// 種類がコンパイル時に固定できるならこれが最短です。
struct StoppedState
{
};

struct IdleState
{
};

struct RunningState
{
  std::uint8_t duty_percent = kCruiseDutyPercent;
};

struct FaultedState
{
  MachineEvent cause = MachineEvent::EmergencyStop;
};

using StateVariant = std::variant<StoppedState, IdleState, RunningState, FaultedState>;

/// variant の中身から状態 id を取り出す。std::visit を使ってください。
MachineState id_of(const StateVariant & state);

class VariantStateMachine
{
public:
  explicit VariantStateMachine(TransitionLog & log)
  : log_(log)
  {
  }

  MachineState state() const { return id_of(state_); }

  const StateVariant & raw() const { return state_; }

  /// 遷移先を返す純粋関数。遷移しないなら std::nullopt。
  /// ここでも「差し替えは呼び出し側」を守ります。
  static std::optional<StateVariant> next_state(const StateVariant & current, MachineEvent event);

  bool handle(MachineEvent event);

private:
  TransitionLog & log_;
  StateVariant state_ = StoppedState{};
};
