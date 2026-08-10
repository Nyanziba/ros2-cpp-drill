// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <utility>

#include "drill/actuator_kit.hpp"

TEST(AbstractFactoryTest, シミュレーション製品群で抽象コードが動く)
{
  SimulationBus bus;
  const SimulationKitFactory factory{bus};

  const RunResult result = run_open_loop(factory, 10, 3);

  EXPECT_EQ(result.kit_id, KitId::Simulation);
  EXPECT_EQ(result.count_after, 30) << "duty 10 を 3 回。理想モデルなので 30";
}

TEST(AbstractFactoryTest, 実機製品群でも同じ抽象コードが動く)
{
  HardwareRegisterFile registers;
  const HardwareKitFactory factory{registers};

  // run_open_loop の中身は 1 行も変えていない。ファクトリを差し替えただけ。
  const RunResult result = run_open_loop(factory, 10, 3);

  EXPECT_EQ(result.kit_id, KitId::Hardware);
  EXPECT_EQ(result.count_after, 120) << "4 逓倍なので 10 * 3 * 4";
  EXPECT_EQ(registers.duty_register, 10) << "モータがレジスタに書けていません";
}

TEST(AbstractFactoryTest, シミュ用ファクトリの部品は全部シミュ用)
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

TEST(AbstractFactoryTest, 実機用ファクトリの部品は全部実機用)
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

TEST(AbstractFactoryTest, 同じファクトリから出た部品どうしは繋がっている)
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
    << "モータとエンコーダが同じ SimulationBus を見ていません";

  HardwareRegisterFile registers;
  const HardwareKitFactory hw_factory{registers};

  const auto hw_motor = hw_factory.create_motor();
  const auto hw_encoder = hw_factory.create_encoder();
  ASSERT_NE(hw_motor, nullptr);
  ASSERT_NE(hw_encoder, nullptr);

  hw_motor->set_duty(5);
  EXPECT_EQ(hw_encoder->read_count(), 20)
    << "モータとエンコーダが同じ HardwareRegisterFile を見ていません";
}

TEST(AbstractFactoryTest, 生成物の所有権は呼び出し側にある)
{
  static_assert(
    std::is_same<
      decltype(std::declval<const ActuatorKitFactory &>().create_motor()),
      std::unique_ptr<MotorOutput>>::value,
    "create_motor は std::unique_ptr<MotorOutput> を返すこと");
  static_assert(
    std::is_same<
      decltype(std::declval<const ActuatorKitFactory &>().create_encoder()),
      std::unique_ptr<EncoderInput>>::value,
    "create_encoder は std::unique_ptr<EncoderInput> を返すこと");

  SimulationBus bus;
  const SimulationKitFactory factory{bus};

  const auto first = factory.create_motor();
  const auto second = factory.create_motor();
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);

  EXPECT_NE(first.get(), second.get()) << "呼ぶたびに別のインスタンスを作ること";

  {
    const auto temporary = factory.create_motor();
    ASSERT_NE(temporary, nullptr);
    temporary->set_duty(3);
  }  // ここで temporary は解放される。バスは生きたまま。

  EXPECT_EQ(bus.count(), 3);
}

TEST(AbstractFactoryTest, テンプレート版が実行時版と同じ結果になる)
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

TEST(AbstractFactoryTest, テンプレート版の部品にはvtableが無い)
{
  // Core クラスは仮想関数を持ちません。だから vtable ポインタも持ちません。
  static_assert(!std::is_polymorphic<SimMotorCore>::value, "Core に vtable があります");
  static_assert(!std::is_polymorphic<SimEncoderCore>::value, "Core に vtable があります");
  static_assert(!std::is_polymorphic<HwMotorCore>::value, "Core に vtable があります");
  static_assert(!std::is_polymorphic<HwEncoderCore>::value, "Core に vtable があります");

  // 参照 1 つぶん。vtable ポインタは乗っていない。
  static_assert(sizeof(SimMotorCore) == sizeof(void *), "Core が参照 1 つより大きいです");
  static_assert(sizeof(HwMotorCore) == sizeof(void *), "Core が参照 1 つより大きいです");

  // その Core を使って、テンプレート版が実際に動くこと。
  HardwareRegisterFile registers;
  const RunResult result = run_open_loop_static_hw(registers, 2, 1);

  EXPECT_EQ(result.kit_id, KitId::Hardware);
  EXPECT_EQ(result.count_after, 8);
  EXPECT_EQ(registers.duty_register, 2);
}
