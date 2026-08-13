// I AM NOT DONE
//
// 結城本 第21章 Proxy を C++ で書きます。
//
// 実装するのは 5 つです。
//   1. CalibrationProxy::operator->()   — 遅延生成する Virtual Proxy の本体
//   2. RegisterAccess::RegisterAccess() — 一時オブジェクトの入口（"enter" を記録）
//   3. RegisterAccess::~RegisterAccess()— 一時オブジェクトの出口（"leave" を記録）
//   4. RegisterAccess::operator->()     — ここでポインタを返して連鎖を止める
//   5. SafeRegisterProxy::read / write     — 範囲検査と記録
//
// SafeRegisterProxy::operator-> は実装済みです（返し方に C++17 の話があるので）。
//
// CalibrationTable と RegisterFile（本体側）は実装済みです。読むだけでよいです。

#include "drill/calibration_proxy.hpp"

#include <utility>

namespace drill
{

namespace
{

std::size_t & load_counter()
{
  static std::size_t count = 0;
  return count;
}

}  // namespace

// ---------------------------------------------------------------------------
// 本体側（実装済み）
// ---------------------------------------------------------------------------

CalibrationTable::CalibrationTable(std::string source)
: source_(std::move(source))
{
  for (std::size_t i = 0; i < kEntryCount; ++i) {
    entries_[i] = static_cast<double>(source_.size()) + 0.25 * static_cast<double>(i);
  }
  ++load_counter();
}

double CalibrationTable::entry(std::size_t index) const
{
  if (index >= kEntryCount) {
    return 0.0;
  }
  return entries_[index];
}

std::size_t CalibrationTable::load_count()
{
  return load_counter();
}

void CalibrationTable::reset_load_count()
{
  load_counter() = 0;
}

std::uint16_t RegisterFile::read_raw(std::size_t index) const
{
  ++hardware_access_count_;
  if (index >= kRegisterCount) {
    return 0;
  }
  return registers_[index];
}

void RegisterFile::write_raw(std::size_t index, std::uint16_t value)
{
  ++hardware_access_count_;
  if (index >= kRegisterCount) {
    return;
  }
  registers_[index] = value;
}

void RegisterFile::reset_counts()
{
  hardware_access_count_ = 0;
}

// ---------------------------------------------------------------------------
// ここから課題
// ---------------------------------------------------------------------------

CalibrationProxy::CalibrationProxy(std::string source)
: source_(std::move(source))
{
  // ここで本体を作ってはいけません。作った時点で Proxy の意味がありません。
}

const CalibrationTable * CalibrationProxy::operator->() const
{
  // TODO: 遅延生成を実装してください。
  //
  //   1. real_ が空なら std::make_unique<CalibrationTable>(source_) で作る
  //   2. real_.get() を返す
  //
  // 【なぜ const メンバ関数なのに書き換えられるのか】
  // real_ が mutable だからです。ヘッダの宣言を見てください。
  // mutable を外すと、この関数はコンパイルできません。
  //   error: no viable overloaded '='
  //   note: 'this' argument has type 'const std::unique_ptr<CalibrationTable>',
  //         but method is not marked const
  //
  // 【なぜ operator-> を書くと proxy->entry(3) が動くのか】
  // proxy->entry(3) は proxy.operator->()->entry(3) に展開されます。
  // operator-> は「ポインタが返るまで」繰り返し呼ばれる規則です。
  // ここは CalibrationTable * を返すので、1 回で止まります。
  return nullptr;
}

RegisterAccess::RegisterAccess(SafeRegisterProxy & owner)
: owner_(owner)
{
  // TODO: owner_.record("enter"); を呼んでください。
  //
  // この一時オブジェクトは式の終わりまで生きます。だから
  //   proxy->write_raw(1, 0x00ff);
  // と書くと、必ず enter → 本体の呼び出し → leave の順になります。
  // 実務では、ここで std::mutex をロックします。
  (void)owner_;
}

RegisterAccess::~RegisterAccess()
{
  // TODO: owner_.record("leave"); を呼んでください。
  // 実務では、ここで std::mutex を解放します。
}

RegisterFile * RegisterAccess::operator->() const
{
  // TODO: 包んでいる SafeRegisterProxy が持つ RegisterFile へのポインタを返してください。
  //
  //   return &owner_.file_;
  //
  // owner_.file() は const RegisterFile & なので、ここからは書き込めません。
  // RegisterAccess はヘッダで SafeRegisterProxy の friend になっているので、
  // private メンバ file_ に直接届きます。const_cast は要りません。
  //
  // 【ここでポインタを返すことが本質です】
  // ポインタを返した時点で operator-> の連鎖が止まり、
  // 続く ->read_raw(0) が RegisterFile のメンバ呼び出しになります。
  // もしここで別の Proxy を返したら、連鎖はさらに 1 段深くなります。
  return nullptr;
}

SafeRegisterProxy::SafeRegisterProxy(RegisterFile & file)
: file_(file)
{
}

RegisterAccess SafeRegisterProxy::operator->()
{
  // ここは実装済みです。読んでください。
  //
  // RegisterAccess はコピーもムーブもできません。それでも値で返せるのは、
  // C++17 の「保証されたコピー省略」で、prvalue が呼び出し側の場所に
  // 直接構築されるからです。C++11 ではコンパイルが通りません。
  return RegisterAccess{*this};
}

std::optional<std::uint16_t> SafeRegisterProxy::read(std::size_t index)
{
  // TODO: 範囲検査つきの読み出しを実装してください。
  //
  //   - index >= RegisterFile::kRegisterCount なら
  //       record("reject:read:" + std::to_string(index));
  //       ++rejected_;
  //       本体には触らずに std::nullopt を返す
  //   - 範囲内なら
  //       record("read:" + std::to_string(index));
  //       file_.read_raw(index) を返す
  //
  // 記録と本体呼び出しの順序は問いません（テストは順序を見ていません）。
  // ただし**弾いたときに本体へ触ってはいけません**。
  // テストが RegisterFile::hardware_access_count() を見ています。
  (void)index;
  return std::nullopt;
}

bool SafeRegisterProxy::write(std::size_t index, std::uint16_t value)
{
  // TODO: 範囲検査つきの書き込みを実装してください。
  //
  //   - 範囲外なら record("reject:write:" + std::to_string(index));
  //     ++rejected_; して false を返す（本体には触らない）
  //   - 範囲内なら record("write:" + std::to_string(index) + "=" + std::to_string(value));
  //     file_.write_raw(index, value); して true を返す
  //
  // std::to_string(value) は std::uint16_t を int に昇格させてから文字列にします。
  // 0x00ff なら "255" です。
  (void)index;
  (void)value;
  return false;
}

}  // namespace drill
