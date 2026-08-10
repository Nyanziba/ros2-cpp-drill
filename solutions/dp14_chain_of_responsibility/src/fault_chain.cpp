// 解答例。
//
// 結城本 第14章 Chain of Responsibility。
// 難所は連鎖の寿命なので、next を unique_ptr で所有させています。

#include "drill/fault_chain.hpp"

#include <utility>

FaultHandler::FaultHandler(std::string name, std::vector<std::string> * destruction_log)
: name_(std::move(name)),
  destruction_log_(destruction_log)
{
}

FaultHandler::~FaultHandler()
{
  if (destruction_log_ != nullptr) {
    destruction_log_->push_back(name_);
  }
  // next_ はこのデストラクタ本体の「あと」に破棄されます。
  // だからログは 先頭 → 次 → その次 の順に並びます。
}

FaultHandler & FaultHandler::set_next(std::unique_ptr<FaultHandler> next)
{
  // 古い next_ はここで解放されます（所有権を持っているので当然）。
  next_ = std::move(next);
  return *next_;
}

std::optional<FaultAction> FaultHandler::support(const Fault & fault) const
{
  if (std::optional<FaultAction> mine = resolve(fault)) {
    return mine;
  }
  if (next_ != nullptr) {
    return next_->support(fault);
  }
  // 誰も処理しなかった。throw しないのがこの設計の約束です。
  return std::nullopt;
}

std::optional<FaultAction> FaultHandler::support_alone(const Fault & fault) const
{
  return resolve(fault);
}

std::optional<FaultAction> dispatch(
  FaultHandler * const * handlers, std::size_t count, const Fault & fault)
{
  for (std::size_t i = 0; i < count; ++i) {
    if (std::optional<FaultAction> action = handlers[i]->support_alone(fault)) {
      return action;
    }
  }
  return std::nullopt;
}

namespace
{

/// 電圧低下ハンドラ。
class LowVoltageHandler : public FaultHandler
{
public:
  LowVoltageHandler(std::string name, int threshold_mv, std::vector<std::string> * destruction_log)
  : FaultHandler(std::move(name), destruction_log),
    threshold_mv_(threshold_mv)
  {
  }

protected:
  std::optional<FaultAction> resolve(const Fault & fault) const override
  {
    if (fault.kind == FaultKind::kLowVoltage && fault.magnitude < threshold_mv_) {
      return FaultAction{name(), "reduce_duty"};
    }
    return std::nullopt;
  }

private:
  int threshold_mv_;
};

/// 過電流ハンドラ。
class OverCurrentHandler : public FaultHandler
{
public:
  OverCurrentHandler(std::string name, int limit_ma, std::vector<std::string> * destruction_log)
  : FaultHandler(std::move(name), destruction_log),
    limit_ma_(limit_ma)
  {
  }

protected:
  std::optional<FaultAction> resolve(const Fault & fault) const override
  {
    if (fault.kind == FaultKind::kOverCurrent && fault.magnitude >= limit_ma_) {
      return FaultAction{name(), "cut_output"};
    }
    return std::nullopt;
  }

private:
  int limit_ma_;
};

/// 通信断ハンドラ。
class CommTimeoutHandler : public FaultHandler
{
public:
  CommTimeoutHandler(std::string name, int timeout_ms, std::vector<std::string> * destruction_log)
  : FaultHandler(std::move(name), destruction_log),
    timeout_ms_(timeout_ms)
  {
  }

protected:
  std::optional<FaultAction> resolve(const Fault & fault) const override
  {
    if (fault.kind == FaultKind::kCommTimeout && fault.magnitude >= timeout_ms_) {
      return FaultAction{name(), "safe_stop"};
    }
    return std::nullopt;
  }

private:
  int timeout_ms_;
};

}  // namespace

std::unique_ptr<FaultHandler> make_low_voltage_handler(
  std::string name, int threshold_mv, std::vector<std::string> * destruction_log)
{
  return std::make_unique<LowVoltageHandler>(std::move(name), threshold_mv, destruction_log);
}

std::unique_ptr<FaultHandler> make_over_current_handler(
  std::string name, int limit_ma, std::vector<std::string> * destruction_log)
{
  return std::make_unique<OverCurrentHandler>(std::move(name), limit_ma, destruction_log);
}

std::unique_ptr<FaultHandler> make_comm_timeout_handler(
  std::string name, int timeout_ms, std::vector<std::string> * destruction_log)
{
  return std::make_unique<CommTimeoutHandler>(std::move(name), timeout_ms, destruction_log);
}
