// 解答例。結城本 第21章 Proxy を C++ で書いたものです。
//
// 要点は 3 つです。
//   - Virtual Proxy の遅延生成は mutable な unique_ptr + const な operator->
//   - operator-> はポインタが返るまで繰り返し呼ばれる（drill-down）
//   - 一時オブジェクトの寿命が「本体アクセスの前後」を作る

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
  // real_ は mutable なので、const メンバ関数の中でも書き換えられます。
  // 「論理的には const、物理的には書き換える」典型例です。
  if (!real_) {
    real_ = std::make_unique<CalibrationTable>(source_);
  }
  return real_.get();
}

RegisterAccess::RegisterAccess(SafeRegisterProxy & owner)
: owner_(owner)
{
  // 実務では、ここで std::mutex をロックします。
  owner_.record("enter");
}

RegisterAccess::~RegisterAccess()
{
  // 一時オブジェクトは式の終わりで壊れるので、必ず本体アクセスの後に呼ばれます。
  // 実務では、ここで std::mutex を解放します。
  owner_.record("leave");
}

RegisterFile * RegisterAccess::operator->() const
{
  // ポインタを返した時点で operator-> の連鎖が止まり、
  // 続く ->read_raw(...) が RegisterFile のメンバ呼び出しになります。
  // friend なので private の file_ に直接届きます。
  return &owner_.file_;
}

SafeRegisterProxy::SafeRegisterProxy(RegisterFile & file)
: file_(file)
{
}

RegisterAccess SafeRegisterProxy::operator->()
{
  // RegisterAccess はコピーもムーブもできません。それでも値で返せるのは、
  // C++17 の「保証されたコピー省略」で、prvalue が呼び出し側の場所に
  // 直接構築されるからです。C++11 ではコンパイルが通りません。
  return RegisterAccess{*this};
}

std::optional<std::uint16_t> SafeRegisterProxy::read(std::size_t index)
{
  if (index >= RegisterFile::kRegisterCount) {
    record("reject:read:" + std::to_string(index));
    ++rejected_;
    return std::nullopt;
  }
  record("read:" + std::to_string(index));
  return file_.read_raw(index);
}

bool SafeRegisterProxy::write(std::size_t index, std::uint16_t value)
{
  if (index >= RegisterFile::kRegisterCount) {
    record("reject:write:" + std::to_string(index));
    ++rejected_;
    return false;
  }
  record("write:" + std::to_string(index) + "=" + std::to_string(value));
  file_.write_raw(index, value);
  return true;
}

}  // namespace drill
