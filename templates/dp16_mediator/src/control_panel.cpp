// I AM NOT DONE
//
// 結城本 第16章 Mediator を C++ で書きます。
// Colleague（部品）どうしは直接やり取りせず、必ず Mediator（ControlPanel）を経由します。
//
// この章の C++ 固有の急所は「相互参照」です。
// Mediator は Colleague を所有し（unique_ptr）、Colleague は Mediator を指すだけ（生ポインタ）。
// 両方を shared_ptr にすると循環参照になり、どちらも解放されません。

#include "drill/control_panel.hpp"

#include <utility>

// ---------------------------------------------------------------------------
// LifetimeLog（実装済み。テスト用の観測点）
// ---------------------------------------------------------------------------

LifetimeLog & LifetimeLog::instance()
{
  static LifetimeLog log;
  return log;
}

void LifetimeLog::record(std::string name)
{
  entries_.push_back(std::move(name));
}

const std::vector<std::string> & LifetimeLog::entries() const
{
  return entries_;
}

void LifetimeLog::clear()
{
  entries_.clear();
}

// ---------------------------------------------------------------------------
// PanelWidget（Colleague の共通部分）
// ---------------------------------------------------------------------------

PanelWidget::PanelWidget(std::string name)
: name_(std::move(name))
{
}

PanelWidget::~PanelWidget()
{
  // 「本当に解放されたか」をここで残します。循環参照が起きているとこの行は実行されません。
  LifetimeLog::instance().record(name_);
}

const std::string & PanelWidget::name() const
{
  return name_;
}

void PanelWidget::set_mediator(PanelMediator * mediator)
{
  // TODO: mediator_ に mediator を保存してください。
  //
  // 所有はしません。std::shared_ptr でも std::unique_ptr でもなく生ポインタです。
  // Mediator 側が Colleague を unique_ptr で所有しているので、
  // ここで所有を持ち返すと相互所有（循環参照）になります。
  (void)mediator;
}

bool PanelWidget::has_mediator() const
{
  return mediator_ != nullptr;
}

bool PanelWidget::is_enabled() const
{
  return is_enabled_;
}

void PanelWidget::set_enabled(bool enabled)
{
  is_enabled_ = enabled;
}

void PanelWidget::notify_changed()
{
  // TODO: Mediator が設定されていれば widget_changed(this) を呼んでください。
  //
  // 未設定（nullptr）のときは何もしません。2 段階初期化なので
  // 「まだ Mediator が繋がっていない Colleague」が一瞬だけ存在します。
  // そこで落ちないようにするのがこの nullptr チェックです。
}

// ---------------------------------------------------------------------------
// ToggleWidget
// ---------------------------------------------------------------------------

ToggleWidget::ToggleWidget(std::string name)
: PanelWidget(std::move(name))
{
}

bool ToggleWidget::is_checked() const
{
  return is_checked_;
}

void ToggleWidget::set_checked(bool checked)
{
  // TODO: 次の順で実装してください。
  //   1. is_enabled() が false なら、何もせずに return（無効な部品は操作できない）
  //   2. checked が今の is_checked_ と同じなら、何もせずに return
  //      （変化していないのに報告すると、Mediator が無駄に動きます）
  //   3. is_checked_ を更新する
  //   4. notify_changed() で Mediator に報告する
  //
  // 注意: ここで他の部品（手動ボタンなど）を直接触ってはいけません。
  //       それをやると Colleague どうしが直接つながり、Mediator の意味が消えます。
  (void)checked;
}

// ---------------------------------------------------------------------------
// ButtonWidget
// ---------------------------------------------------------------------------

ButtonWidget::ButtonWidget(std::string name)
: PanelWidget(std::move(name))
{
}

int ButtonWidget::press_count() const
{
  return press_count_;
}

bool ButtonWidget::press()
{
  // TODO: 次の順で実装してください。
  //   1. is_enabled() が false なら false を返す（押下回数も増やさない）
  //   2. press_count_ を 1 増やす
  //   3. notify_changed() で Mediator に報告する
  //   4. true を返す
  return false;
}

// ---------------------------------------------------------------------------
// ControlPanel（ConcreteMediator）
// ---------------------------------------------------------------------------

ControlPanel::ControlPanel()
: emergency_stop_(std::make_unique<ToggleWidget>("emergency_stop")),
  auto_mode_(std::make_unique<ToggleWidget>("auto_mode")),
  manual_forward_(std::make_unique<ButtonWidget>("manual_forward")),
  manual_stop_(std::make_unique<ButtonWidget>("manual_stop"))
{
  // ここまでで Colleague は「生成済み・Mediator 未設定」の状態です。
  // コンストラクタの初期化子リストで両方向を繋ぐことはできません。
  // Colleague を作るには Mediator が要り、Mediator を渡すには Colleague が要るからです。
  //
  // TODO: 4 つの Colleague すべてに set_mediator(this) を呼んで結線し、
  //       そのあと update_enabled_states() で初期状態を整えてください。
  //
  // 注意: this を配る時点で ControlPanel はまだ構築中です。
  //       ここから Colleague の仮想関数を呼ぶのは危険ですが、
  //       set_mediator() はポインタを保存するだけなので安全です。
}

ControlPanel::~ControlPanel()
{
  LifetimeLog::instance().record("ControlPanel");
  // このあとメンバの unique_ptr が破棄され、4 つの Colleague も解放されます。
}

ToggleWidget & ControlPanel::emergency_stop()
{
  return *emergency_stop_;
}

ToggleWidget & ControlPanel::auto_mode()
{
  return *auto_mode_;
}

ButtonWidget & ControlPanel::manual_forward()
{
  return *manual_forward_;
}

ButtonWidget & ControlPanel::manual_stop()
{
  return *manual_stop_;
}

const std::vector<std::string> & ControlPanel::change_log() const
{
  return change_log_;
}

void ControlPanel::clear_change_log()
{
  change_log_.clear();
}

void ControlPanel::widget_changed(PanelWidget * widget)
{
  // TODO: 次の 2 つをしてください。
  //   1. change_log_ に widget->name() を追加する（誰から来たかの記録）
  //   2. update_enabled_states() を呼ぶ
  //
  // 結城本の Java 版は colleagueChanged() の中で if (colleague == checkGuest) ... と
  // 送り主で分岐しますが、この課題では「今の状態から全部決め直す」方式にします。
  // 分岐が増えても Mediator が肥大化しにくく、状態の食い違いも起きません。
  (void)widget;
}

void ControlPanel::update_enabled_states()
{
  // TODO: 今のトグル状態から、4 つの部品の有効・無効を決め直してください。
  //
  //   const bool is_emergency = emergency_stop_->is_checked();
  //   非常停止トグル : 常に有効（無効にすると解除できなくなる）
  //   自動モードトグル : 非常停止中は無効
  //   手動ボタン 2 つ : 非常停止中でなく、かつ自動モードがオフのときだけ有効
  //
  // 注意: 有効・無効の変更は set_enabled() で行います。
  //       set_enabled() は notify_changed() を呼びません。呼ぶと
  //       widget_changed() → update_enabled_states() → widget_changed() と無限再帰します。
}
