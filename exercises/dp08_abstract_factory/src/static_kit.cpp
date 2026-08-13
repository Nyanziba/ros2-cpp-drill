// I AM NOT DONE
//
// Abstract Factory のテンプレート版（ポリシーベース）です。
// 実行時にファクトリを差し替える必要が無いなら、こちらで足ります。
// vtable もヒープ確保もゼロになります。マイコンではこちらが本命です。

#include "drill/actuator_kit.hpp"

namespace
{

// TODO: 製品群を Traits で受け取る関数テンプレートを 1 つ書いてください。
//
//   template <typename KitTraits>
//   RunResult run_open_loop_static(typename KitTraits::Bus & bus, int duty, int steps)
//   {
//     typename KitTraits::Motor motor{bus};        // ヒープ確保なし。スタックに置く
//     typename KitTraits::Encoder encoder{bus};
//     ...
//     return RunResult{KitTraits::kit_id, encoder.read_count()};
//   }
//
// 中身の手順は run_open_loop とまったく同じです。違うのは
//   - 生成が make_unique ではなく「その場に置く」だけになること
//   - Motor / Encoder の型がコンパイル時に決まること（仮想関数呼び出しが消える）
//
// typename が要る理由: KitTraits::Motor は「テンプレート引数に依存する名前」なので、
// コンパイラはそれが型なのか値なのか判断できません。typename で型だと教えます。

}  // namespace

RunResult run_open_loop_static_sim(SimulationBus & bus, int duty, int steps)
{
  // TODO: run_open_loop_static<SimulationKitTraits>(bus, duty, steps) を返してください。
  (void)bus;
  (void)duty;
  (void)steps;
  return RunResult{KitId::Hardware, -1};
}

RunResult run_open_loop_static_hw(HardwareRegisterFile & registers, int duty, int steps)
{
  // TODO: run_open_loop_static<HardwareKitTraits>(registers, duty, steps) を返してください。
  (void)registers;
  (void)duty;
  (void)steps;
  return RunResult{KitId::Simulation, -1};
}
