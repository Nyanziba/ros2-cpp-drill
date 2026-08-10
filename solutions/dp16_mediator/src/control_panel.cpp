// 解答例
//
// 結城本 第16章 Mediator を C++ で書きます。
// Colleague（部品）どうしは直接やり取りせず、必ず Mediator（ControlPanel）を経由します。
//
// 所有の向きは 1 方向だけです。
//   ControlPanel --(unique_ptr で所有)--> PanelWidget
//   PanelWidget  --(生ポインタで指すだけ)--> PanelMediator
// 両方を shared_ptr にすると循環参照になり、どちらも解放されません。

#include "drill/control_panel.hpp"

#include <utility>

// ---------------------------------------------------------------------------
// LifetimeLog（テスト用の観測点）
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
  LifetimeLog::instance().record(name_);
}

const std::string & PanelWidget::name() const
{
  return name_;
}

void PanelWidget::set_mediator(PanelMediator * mediator)
{
  // 保存するだけ。所有はしません。
  mediator_ = mediator;
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
  // Mediator から呼ばれる入口。ここから notify_changed() を呼ぶと無限再帰します。
  is_enabled_ = enabled;
}

void PanelWidget::notify_changed()
{
  // 2 段階初期化なので「まだ結線されていない Colleague」がありえます。
  if (mediator_ == nullptr) {
    return;
  }
  mediator_->widget_changed(this);
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
  if (!is_enabled()) {
    return;
  }
  if (checked == is_checked_) {
    return;
  }
  is_checked_ = checked;
  notify_changed();
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
  if (!is_enabled()) {
    return false;
  }
  ++press_count_;
  notify_changed();
  return true;
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
  // 2 段階初期化。初期化子リストでは両方向を繋げません。
  // set_mediator() はポインタを保存するだけなので、構築中の this を渡しても安全です。
  emergency_stop_->set_mediator(this);
  auto_mode_->set_mediator(this);
  manual_forward_->set_mediator(this);
  manual_stop_->set_mediator(this);

  update_enabled_states();
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
  change_log_.push_back(widget->name());
  update_enabled_states();
}

void ControlPanel::update_enabled_states()
{
  const bool is_emergency = emergency_stop_->is_checked();
  const bool is_manual_allowed = !is_emergency && !auto_mode_->is_checked();

  // 非常停止トグルだけは常に有効。無効にすると解除できなくなります。
  emergency_stop_->set_enabled(true);
  auto_mode_->set_enabled(!is_emergency);
  manual_forward_->set_enabled(is_manual_allowed);
  manual_stop_->set_enabled(is_manual_allowed);
}
