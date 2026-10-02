// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <utility>

#include "drill/actuator_kit.hpp"
#include "drill_i18n.hpp"
#include "drill_i18n.hpp"

TEST(AbstractFactoryTest, AbstractCodeWorksWithSimulationFamily)
{
  SimulationBus bus;
  const SimulationKitFactory factory{bus};

  const RunResult result = run_open_loop(factory, 10, 3);

  EXPECT_EQ(result.kit_id, KitId::Simulation);
  EXPECT_EQ(result.count_after, 30) << drill::localized("duty 10 を 3 回。理想モデルなので 30", "duty 10 three times. It is an ideal model, so 30");
}

TEST(AbstractFactoryTest, SameAbstractCodeWorksWithHardwareFamily)
{
  HardwareRegisterFile registers;
  const HardwareKitFactory factory{registers};

  // run_open_loop の中身は 1 行も変えていない。ファクトリを差し替えただけ。
  const RunResult result = run_open_loop(factory, 10, 3);

  EXPECT_EQ(result.kit_id, KitId::Hardware);
  EXPECT_EQ(result.count_after, 120) << drill::localized("4 逓倍なので 10 * 3 * 4", "It is multiplied by 4, so 10 * 3 * 4");
  EXPECT_EQ(registers.duty_register, 10) << drill::localized("モータがレジスタに書けていません", "The motor did not write to the register");
}

TEST(AbstractFactoryTest, SimulationFactoryPartsAreAllSimulation)
{
  SimulationBus bus;
  const SimulationKitFactory factory{bus};

  const auto motor = factory.create_motor();
  const auto encoder = factory.create_encoder();
  ASSERT_NE(motor, nullptr);
  ASSERT_NE(encoder, nullptr);

  EXPECT_EQ(factory.kit_id(), KitId::Simulation);
  EXPECT_EQ(motor->kit_id(), KitId::Simulation);
  EXPECT_EQ(encoder->kit_id(), KitId::Simulation);
}

TEST(AbstractFactoryTest, HardwareFactoryPartsAreAllHardware)
{
  HardwareRegisterFile registers;
  const HardwareKitFactory factory{registers};

  const auto motor = factory.create_motor();
  const auto encoder = factory.create_encoder();
  ASSERT_NE(motor, nullptr);
  ASSERT_NE(encoder, nullptr);

  EXPECT_EQ(factory.kit_id(), KitId::Hardware);
  EXPECT_EQ(motor->kit_id(), KitId::Hardware);
  EXPECT_EQ(encoder->kit_id(), KitId::Hardware);
}

TEST(AbstractFactoryTest, PartsFromSameFactoryAreConnected)
{
  // Abstract Factory の本来の価値がこれ。
  // 「モータとエンコーダが対になっている」ことをファクトリが保証する。
  SimulationBus bus;
  const SimulationKitFactory sim_factory{bus};

  const auto sim_motor = sim_factory.create_motor();
  const auto sim_encoder = sim_factory.create_encoder();
  ASSERT_NE(sim_motor, nullptr);
  ASSERT_NE(sim_encoder, nullptr);

  sim_motor->set_duty(7);
  EXPECT_EQ(sim_encoder->read_count(), 7)
    << drill::localized("モータとエンコーダが同じ SimulationBus を見ていません", "The motor and the encoder do not use the same SimulationBus");

  HardwareRegisterFile registers;
  const HardwareKitFactory hw_factory{registers};

  const auto hw_motor = hw_factory.create_motor();
  const auto hw_encoder = hw_factory.create_encoder();
  ASSERT_NE(hw_motor, nullptr);
  ASSERT_NE(hw_encoder, nullptr);

  hw_motor->set_duty(5);
  EXPECT_EQ(hw_encoder->read_count(), 20)
    << drill::localized("モータとエンコーダが同じ HardwareRegisterFile を見ていません", "The motor and the encoder do not use the same HardwareRegisterFile");
}

TEST(AbstractFactoryTest, CallerOwnsCreatedProducts)
{
  static_assert(
    std::is_same<
      decltype(std::declval<const ActuatorKitFactory &>().create_motor()),
      std::unique_ptr<MotorOutput>>::value,
    "create_motor は std::unique_ptr<MotorOutput> を返すこと / create_motor must return std::unique_ptr<MotorOutput>");
  static_assert(
    std::is_same<
      decltype(std::declval<const ActuatorKitFactory &>().create_encoder()),
      std::unique_ptr<EncoderInput>>::value,
    "create_encoder は std::unique_ptr<EncoderInput> を返すこと / create_encoder must return std::unique_ptr<EncoderInput>");

  SimulationBus bus;
  const SimulationKitFactory factory{bus};

  const auto first = factory.create_motor();
  const auto second = factory.create_motor();
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);

  EXPECT_NE(first.get(), second.get()) << drill::localized("呼ぶたびに別のインスタンスを作ること", "Create a new instance on each call");

  {
    const auto temporary = factory.create_motor();
    ASSERT_NE(temporary, nullptr);
    temporary->set_duty(3);
  }  // ここで temporary は解放される。バスは生きたまま。

  EXPECT_EQ(bus.count(), 3);
}

TEST(AbstractFactoryTest, TemplateVersionMatchesRuntimeVersion)
{
  SimulationBus runtime_bus;
  SimulationBus static_bus;
  const SimulationKitFactory sim_factory{runtime_bus};

  const RunResult runtime_sim = run_open_loop(sim_factory, 4, 5);
  const RunResult static_sim = run_open_loop_static_sim(static_bus, 4, 5);

  EXPECT_EQ(static_sim.kit_id, runtime_sim.kit_id);
  EXPECT_EQ(static_sim.count_after, runtime_sim.count_after);
  EXPECT_EQ(static_sim.count_after, 20);

  HardwareRegisterFile runtime_registers;
  HardwareRegisterFile static_registers;
  const HardwareKitFactory hw_factory{runtime_registers};

  const RunResult runtime_hw = run_open_loop(hw_factory, 4, 5);
  const RunResult static_hw = run_open_loop_static_hw(static_registers, 4, 5);

  EXPECT_EQ(static_hw.kit_id, runtime_hw.kit_id);
  EXPECT_EQ(static_hw.count_after, runtime_hw.count_after);
  EXPECT_EQ(static_hw.count_after, 80);
}

TEST(AbstractFactoryTest, TemplateVersionPartsHaveNoVtable)
{
  // Core クラスは仮想関数を持ちません。だから vtable ポインタも持ちません。
  static_assert(!std::is_polymorphic<SimMotorCore>::value, "Core に vtable があります / Core has a vtable");
  static_assert(!std::is_polymorphic<SimEncoderCore>::value, "Core に vtable があります / Core has a vtable");
  static_assert(!std::is_polymorphic<HwMotorCore>::value, "Core に vtable があります / Core has a vtable");
  static_assert(!std::is_polymorphic<HwEncoderCore>::value, "Core に vtable があります / Core has a vtable");

  // 参照 1 つぶん。vtable ポインタは乗っていない。
  static_assert(sizeof(SimMotorCore) == sizeof(void *), "Core が参照 1 つより大きいです / Core is larger than one reference");
  static_assert(sizeof(HwMotorCore) == sizeof(void *), "Core が参照 1 つより大きいです / Core is larger than one reference");

  // その Core を使って、テンプレート版が実際に動くこと。
  HardwareRegisterFile registers;
  const RunResult result = run_open_loop_static_hw(registers, 2, 1);

  EXPECT_EQ(result.kit_id, KitId::Hardware);
  EXPECT_EQ(result.count_after, 8);
  EXPECT_EQ(registers.duty_register, 2);
}
