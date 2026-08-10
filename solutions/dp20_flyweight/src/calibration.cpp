// 解答例
//
// 結城本 第20章 Flyweight を C++ で書いたもの。
// プールを std::weak_ptr で持つのが Java 版との一番大きい差です。

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
  // 1. 生きているものがプールにあれば、それを共有する。
  //    weak_ptr::lock() は「まだ生きていれば shared_ptr を作る」操作です。
  //    ここで shared_ptr が返ることが、Flyweight が共有されているということです。
  const auto found = pool_.find(model_id);
  if (found != pool_.end()) {
    Handle alive = found->second.lock();
    if (alive != nullptr) {
      return alive;
    }
    // lock() が nullptr = 誰も使わなくなった残骸。この下で作り直します。
  }

  // 2. 本質的状態は ROM から取る。実行時に決まるのはここだけです。
  const CalibrationSpec * spec = find_spec(model_id);
  if (spec == nullptr) {
    // 未知の型番。例外は投げません（マイコンでは -fno-exceptions が普通なので）。
    return nullptr;
  }

  // 3. 作って、プールには weak_ptr として登録する。
  //    ここで shared_ptr を登録すると、プールが握り続けて永久に解放されなくなります。
  const std::shared_ptr<CalibrationTable> created =
    std::make_shared<CalibrationTable>(model_id, spec->gain, spec->offset);
  pool_[model_id] = created;
  return created;
}

std::size_t CalibrationRegistry::pool_size() const
{
  // expired な残骸も数に入ります。weak_ptr は自分で自分を map から消せません。
  return pool_.size();
}

std::size_t CalibrationRegistry::sweep_expired()
{
  std::size_t removed = 0;
  for (auto it = pool_.begin(); it != pool_.end();) {
    if (it->second.expired()) {
      it = pool_.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }
  return removed;
}

Sensor::Sensor(std::string name, CalibrationRegistry::Handle table, double zero_offset)
: name_(std::move(name)),
  table_(std::move(table)),
  zero_offset_(zero_offset)
{
}

double Sensor::convert(int raw) const
{
  if (table_ == nullptr) {
    return 0.0;
  }
  // gain / offset は共有（intrinsic）、zero_offset_ は個体ごと（extrinsic）。
  // zero_offset_ を CalibrationTable 側に置いた瞬間、共有できなくなります。
  return raw * table_->gain() + table_->offset() + zero_offset_;
}

}  // namespace drill
