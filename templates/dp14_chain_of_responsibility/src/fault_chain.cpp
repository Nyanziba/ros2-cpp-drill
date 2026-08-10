// I AM NOT DONE
//
// 結城本 第14章 Chain of Responsibility を C++ で書きます。
// 難所は連鎖の「寿命」です。next を unique_ptr で所有させて、
// 先頭を捨てたら連鎖全体が消えることをテストで確かめます。

#include "drill/fault_chain.hpp"

#include <utility>

FaultHandler::FaultHandler(std::string name, std::vector<std::string> * destruction_log)
: name_(std::move(name)),
  destruction_log_(destruction_log)
{
}

FaultHandler::~FaultHandler()
{
  // TODO: destruction_log_ が非 nullptr なら、そこに name_ を push_back してください。
  //
  // これで「先頭を破棄すると連鎖全体が破棄される」ことをテストから観測できます。
  // 注意: 何もしないとテスト「先頭を破棄すると連鎖全体が破棄される」が落ちます。
  (void)destruction_log_;
}

FaultHandler & FaultHandler::set_next(std::unique_ptr<FaultHandler> next)
{
  // TODO: next をメンバ next_ に move して所有し、
  //       「今つないだ次のハンドラ自身」への参照を返してください。
  //
  // 注意: std::move した後の next は空です。参照を取るなら move する前に取るか、
  //       move したあとの next_ から取ってください。
  //       戻り値を *this にすると a.set_next(b).set_next(c) で c が b ではなく
  //       a の次に付いてしまいます。
  (void)next;
  return *this;
}

std::optional<FaultAction> FaultHandler::support(const Fault & fault) const
{
  // TODO: Chain of Responsibility の本体です。
  //   1. 自分で resolve(fault) してみる。値が返ったらそれをそのまま返す
  //   2. 返らなければ next_ があるか見る。あれば next_->support(fault) に丸投げする
  //   3. next_ が無ければ std::nullopt を返す（例外は投げません）
  (void)fault;
  (void)next_;
  return std::nullopt;
}

std::optional<FaultAction> FaultHandler::support_alone(const Fault & fault) const
{
  // TODO: 次には回さず、自分の resolve(fault) の結果だけを返してください。
  (void)fault;
  return std::nullopt;
}

std::optional<FaultAction> dispatch(
  FaultHandler * const * handlers, std::size_t count, const Fault & fault)
{
  // TODO: handlers[0] から handlers[count - 1] まで順に support_alone() を呼び、
  //       最初に値を返したものをそのまま返してください。
  //       誰も返さなければ std::nullopt。
  //
  // これが「連鎖を作らない」方式です。ポインタのつなぎ替えが無いので寿命が単純になります。
  (void)handlers;
  (void)count;
  (void)fault;
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
    // TODO: fault.kind が kLowVoltage で、かつ fault.magnitude が threshold_mv_ 未満なら
    //       FaultAction{name(), "reduce_duty"} を返してください。
    //       そうでなければ std::nullopt（= 次に回してもらう）。
    //
    // 注意: ここに next の話を書いてはいけません。たらい回しは support() の仕事です。
    (void)fault;
    (void)threshold_mv_;
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
    // TODO: kOverCurrent かつ fault.magnitude >= limit_ma_ なら
    //       FaultAction{name(), "cut_output"} を返してください。
    (void)fault;
    (void)limit_ma_;
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
    // TODO: kCommTimeout かつ fault.magnitude >= timeout_ms_ なら
    //       FaultAction{name(), "safe_stop"} を返してください。
    (void)fault;
    (void)timeout_ms_;
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
