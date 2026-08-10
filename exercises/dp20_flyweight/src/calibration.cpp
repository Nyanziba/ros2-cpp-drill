// I AM NOT DONE
//
// 結城本 第20章 Flyweight を C++ で書きます。
//
// 実装するのは 3 つです。
//   1. CalibrationRegistry::get()        — weak_ptr のプールから共有インスタンスを引く
//   2. CalibrationRegistry::sweep_expired() — 誰も使わなくなった残骸を掃除する
//   3. Sensor::convert()                 — 共有（intrinsic）と個体（extrinsic）を合成する
//
// ヘッダの constexpr 版（kCalibrationRom / find_spec）は実装済みです。
// 「実行時に共有する必要が本当にあるのか」を考えながら書いてください。

#include "drill/calibration.hpp"

#include <utility>

namespace drill
{

namespace
{

std::size_t & construction_counter()
{
  static std::size_t count = 0;
  return count;
}

std::size_t & destruction_counter()
{
  static std::size_t count = 0;
  return count;
}

}  // namespace

CalibrationTable::CalibrationTable(std::string model_id, double gain, double offset)
: model_id_(std::move(model_id)),
  gain_(gain),
  offset_(offset)
{
  ++construction_counter();
}

CalibrationTable::~CalibrationTable()
{
  ++destruction_counter();
}

std::size_t CalibrationTable::construction_count()
{
  return construction_counter();
}

std::size_t CalibrationTable::destruction_count()
{
  return destruction_counter();
}

void CalibrationTable::reset_counts()
{
  construction_counter() = 0;
  destruction_counter() = 0;
}

CalibrationRegistry::Handle CalibrationRegistry::get(const std::string & model_id)
{
  // TODO: プールから共有インスタンスを引いてください。手順は 4 段です。
  //
  //   1. pool_ から model_id を探す。見つかったら weak_ptr::lock() を呼ぶ。
  //      lock() が非 null を返したら、それをそのまま返す（ここが共有の本体）。
  //   2. lock() が nullptr なら、それは「誰も使わなくなった残骸」です。
  //      そのエントリは作り直します。
  //   3. find_spec(model_id) で ROM を引く。nullptr なら未知の型番なので nullptr を返す。
  //      （例外は投げません。マイコンでは -fno-exceptions が普通なので）
  //   4. std::make_shared<CalibrationTable>(...) で作り、
  //      pool_[model_id] に weak_ptr として登録してから返す。
  //
  // 注意: pool_ に shared_ptr を入れてはいけません。入れるとプールが参照を握り続け、
  //       プロセスが終わるまで CalibrationTable が解放されなくなります。
  //       テスト「全員が手放すと解放される」はそれを見ています。
  //
  // ヒント: make_shared<CalibrationTable> は shared_ptr<CalibrationTable> を返します。
  //         Handle は shared_ptr<const CalibrationTable> ですが、
  //         非 const → const への変換は暗黙に通ります。
  (void)model_id;
  return nullptr;
}

std::size_t CalibrationRegistry::pool_size() const
{
  // ここは実装済みです。expired な残骸も数に入る、という点が要点です。
  return pool_.size();
}

std::size_t CalibrationRegistry::sweep_expired()
{
  // TODO: 中身が expired になったエントリを pool_ から消し、消した個数を返してください。
  //
  // weak_ptr::expired() が true のものが対象です。
  // std::map の erase はイテレータを進めてから消す形に注意してください。
  //   for (auto it = pool_.begin(); it != pool_.end(); ) {
  //     if (...) { it = pool_.erase(it); } else { ++it; }
  //   }
  return 0;
}

Sensor::Sensor(std::string name, CalibrationRegistry::Handle table, double zero_offset)
: name_(std::move(name)),
  table_(std::move(table)),
  zero_offset_(zero_offset)
{
}

double Sensor::convert(int raw) const
{
  // TODO: raw * gain + offset + zero_offset を返してください。
  //
  //   gain と offset は table_（共有・本質的状態）から取ります。
  //   zero_offset_ は個体ごと（付帯的状態）で、共有してはいけない値です。
  //
  // table_ が nullptr のときは 0.0 を返してください（未知の型番のセンサ）。
  (void)raw;
  return 0.0;
}

}  // namespace drill
