// 解答例: 結城本 第8章 Abstract Factory（実行時版）

#include "drill/actuator_kit.hpp"

namespace
{

/// シミュレーション用モータ。中身は SimMotorCore に丸投げする。
/// 仮想関数を足しているのは「実行時に製品群を差し替えたい」からだけで、
/// 動作そのものは Core が持っています。
class SimMotor final : public MotorOutput
{
public:
  explicit SimMotor(SimulationBus & bus)
  : core_(bus)
  {
  }

  void set_duty(int duty) override { core_.set_duty(duty); }
  KitId kit_id() const override { return KitId::Simulation; }

private:
  SimMotorCore core_;
};

class SimEncoder final : public EncoderInput
{
public:
  explicit SimEncoder(const SimulationBus & bus)
  : core_(bus)
  {
  }

  int read_count() const override { return core_.read_count(); }
  KitId kit_id() const override { return KitId::Simulation; }

private:
  SimEncoderCore core_;
};

class HwMotor final : public MotorOutput
{
public:
  explicit HwMotor(HardwareRegisterFile & registers)
  : core_(registers)
  {
  }

  void set_duty(int duty) override { core_.set_duty(duty); }
  KitId kit_id() const override { return KitId::Hardware; }

private:
  HwMotorCore core_;
};

class HwEncoder final : public EncoderInput
{
public:
  explicit HwEncoder(const HardwareRegisterFile & registers)
  : core_(registers)
  {
  }

  int read_count() const override { return core_.read_count(); }
  KitId kit_id() const override { return KitId::Hardware; }

private:
  HwEncoderCore core_;
};

}  // namespace

std::unique_ptr<MotorOutput> SimulationKitFactory::create_motor() const
{
  return std::make_unique<SimMotor>(bus_);
}

std::unique_ptr<EncoderInput> SimulationKitFactory::create_encoder() const
{
  // create_motor と同じ bus_ を渡している。だから対になる。
  return std::make_unique<SimEncoder>(bus_);
}

KitId SimulationKitFactory::kit_id() const
{
  return KitId::Simulation;
}

std::unique_ptr<MotorOutput> HardwareKitFactory::create_motor() const
{
  return std::make_unique<HwMotor>(registers_);
}

std::unique_ptr<EncoderInput> HardwareKitFactory::create_encoder() const
{
  return std::make_unique<HwEncoder>(registers_);
}

KitId HardwareKitFactory::kit_id() const
{
  return KitId::Hardware;
}

RunResult run_open_loop(const ActuatorKitFactory & factory, int duty, int steps)
{
  // 具体ファクトリの名前はどこにも出てこない。ここが抽象化できている証拠。
  const std::unique_ptr<MotorOutput> motor = factory.create_motor();
  const std::unique_ptr<EncoderInput> encoder = factory.create_encoder();

  for (int step = 0; step < steps; ++step) {
    motor->set_duty(duty);
  }

  return RunResult{factory.kit_id(), encoder->read_count()};
}
