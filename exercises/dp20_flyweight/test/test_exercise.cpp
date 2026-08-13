// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <new>
#include <string>
#include <vector>

#include "drill/calibration.hpp"

// ---------------------------------------------------------------------------
// 確保カウンタ。
// グローバルの operator new を置き換えて、ヒープ確保が走った回数を数えます。
// 「constexpr 版は 1 バイトも確保しない」ことを実測するために使います。
// ---------------------------------------------------------------------------
namespace
{
std::size_t g_allocation_count = 0;
}  // namespace

void * operator new(std::size_t size)
{
  ++g_allocation_count;
  void * p = std::malloc(size == 0 ? 1 : size);
  if (p == nullptr) {
    throw std::bad_alloc();
  }
  return p;
}

void * operator new[](std::size_t size)
{
  ++g_allocation_count;
  void * p = std::malloc(size == 0 ? 1 : size);
  if (p == nullptr) {
    throw std::bad_alloc();
  }
  return p;
}

void operator delete(void * p) noexcept { std::free(p); }
void operator delete[](void * p) noexcept { std::free(p); }
void operator delete(void * p, std::size_t) noexcept { std::free(p); }
void operator delete[](void * p, std::size_t) noexcept { std::free(p); }

namespace
{

using drill::CalibrationRegistry;
using drill::CalibrationSpec;
using drill::CalibrationTable;
using drill::Sensor;
using drill::find_spec;

}  // namespace

TEST(FlyweightTest, 同じ型番を二度引くと同一のインスタンスが返る)
{
  CalibrationTable::reset_counts();
  CalibrationRegistry registry;

  const CalibrationRegistry::Handle first = registry.get("MPU6050-GYRO");
  const CalibrationRegistry::Handle second = registry.get("MPU6050-GYRO");

  ASSERT_NE(first, nullptr) << "get() が nullptr を返しています";
  ASSERT_NE(second, nullptr);

  // Flyweight の本体。中身が等しいのではなく、同じオブジェクトであること。
  EXPECT_EQ(first.get(), second.get())
    << "同じ型番なのに別のインスタンスが返っています。プールから引けていません";
  EXPECT_EQ(first.use_count(), 2);
}

TEST(FlyweightTest, 生成回数は引いた回数ではなく種類の数と一致する)
{
  CalibrationTable::reset_counts();
  CalibrationRegistry registry;

  std::vector<CalibrationRegistry::Handle> handles;
  handles.push_back(registry.get("MPU6050-GYRO"));
  handles.push_back(registry.get("AS5600-ENC"));
  handles.push_back(registry.get("MPU6050-GYRO"));
  handles.push_back(registry.get("NTC-10K"));
  handles.push_back(registry.get("AS5600-ENC"));

  for (const CalibrationRegistry::Handle & handle : handles) {
    ASSERT_NE(handle, nullptr);
  }

  EXPECT_EQ(CalibrationTable::construction_count(), 3u)
    << "5 回引いていますが、種類は 3 つです。3 個だけ作られるはずです";
  EXPECT_EQ(registry.pool_size(), 3u);
}

TEST(FlyweightTest, ROMに無い型番はnullptrを返す)
{
  CalibrationTable::reset_counts();
  CalibrationRegistry registry;

  // 既知の型番はちゃんと引けること（これが引けないと、下の比較に意味がありません）。
  const CalibrationRegistry::Handle known = registry.get("NTC-10K");
  ASSERT_NE(known, nullptr);

  const CalibrationRegistry::Handle handle = registry.get("NO-SUCH-SENSOR");
  EXPECT_EQ(handle, nullptr) << "未知の型番では nullptr を返してください";
  EXPECT_EQ(CalibrationTable::construction_count(), 1u);
  EXPECT_EQ(registry.pool_size(), 1u)
    << "作れなかった型番をプールに登録してはいけません";

  const Sensor broken{"unknown", handle, 3.0};
  EXPECT_DOUBLE_EQ(broken.convert(100), 0.0);
}

TEST(FlyweightTest, 全員が手放すとFlyweightは解放されプールからも消える)
{
  CalibrationTable::reset_counts();
  CalibrationRegistry registry;

  {
    const CalibrationRegistry::Handle first = registry.get("AS5600-ENC");
    const CalibrationRegistry::Handle second = registry.get("AS5600-ENC");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    EXPECT_EQ(first.use_count(), 2)
      << "プールが shared_ptr を握っていると use_count が 3 になります。weak_ptr で持ってください";
    EXPECT_EQ(CalibrationTable::destruction_count(), 0u);
  }

  // 利用者が全員いなくなったので、ここで破棄されているはず。
  EXPECT_EQ(CalibrationTable::destruction_count(), 1u)
    << "プールが shared_ptr で握り続けているため解放されていません";

  // ただし map のエントリ自体は残っています。weak_ptr は自分で自分を消せません。
  EXPECT_EQ(registry.pool_size(), 1u);
  EXPECT_EQ(registry.sweep_expired(), 1u);
  EXPECT_EQ(registry.pool_size(), 0u);
  EXPECT_EQ(registry.sweep_expired(), 0u);
}

TEST(FlyweightTest, 手放したあとに引き直すと作り直される)
{
  CalibrationTable::reset_counts();
  CalibrationRegistry registry;

  const CalibrationTable * first_address = nullptr;
  {
    const CalibrationRegistry::Handle first = registry.get("NTC-10K");
    ASSERT_NE(first, nullptr);
    first_address = first.get();
  }
  EXPECT_EQ(CalibrationTable::construction_count(), 1u);
  EXPECT_EQ(CalibrationTable::destruction_count(), 1u);

  const CalibrationRegistry::Handle again = registry.get("NTC-10K");
  ASSERT_NE(again, nullptr)
    << "expired な残骸に当たったとき、作り直さずに nullptr を返しています";
  EXPECT_EQ(again.use_count(), 1);
  EXPECT_EQ(CalibrationTable::construction_count(), 2u);
  EXPECT_EQ(registry.pool_size(), 1u);
  EXPECT_DOUBLE_EQ(again->offset(), -40.0);

  // アドレスが同じになることもあり得る（同じ領域が再利用される）ので、
  // アドレスの比較ではなく生成回数で判定しています。
  (void)first_address;
}

TEST(FlyweightTest, 付帯的状態は共有されない)
{
  CalibrationTable::reset_counts();
  CalibrationRegistry registry;

  const CalibrationRegistry::Handle table = registry.get("MPU6050-GYRO");
  ASSERT_NE(table, nullptr);

  const Sensor left{"gyro_left", table, 1.5};
  const Sensor right{"gyro_right", table, -2.0};

  // 本質的状態（gain / offset）は 1 個を共有している。
  EXPECT_EQ(left.table(), right.table());
  EXPECT_EQ(CalibrationTable::construction_count(), 1u);

  // 付帯的状態（zero_offset）は各センサが別々に持っている。
  const CalibrationSpec * spec = find_spec("MPU6050-GYRO");
  ASSERT_NE(spec, nullptr);

  EXPECT_DOUBLE_EQ(left.convert(100), 100 * spec->gain + spec->offset + 1.5);
  EXPECT_DOUBLE_EQ(right.convert(100), 100 * spec->gain + spec->offset - 2.0);
  EXPECT_NE(left.convert(100), right.convert(100))
    << "zero_offset を Flyweight 側に持たせると、この 2 つが同じ値になります";
}

TEST(FlyweightTest, ROMテーブルはconstexprで実行時確保がゼロ)
{
  // コンパイル時に引けている＝実行時には何も起きていない。
  constexpr const CalibrationSpec * ntc = find_spec("NTC-10K");
  static_assert(ntc != nullptr, "find_spec がコンパイル時に評価できていません");
  static_assert(ntc->offset == -40.0, "ROM の値が違います");
  static_assert(find_spec("NO-SUCH-SENSOR") == nullptr, "未知の型番は nullptr のはず");
  static_assert(std::size(drill::kCalibrationRom) == 4, "ROM の要素数");

  // 実行時に引いても確保は 1 回も走らない。
  const std::size_t before = g_allocation_count;
  const CalibrationSpec * current = find_spec("ACS712-30A");
  const double gain = current->gain;
  const std::size_t after = g_allocation_count;

  EXPECT_EQ(after, before) << "constexpr テーブルを引くのにヒープ確保は不要です";
  EXPECT_GT(gain, 0.0);

  // 実行時プールは、同じ ROM の値を持ってくるだけ。
  CalibrationTable::reset_counts();
  CalibrationRegistry registry;
  const CalibrationRegistry::Handle handle = registry.get("ACS712-30A");
  ASSERT_NE(handle, nullptr);
  EXPECT_DOUBLE_EQ(handle->gain(), current->gain);
  EXPECT_DOUBLE_EQ(handle->offset(), current->offset);
  EXPECT_EQ(handle->model_id(), std::string{"ACS712-30A"});
}
