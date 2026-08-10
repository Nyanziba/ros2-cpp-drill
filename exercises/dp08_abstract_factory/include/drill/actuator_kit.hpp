// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>

/// どちらの製品群（family）に属するか。
///
/// Abstract Factory の価値は「製品群を混ぜられないこと」です。
/// この ID は、混ざっていないことをテストから確認するためのものです。
/// 実務のコードで毎回こういう ID を持たせる必要はありません。
enum class KitId
{
  Simulation,
  Hardware
};

// ---------------------------------------------------------------------------
// 製品群 A: シミュレーション用
// ---------------------------------------------------------------------------

/// シミュレーション側の共有状態。
/// この製品群のモータとエンコーダは、この 1 つのバスを通じて繋がっています。
/// 「モータに duty を与えるとエンコーダのカウントが進む」という理想モデルです。
class SimulationBus
{
public:
  void apply_duty(int duty)
  {
    last_duty_ = duty;
    count_ += duty;
  }

  int count() const { return count_; }
  int last_duty() const { return last_duty_; }

private:
  int count_ = 0;
  int last_duty_ = 0;
};

/// シミュレーション用モータの中身。**仮想関数を 1 つも持ちません。**
/// 実行時版（MotorOutput の派生）もテンプレート版も、実体はこれを使います。
class SimMotorCore
{
public:
  explicit SimMotorCore(SimulationBus & bus)
  : bus_(bus)
  {
  }

  void set_duty(int duty) { bus_.apply_duty(duty); }

private:
  SimulationBus & bus_;
};

/// シミュレーション用エンコーダの中身。仮想関数なし。
class SimEncoderCore
{
public:
  explicit SimEncoderCore(const SimulationBus & bus)
  : bus_(bus)
  {
  }

  int read_count() const { return bus_.count(); }

private:
  const SimulationBus & bus_;
};

// ---------------------------------------------------------------------------
// 製品群 B: 実機用
// ---------------------------------------------------------------------------

/// 実機側のレジスタ群（この課題では疑似 MMIO）。
/// エンコーダは 4 逓倍なので、同じ duty でもカウントは 4 倍進みます。
/// **シミュレーション側と数値が違うのは意図的**です。製品群が違えば挙動も違います。
class HardwareRegisterFile
{
public:
  static constexpr int kQuadratureFactor = 4;

  void write_duty(int duty)
  {
    duty_register = duty;
    count_register += duty * kQuadratureFactor;
  }

  int read_count() const { return count_register; }

  int duty_register = 0;
  int count_register = 0;
};

/// 実機用モータの中身。仮想関数なし。
class HwMotorCore
{
public:
  explicit HwMotorCore(HardwareRegisterFile & registers)
  : registers_(registers)
  {
  }

  void set_duty(int duty) { registers_.write_duty(duty); }

private:
  HardwareRegisterFile & registers_;
};

/// 実機用エンコーダの中身。仮想関数なし。
class HwEncoderCore
{
public:
  explicit HwEncoderCore(const HardwareRegisterFile & registers)
  : registers_(registers)
  {
  }

  int read_count() const { return registers_.read_count(); }

private:
  const HardwareRegisterFile & registers_;
};

// ---------------------------------------------------------------------------
// 実行時版（GoF の Abstract Factory）
// ---------------------------------------------------------------------------

/// 抽象製品 1: モータ出力。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須。unique_ptr で解放するので、無いと派生の
///     デストラクタが呼ばれません（未定義動作）。
class MotorOutput
{
public:
  virtual ~MotorOutput() = default;

  virtual void set_duty(int duty) = 0;

  /// どの製品群の部品か。
  virtual KitId kit_id() const = 0;
};

/// 抽象製品 2: エンコーダ入力。
/// MotorOutput と**対**です。この 2 つを別の製品群から取ってはいけません。
class EncoderInput
{
public:
  virtual ~EncoderInput() = default;

  virtual int read_count() const = 0;

  virtual KitId kit_id() const = 0;
};

/// 抽象ファクトリ。**製品群まるごと**を作ります。
///
/// Factory Method（第4章）との違いはここです。
/// Factory Method は「1 つの製品」を作る仮想関数、
/// Abstract Factory は「対になった複数の製品」を作る型です。
///
/// 生成物の所有権は呼び出し側に移ります（だから unique_ptr を返します）。
class ActuatorKitFactory
{
public:
  virtual ~ActuatorKitFactory() = default;

  virtual std::unique_ptr<MotorOutput> create_motor() const = 0;
  virtual std::unique_ptr<EncoderInput> create_encoder() const = 0;

  virtual KitId kit_id() const = 0;
};

/// 具体ファクトリ A。
///
/// 【寿命の約束】このファクトリも、ここから作った部品も、
/// 渡した SimulationBus より長生きさせてはいけません。
class SimulationKitFactory final : public ActuatorKitFactory
{
public:
  explicit SimulationKitFactory(SimulationBus & bus)
  : bus_(bus)
  {
  }

  std::unique_ptr<MotorOutput> create_motor() const override;
  std::unique_ptr<EncoderInput> create_encoder() const override;
  KitId kit_id() const override;

private:
  SimulationBus & bus_;
};

/// 具体ファクトリ B。
///
/// 【寿命の約束】渡した HardwareRegisterFile より長生きさせないこと。
class HardwareKitFactory final : public ActuatorKitFactory
{
public:
  explicit HardwareKitFactory(HardwareRegisterFile & registers)
  : registers_(registers)
  {
  }

  std::unique_ptr<MotorOutput> create_motor() const override;
  std::unique_ptr<EncoderInput> create_encoder() const override;
  KitId kit_id() const override;

private:
  HardwareRegisterFile & registers_;
};

/// 走らせた結果。
struct RunResult
{
  KitId kit_id;
  int count_after;
};

/// 抽象コード。**具体ファクトリの名前を 1 つも書かずに**書けること。
/// ファクトリを差し替えるだけで、両方の製品群で同じ手順が動きます。
///
/// duty を steps 回与えたあと、エンコーダのカウントを読んで返します。
RunResult run_open_loop(const ActuatorKitFactory & factory, int duty, int steps);

// ---------------------------------------------------------------------------
// テンプレート版（ポリシーベース）
// ---------------------------------------------------------------------------

/// 製品群 A の Traits。実行時多態が要らないときはこちらで足ります。
struct SimulationKitTraits
{
  using Bus = SimulationBus;
  using Motor = SimMotorCore;
  using Encoder = SimEncoderCore;
  static constexpr KitId kit_id = KitId::Simulation;
};

/// 製品群 B の Traits。
struct HardwareKitTraits
{
  using Bus = HardwareRegisterFile;
  using Motor = HwMotorCore;
  using Encoder = HwEncoderCore;
  static constexpr KitId kit_id = KitId::Hardware;
};

/// テンプレート版の入口。run_open_loop と**同じ結果**になること。
/// vtable もヒープ確保も使いません。
RunResult run_open_loop_static_sim(SimulationBus & bus, int duty, int steps);
RunResult run_open_loop_static_hw(HardwareRegisterFile & registers, int duty, int steps);
