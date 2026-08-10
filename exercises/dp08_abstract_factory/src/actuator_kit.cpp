// I AM NOT DONE
//
// 結城本 第8章 Abstract Factory の実行時版です。
// 「製品群まるごとを差し替える」構造を作ります。
//
// このファイルで実装するのは 3 つです。
//   1. 具体製品（MotorOutput / EncoderInput の派生）を匿名 namespace の中に作る
//   2. SimulationKitFactory / HardwareKitFactory の create_* と kit_id
//   3. run_open_loop — 具体ファクトリの名前を 1 つも書かずに書けること

#include "drill/actuator_kit.hpp"

namespace
{

// TODO: ここに具体製品を 4 つ作ってください。
//
//   SimMotor     : MotorOutput を継承。中身は SimMotorCore を値で持つ
//   SimEncoder   : EncoderInput を継承。中身は SimEncoderCore を値で持つ
//   HwMotor      : MotorOutput を継承。中身は HwMotorCore を値で持つ
//   HwEncoder    : EncoderInput を継承。中身は HwEncoderCore を値で持つ
//
// 書き方の例（これを 4 つ分書きます）:
//
//   class SimMotor final : public MotorOutput
//   {
//   public:
//     explicit SimMotor(SimulationBus & bus) : core_(bus) {}
//     void set_duty(int duty) override { core_.set_duty(duty); }
//     KitId kit_id() const override { return KitId::Simulation; }
//   private:
//     SimMotorCore core_;
//   };
//
// ポイント:
//   - 継承を「これ以上されない」なら final を付けます
//   - 仮想デストラクタは基底（MotorOutput）にあるので、ここでは書かなくて構いません
//   - ヘッダに出さないのは、外から名前を知る必要が無いからです。
//     呼び出し側は MotorOutput / EncoderInput という抽象しか見ません

}  // namespace

std::unique_ptr<MotorOutput> SimulationKitFactory::create_motor() const
{
  // TODO: std::make_unique<SimMotor>(bus_) を返してください。
  //
  // 生ポインタを返すと「誰が delete するか」が型に書かれません。
  (void)bus_;  // 実装したらこの行は消してください
  return nullptr;
}

std::unique_ptr<EncoderInput> SimulationKitFactory::create_encoder() const
{
  // TODO: std::make_unique<SimEncoder>(bus_) を返してください。
  //
  // ここが Abstract Factory の肝です。create_motor と同じ bus_ を渡すので、
  // このファクトリから出た部品は必ず**同じ製品群**になります。
  return nullptr;
}

KitId SimulationKitFactory::kit_id() const
{
  // TODO: KitId::Simulation を返してください。
  return KitId::Hardware;
}

std::unique_ptr<MotorOutput> HardwareKitFactory::create_motor() const
{
  // TODO: std::make_unique<HwMotor>(registers_) を返してください。
  (void)registers_;  // 実装したらこの行は消してください
  return nullptr;
}

std::unique_ptr<EncoderInput> HardwareKitFactory::create_encoder() const
{
  // TODO: std::make_unique<HwEncoder>(registers_) を返してください。
  return nullptr;
}

KitId HardwareKitFactory::kit_id() const
{
  // TODO: KitId::Hardware を返してください。
  return KitId::Simulation;
}

RunResult run_open_loop(const ActuatorKitFactory & factory, int duty, int steps)
{
  // TODO: 次の手順を、具体ファクトリの名前を 1 つも書かずに実装してください。
  //
  //   1. factory.create_motor() と factory.create_encoder() で部品を作る
  //   2. duty を steps 回 motor に与える
  //   3. encoder のカウントを読む
  //   4. RunResult{factory.kit_id(), 読んだ値} を返す
  //
  // 作った部品は unique_ptr です。この関数を抜けると自動的に解放されます。
  (void)factory;
  (void)duty;
  (void)steps;
  return RunResult{KitId::Simulation, -1};
}
