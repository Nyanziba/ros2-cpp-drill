// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/// 異常の種類。ロボットの電源系・通信系で実際に見るものだけに絞っています。
enum class FaultKind
{
  kLowVoltage,    ///< 電圧低下。magnitude はバッテリ電圧 [mV]
  kOverCurrent,   ///< 過電流。magnitude は電流 [mA]
  kCommTimeout,   ///< 通信断。magnitude は最後の受信からの経過 [ms]
  kEncoderSlip,   ///< エンコーダの飛び。この課題では誰も処理しません
};

/// 検出した異常 1 件。
struct Fault
{
  FaultKind kind = FaultKind::kLowVoltage;
  int magnitude = 0;
};

/// 「誰が」「何をしたか」。処理できた場合だけ返ります。
struct FaultAction
{
  std::string handler_name;
  std::string action;
};

inline bool operator==(const FaultAction & lhs, const FaultAction & rhs)
{
  return lhs.handler_name == rhs.handler_name && lhs.action == rhs.action;
}

/// Chain of Responsibility の Handler（結城本 第14章の Support に対応）。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須。
///   - next は生ポインタではなく std::unique_ptr で「所有」します。
///     これで連鎖全体の寿命が先頭に紐付き、循環も型として作れなくなります。
///   - 誰も処理しなかったときに例外を投げません（マイコンで -fno-exceptions のため）。
///     std::optional<FaultAction> の nullopt で表現します。
///   - support() は public 非仮想 / resolve() は protected 純粋仮想（NVI）。
///     「次に回す」という共通処理を派生に書かせないためです。
class FaultHandler
{
public:
  /// destruction_log が非 nullptr なら、デストラクタで自分の名前を push_back します。
  /// 連鎖の所有権をテストから観測するための仕掛けです。
  explicit FaultHandler(std::string name, std::vector<std::string> * destruction_log = nullptr);

  virtual ~FaultHandler();

  // 連鎖のノードをコピーする意味は無い（next_ が unique_ptr なので不可能でもある）。
  FaultHandler(const FaultHandler &) = delete;
  FaultHandler & operator=(const FaultHandler &) = delete;

  const std::string & name() const { return name_; }

  /// 次のハンドラを所有する。
  /// 戻り値は「今つないだ次のハンドラ自身」への参照。こう書けます:
  ///   head.set_next(std::move(b)).set_next(std::move(c));
  FaultHandler & set_next(std::unique_ptr<FaultHandler> next);

  /// 連鎖の先頭からたらい回しする。誰も処理しなければ std::nullopt。
  /// 例外は投げません。
  std::optional<FaultAction> support(const Fault & fault) const;

  /// 自分ひとりに聞く（次には回さない）。
  /// vector<unique_ptr<Handler>> を順に回す「連鎖を作らない」方式で使います。
  std::optional<FaultAction> support_alone(const Fault & fault) const;

protected:
  /// 自分が処理できるなら FaultAction を、できないなら std::nullopt を返す。
  /// 「次に回す」処理をここに書いてはいけません。support() の仕事です。
  virtual std::optional<FaultAction> resolve(const Fault & fault) const = 0;

private:
  std::string name_;
  std::vector<std::string> * destruction_log_;
  std::unique_ptr<FaultHandler> next_;
};

/// 連鎖を作らない方式。並んだハンドラを先頭から順に support_alone() で聞きます。
/// handlers は count 個の非 nullptr な FaultHandler * が並んだ配列。
/// 所有権は一切持ちません（呼び出し側が生かしておくこと）。
std::optional<FaultAction> dispatch(
  FaultHandler * const * handlers, std::size_t count, const Fault & fault);

/// 電圧低下ハンドラ。kLowVoltage かつ magnitude < threshold_mv のとき
/// action = "reduce_duty" を返します。それ以外は次に回します。
std::unique_ptr<FaultHandler> make_low_voltage_handler(
  std::string name, int threshold_mv, std::vector<std::string> * destruction_log = nullptr);

/// 過電流ハンドラ。kOverCurrent かつ magnitude >= limit_ma のとき
/// action = "cut_output" を返します。それ以外は次に回します。
std::unique_ptr<FaultHandler> make_over_current_handler(
  std::string name, int limit_ma, std::vector<std::string> * destruction_log = nullptr);

/// 通信断ハンドラ。kCommTimeout かつ magnitude >= timeout_ms のとき
/// action = "safe_stop" を返します。それ以外は次に回します。
std::unique_ptr<FaultHandler> make_comm_timeout_handler(
  std::string name, int timeout_ms, std::vector<std::string> * destruction_log = nullptr);
