# 8. Abstract Factory

> **結城本 第8章 対応。** `Factory`・`Link`・`Tray`・`Page` と、`ListFactory` 側の実装を手元に開いてください。
>
> **この章のねらい**: Abstract Factory は **23 章で一番「入れなくていいのに入れられる」パターン**です。
> なので順番を逆にします。**先に「入れない判断」をやってから**、
> 入れると決めたときの C++ の書き方（`unique_ptr` を返す純粋仮想クラス）に進みます。
> そのうえで、**実行時多態が要らないならテンプレートで書ける**ことを示します。
> vtable もヒープも消えます。マイコンではそちらが本命です。

## 8.1 まず「入れない」判断から

[0. 使う前に](00_使う前に.md) の 0.3「生成が 3 段になる」で挙げた例を思い出してください。

```cpp
auto factory = SensorFactoryProvider::instance().get_factory("imu");
auto builder = factory->create_builder();
auto sensor  = builder->with_address(0x68)->build();
```

Abstract Factory はこれの主犯です。入れる前に、**2 つ**だけ自分に聞いてください。

### 問1: 製品群（family）が本当に 2 つ以上あるか

Abstract Factory が扱うのは「1 種類の製品」ではなく、**対になった複数の製品のセット**です。
結城本の例なら `Link` と `Tray` と `Page` の 3 つで 1 セット、それが HTML 版と箇条書き版で 2 セットあります。

部活のコードで、これが本当に 2 セットありますか。
「モータドライバの実装が 2 つある」だけなら、それは**製品が 1 つ**です。
Factory Method（第4章）で足ります。Abstract Factory は要りません。

**セットが 1 つしか無いのに Abstract Factory を入れると、ファイルが 3 倍になって得るものがゼロです。**

### 問2: 実行時に切り替える必要が本当にあるか

ここが C++ 固有の問いです。Java では実行時多態しか選択肢がないので、
この問いは立ちません。C++ には**コンパイル時に決める**手段があります。

```cpp
// 実行時に決まる？　本当に？
const char * mode = std::getenv("ROBOT_MODE");
auto factory = make_factory(mode);
```

部活のコードで、実機用とシミュ用を**同じバイナリの中で切り替える**場面はありますか。
ビルドを分けているなら、切り替えはコンパイル時です。その場合、

```cpp
// これで足りる。vtable もヒープも要らない
using Kit = SimulationKitTraits;
```

とテンプレート引数で渡せば済みます（8.6 でやります）。

**判断表**

| 状況 | 使うもの |
| --- | --- |
| 製品が 1 種類、実装が 2 つ | Factory Method（第4章）。または関数 1 つ |
| 製品群が 2 つ以上、**ビルド時**に決まる | テンプレート（ポリシークラス） |
| 製品群が 2 つ以上、**実行時**に切り替わる | Abstract Factory（この章） |
| 製品群が 1 つしか無い | **何も入れない。** 直接 `new` するか値で持つ |

この章の課題では、**実行時版とテンプレート版の両方**を書きます。
書き比べないと「どちらで足りるか」を判断できないからです。

## 8.2 Factory Method（第4章）との違いは「1製品 vs 製品群」

この 2 つは名前が似ているせいで混同されます。違いは 1 点だけです。

| | Factory Method（第4章） | Abstract Factory（第8章） |
| --- | --- | --- |
| 作るもの | **1 種類**の製品 | **対になった複数**の製品 |
| 抽象の形 | 生成する仮想関数（1 個） | 生成する仮想関数を**複数持つ型** |
| 守りたいこと | 生成手順の共通化 | **製品群の整合性**（混ざらないこと） |
| 増やしやすいもの | 製品の**種類** | 製品**群** |
| 増やしにくいもの | — | 製品の種類（全ファクトリに追加が要る） |

3 行目が本題です。**Abstract Factory の価値は「生成をまとめること」ではありません。
「A の部品と B の部品を混ぜられないこと」です。**
ここが要らないなら、入れる意味はありません。

## 8.3 Java 版をそのまま C++ にすると

結城本の `Factory` はこうです。

```java
public abstract class Factory {
    public abstract Link createLink(String caption, String url);
    public abstract Tray createTray(String caption);
    public abstract Page createPage(String title, String author);
}
```

課題の題材（モータ出力とエンコーダ入力の対）に置き換えて、素直に C++ に移します。

```cpp
class ActuatorKitFactory
{
public:
  virtual ~ActuatorKitFactory() = default;

  virtual std::unique_ptr<MotorOutput> create_motor() const = 0;
  virtual std::unique_ptr<EncoderInput> create_encoder() const = 0;

  virtual KitId kit_id() const = 0;
};
```

Java 版から変えた点が 3 つあります。

### 変更点1: `virtual ~ActuatorKitFactory() = default;` を足した

第1章から毎回同じです。**純粋仮想関数を 1 つでも書いたら仮想デストラクタ。**
Abstract Factory では基底クラスが 3 つ（`MotorOutput` / `EncoderInput` / `ActuatorKitFactory`）
出てくるので、**書き忘れる箇所も 3 倍**です。

### 変更点2: `abstract class` ではなく純粋仮想関数だけの class にした

Java の `abstract class` は「実装を一部持てる抽象クラス」です。
結城本の `Factory` は `getFactory(String)` という静的メソッドも持っています。

C++ で同じことをやると `dynamic_cast` や RTTI の話が絡んで面倒になるので、
**抽象クラスは純粋仮想関数だけにして、生成手段（どのファクトリを使うか）は
呼び出し側に置きます。** ファクトリの生成をファクトリに書くと 0.3 の「3 段」になります。

`= 0` を書いた関数が 1 つでもあれば、そのクラスはインスタンス化できません。
Java の `abstract` に相当する印は**メンバ関数側**に付く、というのが C++ の形です。

### 変更点3: 戻りを `std::unique_ptr` にした

Java 版は `new` して返すだけ。C++ では所有権を型に書きます。次の節で扱います。

なお `create_motor()` に `const` を付けています。
ファクトリ自身の状態は変わらないからです。**Java にこの区別はありません。**

## 8.4 誰が所有するのか

```cpp
MotorOutput * create_motor() const;                  // 誰が delete する？
std::unique_ptr<MotorOutput> create_motor() const;   // 呼んだ人が所有する
```

第1章・第4章と同じ判断です。**Java が `new` して返しているところは `std::unique_ptr` を返す。**

呼ぶ側はこうなります。

```cpp
RunResult run_open_loop(const ActuatorKitFactory & factory, int duty, int steps)
{
  const std::unique_ptr<MotorOutput> motor = factory.create_motor();
  const std::unique_ptr<EncoderInput> encoder = factory.create_encoder();

  for (int step = 0; step < steps; ++step) {
    motor->set_duty(duty);
  }

  return RunResult{factory.kit_id(), encoder->read_count()};
}
```

**この関数に具体ファクトリの名前が 1 つも出てこないこと**を確認してください。
出てきたら抽象化できていません。Abstract Factory を入れた意味がありません。

一方、ファクトリ自身は `const ActuatorKitFactory &` で受けています。
`std::unique_ptr<ActuatorKitFactory>` で受けたくなりますが、**この関数はファクトリを所有しません**。
所有しないものは参照で受けます。ここも Java との差です（Java は全部参照なので迷いません）。

## 8.5 C++ 固有の危険 — 混ぜられるのは「人間」であってコンパイラではない

Abstract Factory の売りは「製品群が混ざらないこと」ですが、
**ファクトリを経由しなければ簡単に混ざります。**

```cpp
SimulationBus bus;
HardwareRegisterFile registers;

auto motor = std::make_unique<SimMotor>(bus);          // シミュ用
auto encoder = std::make_unique<HwEncoder>(registers); // 実機用。コンパイルは通る
```

`MotorOutput` と `EncoderInput` は別の型なので、この組み合わせは**型として矛盾しません**。
コンパイラは止めてくれません。制御ループはエンコーダが動かないまま duty を上げ続けます。

つまり、**「混ざらないこと」を保証しているのは型ではなく「ファクトリ経由でしか作らない」という規約**です。
規約を強制するには、具体製品クラスをヘッダに出さないことです。

```cpp
// actuator_kit.cpp の中
namespace
{
class SimMotor final : public MotorOutput { /* ... */ };
class HwEncoder final : public EncoderInput { /* ... */ };
}  // namespace
```

**匿名 namespace に閉じ込めれば、外からは名前すら書けません。**
ファクトリ経由が唯一の入口になります。課題でもこの形で書きます。

もう 1 つ、C++ 固有の落とし穴があります。**製品はファクトリより長生きできません。**

```cpp
std::unique_ptr<MotorOutput> make_motor()
{
  SimulationBus bus;                       // ローカル
  const SimulationKitFactory factory{bus};
  return factory.create_motor();           // bus はここで死ぬ
}                                          // 返ったモータは死んだバスを指している
```

製品はバス（共有状態）への参照を持っています。Java なら GC が `bus` を生かします。C++ では落ちます。
第1章 1.3 と同じ構図で、**Abstract Factory では「共有状態 → ファクトリ → 製品」と参照が 2 段になる**ぶん、
気づきにくくなっています。

## 8.6 テンプレート版（ポリシーベース） — vtable もヒープも消える

**実行時に切り替えないなら、Abstract Factory の構造はまるごとテンプレートに移せます。**

製品群を「型の集まり」として記述したものを **Traits**（ポリシー）と呼びます。

```cpp
struct SimulationKitTraits
{
  using Bus = SimulationBus;
  using Motor = SimMotorCore;
  using Encoder = SimEncoderCore;
  static constexpr KitId kit_id = KitId::Simulation;
};

struct HardwareKitTraits
{
  using Bus = HardwareRegisterFile;
  using Motor = HwMotorCore;
  using Encoder = HwEncoderCore;
  static constexpr KitId kit_id = KitId::Hardware;
};
```

これが**抽象ファクトリの代わり**です。`virtual` は 1 つもありません。
クライアントはこう書きます。

```cpp
template <typename KitTraits>
RunResult run_open_loop_static(typename KitTraits::Bus & bus, int duty, int steps)
{
  typename KitTraits::Motor motor{bus};              // ヒープ確保なし。その場に置く
  const typename KitTraits::Encoder encoder{bus};

  for (int step = 0; step < steps; ++step) {
    motor.set_duty(duty);
  }

  return RunResult{KitTraits::kit_id, encoder.read_count()};
}
```

`typename` が要るのは、`KitTraits::Motor` が**テンプレート引数に依存する名前**だからです。
コンパイラはそれが型か値か判断できないので、`typename` で「型だ」と教えます。
書き忘れると `error: expected ';' after expression` のような読みにくいエラーになります。

**実行時版との対応表**

| | 実行時版 | テンプレート版 |
| --- | --- | --- |
| 抽象ファクトリ | 純粋仮想クラス | `struct` の Traits |
| 具体ファクトリ | 派生クラス | Traits の実体 2 つ |
| 製品の生成 | `std::make_unique`（ヒープ） | ローカル変数（スタック） |
| 呼び出しコスト | 仮想関数 1 回ずつ | ゼロ（インライン展開されうる） |
| 製品 1 個のサイズ | vtable ポインタ + 中身 | 中身だけ |
| 切り替え | 実行時 | **コンパイル時のみ** |
| 差し替えの単位 | 実体（オブジェクト） | 型 |

**製品群の整合性はテンプレート版でも守られます。** `KitTraits::Motor` と `KitTraits::Encoder` は
同じ Traits から取り出すので、混ぜようがありません。むしろ実行時版より強く守られます。

代償は 2 つです。

1. **実行時に切り替えられない。** 両方の製品群を 1 つのバイナリで使うなら、両方が実体化されます
2. **コードが全部ヘッダ（またはテンプレートを定義した翻訳単位）に載る。** コンパイル時間が伸びます

課題では 2 を避けるため、テンプレートを `.cpp` の中で定義し、
`run_open_loop_static_sim` / `run_open_loop_static_hw` という**非テンプレートの入口だけ**を公開します。
テンプレートは必ずヘッダに置かねばならない、というのは誤解です。
**使う場所と同じ翻訳単位にあれば十分**です。

## 8.7 標準ライブラリ／言語機能に同じものが無いか

**ありません。** Iterator（第1章）や Proxy（第21章）と違い、
Abstract Factory に対応する標準ライブラリの部品は無いので、自分で書きます。

ただし、**言語機能で置き換わる**ことはあります。

| やりたいこと | 標準の手段 |
| --- | --- |
| 製品群をコンパイル時に切り替える | テンプレート引数（8.6） |
| 製品の型を後から差し替える | `using` エイリアス、`if constexpr` |
| ビルド構成で切り替える | CMake で `.cpp` を選ぶ。パターンは要らない |

最後の行は真面目な選択肢です。**実機用とシミュ用でリンクするファイルを変える**だけで、
抽象クラスもテンプレートも要らなくなる場合があります。
「ビルドで切り替えられないか」を必ず先に考えてください。

## 8.8 手元で試す

課題を解く前に、この 1 ファイルをコンパイルして**出力を予想してから**実行してください。

```cpp
#include <cstddef>
#include <iostream>
#include <memory>

struct SimBus  { int count = 0; };
struct HwRegs  { int count = 0; };

class Motor
{
public:
  virtual ~Motor() = default;
  virtual void set_duty(int duty) = 0;
};

class Encoder
{
public:
  virtual ~Encoder() = default;
  virtual int read() const = 0;
};

class SimMotor : public Motor
{
public:
  explicit SimMotor(SimBus & bus) : bus_(bus) {}
  void set_duty(int duty) override { bus_.count += duty; }

private:
  SimBus & bus_;
};

class SimEncoder : public Encoder
{
public:
  explicit SimEncoder(const SimBus & bus) : bus_(bus) {}
  int read() const override { return bus_.count; }

private:
  const SimBus & bus_;
};

class HwEncoder : public Encoder
{
public:
  explicit HwEncoder(const HwRegs & regs) : regs_(regs) {}
  int read() const override { return regs_.count; }

private:
  const HwRegs & regs_;
};

// テンプレート版の製品（仮想関数なし）
struct SimMotorCore
{
  explicit SimMotorCore(SimBus & bus) : bus_(bus) {}
  void set_duty(int duty) { bus_.count += duty; }
  SimBus & bus_;
};

int main()
{
  SimBus bus;
  HwRegs regs;

  // ファクトリを通さず手で組むと、製品群を混ぜられてしまう
  std::unique_ptr<Motor> motor = std::make_unique<SimMotor>(bus);
  std::unique_ptr<Encoder> encoder = std::make_unique<HwEncoder>(regs);
  motor->set_duty(10);
  std::cout << "mixed:  " << encoder->read() << "\n";

  std::unique_ptr<Encoder> right = std::make_unique<SimEncoder>(bus);
  std::cout << "paired: " << right->read() << "\n";

  std::cout << "sizeof(SimMotor)     = " << sizeof(SimMotor) << "\n";
  std::cout << "sizeof(SimMotorCore) = " << sizeof(SimMotorCore) << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: <code>mixed</code> は何を出すか。そして 2 つの <code>sizeof</code> の差はどこから来るか</summary>

```
mixed:  0
paired: 10
sizeof(SimMotor)     = 16
sizeof(SimMotorCore) = 8
```

**`mixed` は 0 です。** シミュ用モータに duty を与えても、実機用エンコーダは何も見ていません。
**コンパイルは通ります。** 型は矛盾していないからです。
制御ループなら「指令は出ているのにフィードバックが返らない」状態で、
積分項が飽和して全力で回ります。これが Abstract Factory が防ぎたい事故です。

`sizeof` の差 8 バイトは **vtable ポインタ**です。
`SimMotorCore` は参照 1 つ（8 バイト）だけ、`SimMotor` はそこに vtable ポインタが乗ります。
製品を 100 個持つならこの差がそのまま RAM に出ます。
（64bit 環境の値です。32bit のマイコンなら 4 と 8 になります）
</details>

## 8.9 マイコンでの結論

**結論を先に書きます。**

1. **まずテンプレート版（8.6）を検討する。** ヒープも vtable も要らない
2. **実行時に切り替えたい理由が言えないなら、ビルドで切り替える**
3. それでも実行時多態が要るなら、**`unique_ptr` を返さない Abstract Factory** を書く

3 の書き方が本題です。マイコンで問題なのは `create_motor()` が `std::make_unique` を呼ぶことです。
起動時に 1 回なら許容できますが、そもそも**生成する必要がありません**。
ペリフェラルは電源投入時から 1 個ずつ存在しています。

そこで、**ファクトリが製品を値メンバとして持ち、参照を配る**形にします。

```cpp
/// 抽象キット。「作る」のではなく「既にあるものへの参照を配る」。
class ActuatorKit
{
public:
  virtual ~ActuatorKit() = default;
  virtual MotorOutput & motor() = 0;
  virtual EncoderInput & encoder() = 0;
};

/// 具体キットが製品を値で持つ。ヒープ確保はどこにもない。
class HardwareKit final : public ActuatorKit
{
public:
  explicit HardwareKit(Registers & regs)
  : motor_(regs), encoder_(regs)
  {
  }

  MotorOutput & motor() override { return motor_; }
  EncoderInput & encoder() override { return encoder_; }

private:
  HwMotor motor_;
  HwEncoder encoder_;
};

Registers g_regs{0, 0};
HardwareKit g_kit{g_regs};      // 静的記憶域。確保は起動前に終わっている

std::uint32_t drive(ActuatorKit & kit, std::uint32_t duty)
{
  kit.motor().set_duty(duty);
  return kit.encoder().read_count();
}
```

これは `-fno-exceptions -fno-rtti` を付けても警告ゼロで通ります（実測済み）。

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti -c micro.cpp
```

**変えた点は 1 つだけです。`create_*()` が `unique_ptr` を返すのをやめ、`&` を返すようにした。**
これで、

- ヒープ確保がゼロになる
- 製品の寿命がキットの寿命と一致する（**ぶら下がりポインタが構造的に作れない**）
- 製品群の整合性は保たれる（キットが同じ `Registers` を両方に渡している）

残るコストは vtable だけです。`ActuatorKit` / `MotorOutput` / `EncoderInput` の 3 つで
vtable が 3 + 具体クラスぶん増えます。これが惜しいなら 8.6 のテンプレート版にします。

注意点が 2 つあります。

- **`g_kit` はグローバル変数です。** 複数の翻訳単位にまたがると初期化順序の問題が出ます
  （第5章 Singleton の話）。`g_regs` と `g_kit` は同じ `.cpp` に置いてください
- **`motor()` は `const` にできません。** 非 const 参照を返すからです。
  ここを `const` にしたくなったら設計を見直す合図です

## 8.10 ROS 2 での結論（補足）

ROS 2 側は制約が緩いので、8.3〜8.4 の `unique_ptr` 版をそのまま使えます。

実際に近いのは **`pluginlib`** です。`pluginlib::ClassLoader` が
「実行時に文字列でクラスを選んで `shared_ptr` を返す」ので、役割は Abstract Factory と同じです。
ただし提供するのは**製品 1 種類**なので、正確には Factory Method 側です。
製品群の整合性は pluginlib では守られません。**そこは自分で書く必要があります。**

`rclcpp` 自体に Abstract Factory 的な抽象は出てきません。
シミュレーションと実機の切り替えは、パターンではなく**ノードの差し替え**（launch ファイル）で
やるのが ROS 2 の流儀です。**その方が良い場合がほとんどです。**
「Abstract Factory を書く前に、launch で分けられないか」を先に考えてください。

## 8.11 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| モータに指令してもエンコーダのカウントが変わらない | `create_motor` と `create_encoder` が別の bus / registers を見ている。製品群が混ざっている |
| ファクトリを差し替えたのに挙動が変わらない | `run_open_loop` の中に具体クラス名を書いている |
| ファクトリを解放したらプログラムが落ちる | 仮想デストラクタが無い。基底 3 つ全部に要る |
| ファクトリを返す関数から製品を返したら落ちた | 8.5 の寿命の問題。共有状態 → ファクトリ → 製品と参照が 2 段 |
| `error: expected ';'` がテンプレート版で出る | `typename KitTraits::Motor` の `typename` を忘れている |
| 製品を増やしたら全ファクトリの修正が必要になった | Abstract Factory の構造上の欠点。製品**群**は増やしやすいが製品**種**は増やしにくい |
| `create_motor()` が `const` にできない | ファクトリが状態を持ってしまっている。設計の見直し |
| 具体製品クラスをテストから直接使いたくなった | 匿名 namespace に閉じ込めているから。それが正しい。テストはファクトリ経由で書く |

## 8.12 対応する課題

```bash
./drill run dp08
```

題材は**モータ出力とエンコーダ入力の対**です。
シミュレーション用の製品群と実機用の製品群があり、混ぜてはいけません。

`exercises/dp08_abstract_factory/src/actuator_kit.cpp` に、

1. 具体製品 4 つ（`SimMotor` / `SimEncoder` / `HwMotor` / `HwEncoder`）を**匿名 namespace の中に**
2. `SimulationKitFactory` / `HardwareKitFactory` の `create_motor()` / `create_encoder()` / `kit_id()`
3. `run_open_loop()` — **具体ファクトリの名前を 1 つも書かずに**

`exercises/dp08_abstract_factory/src/static_kit.cpp` に、

4. `run_open_loop_static<KitTraits>()` — テンプレート版

を実装します。テストは、**ファクトリを差し替えるだけで同じ抽象コードが両方の製品群で動くこと**、
**生成された部品どうしが同じ製品群に属していること**、
**所有権が呼び出し側にあること**、
**テンプレート版が実行時版と同じ結果になること**を見ます。
`static_assert` で「テンプレート版の製品に vtable が無い」ことも確認します。

## 8.13 この章のまとめ

- **入れない判断が先。** 製品群が 2 つ以上あり、実行時に切り替わるときだけ入れる
- Factory Method との違いは **1 製品 vs 製品群**。名前が似ているだけで目的が違う
- **本来の価値は「生成をまとめること」ではなく「製品群が混ざらないこと」**。
  ここが要らないなら入れる意味はない
- Java の `interface` / `abstract class` → 純粋仮想関数だけの class。
  **基底が 3 つ出るので仮想デストラクタの書き忘れも 3 倍**
- Java の `new` して返すところは **`std::unique_ptr` を返す**
- **型は混合を止めてくれない。** 具体製品を匿名 namespace に閉じ込めて、ファクトリを唯一の入口にする
- **実行時多態が要らないなら Traits + テンプレート。** vtable もヒープも消える。整合性はむしろ強くなる
- マイコンでは `unique_ptr` を返さず**参照を配る**。製品はキットが値で持つ。確保ゼロ、寿命も安全
- ROS 2 では launch でノードごと差し替えられないかを先に考える

---

前: [7. Builder](07_Builder.md) ／ 次: 9. Bridge（準備中）
