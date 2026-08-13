// 解答例: 結城本 第8章 Abstract Factory（テンプレート版 / ポリシーベース）

#include "drill/actuator_kit.hpp"

namespace
{

/// 実行時版の run_open_loop と手順は同じ。
/// 違うのは、製品の型が**コンパイル時に**決まっていることだけ。
///   - 仮想関数呼び出しがゼロ（すべてインライン展開されうる）
///   - ヒープ確保がゼロ（モータもエンコーダもスタックに置く）
template <typename KitTraits>
RunResult run_open_loop_static(typename KitTraits::Bus & bus, int duty, int steps)
{
  typename KitTraits::Motor motor{bus};
  const typename KitTraits::Encoder encoder{bus};

  for (int step = 0; step < steps; ++step) {
    motor.set_duty(duty);
  }

  return RunResult{KitTraits::kit_id, encoder.read_count()};
}

}  // namespace

RunResult run_open_loop_static_sim(SimulationBus & bus, int duty, int steps)
{
  return run_open_loop_static<SimulationKitTraits>(bus, duty, steps);
}

RunResult run_open_loop_static_hw(HardwareRegisterFile & registers, int duty, int steps)
{
  return run_open_loop_static<HardwareKitTraits>(registers, duty, steps);
}
