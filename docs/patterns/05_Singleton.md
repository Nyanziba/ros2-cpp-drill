# 5. Singleton

> **結城本 第5章 対応。** `Singleton` クラスと `Main` を手元に開いてください。
>
> **この章のねらい**: Java 版の Singleton は `private static Singleton singleton = new Singleton();` の
> 1 行で終わります。C++ で同じ 1 行を書くと、**翻訳ユニットをまたいだ初期化順序が未定義**になって壊れます。
> さらに Java では起きない「コピーできてしまう」事故が加わります。
> この章では **Meyers Singleton**（関数ローカル static）へ書き換える理由を、
> 実際に壊れるコードを動かして確かめます。
> そのうえで、**そもそも使わない判断**を最初に置きます。

## 5.1 まず「使わない判断」から

[0. 使う前に](00_使う前に.md) の 0.2 で書いたことを、もう一度書きます。

**Singleton は、グローバル変数に「デザインパターン」という名前を付けただけになりがちです。**

```cpp
// これはパターンではなく、ただのグローバル変数
class Config
{
public:
  static Config & instance();
  int motor_max_duty = 100;
};
```

これで解決した問題を数えてください。**ゼロ**です。

| グローバル変数の問題 | Singleton にすると |
| --- | --- |
| どこからでも書き換えられる | そのまま |
| 誰が依存しているかコードから読めない | そのまま（引数に現れない） |
| テストで差し替えられない | そのまま |
| 前のテストの状態が次に漏れる | そのまま |
| マルチスレッドで壊れる | 初期化だけは安全になる。**中身の競合はそのまま** |

減ったのは罪悪感だけです。それが一番害です。

### 先に考えること: 引数で渡せないか

```cpp
// Singleton 版: 依存が関数の見た目に出てこない
void drive_motor(double duty)
{
  const int limit = Config::instance().motor_max_duty;   // 隠れた依存
  // ...
}

// 引数版: 何に依存しているかが型に書いてある
void drive_motor(const Config & config, double duty)
{
  const int limit = config.motor_max_duty;
}
```

引数版はテストで別の `Config` を渡すだけで済みます。`reset()` も要りません。
**`Config` を使う関数が 3 つなら、3 つに渡してください。** それで終わりです。

### それでも本当に 1 個しかないもの

判断基準は 1 つだけです。

> **「2 個目を構築できてしまうと壊れるか」**

- `Config`: 2 個あっても壊れません。片方を使えばいいだけ → **シングルトンにしない**
- `UART1`: 2 個の `Uart` オブジェクトが同じレジスタを別々の状態だと思って叩きます → **壊れる**

マイコンのペリフェラル（UART・SPI・I2C・タイマ・DMA チャネル）は、
**物理的に世界に 1 個**です。ここだけがシングルトンの正当な使いどころです。
「持ち回るのが面倒」は理由になりません。

以降は「本当に 1 個しかない」と決まったあとの話です。

## 5.2 Java 版をそのまま C++ にすると

結城本の `Singleton` はこうです。

```java
public class Singleton {
    private static Singleton singleton = new Singleton();
    private Singleton() {
        System.out.println("インスタンスを生成しました。");
    }
    public static Singleton getInstance() {
        return singleton;
    }
}
```

C++ に素直に移すとこうなります。

```cpp
// config.hpp
class Config
{
public:
  Config();
  int max_duty() const { return max_duty_; }

private:
  int max_duty_;
};

extern Config g_config;      // 実体は config.cpp
```

```cpp
// config.cpp
Config g_config;             // Java の static フィールドに相当
```

**これが壊れます。** 変更点は 3 つあります。

### 変更点1: static フィールド相当のグローバルをやめる（本題）

Java の `private static Singleton singleton = new Singleton();` は、
**クラスが最初に使われたときに初期化される**（クラスローダの仕事）ことが仕様で決まっています。

C++ の名前空間スコープのオブジェクトは違います。
**同じ翻訳ユニットの中では書いた順**ですが、
**翻訳ユニットをまたぐと順序は未定義**です。これが
**static initialization order fiasco**（静的初期化順序の大失敗）です。

### 変更点2: コピー・ムーブを `= delete` する

Java では `Singleton s = Singleton.getInstance();` と書いても参照が入るだけです。
C++ では**コピーが作られます**。5.4 でやります。

### 変更点3: `getInstance()` はポインタではなく参照を返す

```cpp
static Config * instance();      // 呼んだ人が delete していいのか分からない
static Config & instance();      // 解放しない、が型に書かれている
```

第1章の `unique_ptr` と同じ話です。**所有権を型で表明します。**

## 5.3 実際に壊す — static initialization order fiasco

3 ファイル作ります。

```cpp
// config.hpp
#pragma once
#include <iostream>

class Config
{
public:
  Config()
  : max_duty_(100)
  {
    std::cout << "Config のコンストラクタ\n";
  }
  int max_duty() const { return max_duty_; }

private:
  int max_duty_;
};

extern Config g_config;   // 実体は config.cpp
```

```cpp
// config.cpp
#include "config.hpp"

Config g_config;          // 翻訳ユニット config.cpp のグローバル
```

```cpp
// limiter.cpp
#include "config.hpp"

class Limiter
{
public:
  Limiter()
  : limit_(g_config.max_duty())   // 別の翻訳ユニットのグローバルを使う
  {
    std::cout << "Limiter のコンストラクタ: limit_ = " << limit_ << "\n";
  }
  int limit() const { return limit_; }

private:
  int limit_;
};

Limiter g_limiter;
```

```cpp
// main_siof.cpp
#include "config.hpp"

int main()
{
  std::cout << "main: g_config.max_duty() = " << g_config.max_duty() << "\n";
  return 0;
}
```

`limiter.cpp` を先に並べてビルドします。

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic limiter.cpp config.cpp main_siof.cpp -o siof1 && ./siof1
```

実際の出力です。

```
Limiter のコンストラクタ: limit_ = 0
Config のコンストラクタ
main: g_config.max_duty() = 100
```

**`limit_` が 0 になりました。** `Limiter` のコンストラクタが
`Config` のコンストラクタより先に走り、まだ構築されていない `g_config` を読んだからです。
`max_duty_` はゼロ初期化されていただけです。

コンパイルは通ります。警告も出ません。**実行してもクラッシュしません。**
モータの上限が 100 のはずが 0 になって、「動かない」とだけ言われる。
これが一番たちの悪い壊れ方です。

### さらに悪いことに、リンク順を変えると直る

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic config.cpp limiter.cpp main_siof.cpp -o siof2 && ./siof2
```

```
Config のコンストラクタ
Limiter のコンストラクタ: limit_ = 100
main: g_config.max_duty() = 100
```

**ソースを 1 文字も変えずに、リンク順だけで直りました。**
つまり「自分の環境では動いた」が何の保証にもなりません。
CMake のターゲットにファイルを 1 つ足しただけで再発します。

### 直し方 — Meyers Singleton

```cpp
// config2.hpp
#pragma once
#include <iostream>

class Config
{
public:
  static Config & instance()      // Meyers Singleton
  {
    static Config the_config;     // 最初にここを通ったときだけ構築される
    return the_config;
  }
  int max_duty() const { return max_duty_; }

private:
  Config()
  : max_duty_(100)
  {
    std::cout << "Config のコンストラクタ\n";
  }
  int max_duty_;
};
```

`limiter.cpp` 側は `g_config` を `Config::instance()` に置き換えるだけです。
どちらのリンク順でもこうなります。

```
Config のコンストラクタ
Limiter のコンストラクタ: limit_ = 100
main: 100
```

**なぜ直ったか。** 関数ローカルの `static` は、
**その行に最初に到達したときに初期化される**と規格が決めています。
初期化の時点が「プログラム開始のどこか」から「使う人が呼んだ瞬間」に変わったので、
**使う前に初期化される**ことが保証されます。順序を人間が管理する必要が消えました。

`static` の寿命とリンケージそのものの説明は
[C++入門編 7. static](../cpp-basics/07_static.md) にあります。ここでは繰り返しません。

### スレッドセーフでもある

C++11 以降、**関数ローカル static の初期化はスレッドセーフであることが規格で保証されています**
（俗に「マジックスタティック」）。
複数のスレッドが同時に `instance()` に入っても、構築は 1 回だけで、
他のスレッドは構築の完了を待ちます。

```cpp
// これは要りません。書くと遅くなるだけです
static std::mutex g_mutex;
Config & Config::instance()
{
  std::lock_guard<std::mutex> lock(g_mutex);   // 不要
  static Config the_config;
  return the_config;
}
```

C++03 の時代に書かれた「ダブルチェックロッキング」の記事が今も残っていますが、
**C++11 以降は不要**です。素直に `static` を 1 行置いてください。

**ただし守られるのは初期化だけです。** 構築後に複数スレッドから
`set_baud_rate()` を呼べば、それは普通のデータ競合です。そこは自分で守ります。

## 5.4 Java では起きない事故 — コピーできてしまう

C++ 固有の危険です。ここを落とすと、シングルトンは静かに崩れます。

```cpp
Config copied = Config::instance();   // ← Java なら参照が入るだけ。C++ ではコピー
```

コンストラクタを `private` にしても**コピーコンストラクタは暗黙に生成されて public** です。
つまり「外から作れない」ようにしたつもりで、**既にあるインスタンスからは複製できます**。

同じ事故は、うっかり値で受けるだけでも起きます。

```cpp
void configure(Config config);        // 参照ではなく値。ここでコピー
auto config = Config::instance();     // auto は Config& ではなく Config に推論される
```

3 つ目が特に多いです。`auto` は参照を落とします。`auto &` と書くべきところです。

防ぎ方は 4 行です。

```cpp
class Config
{
public:
  static Config & instance();

  Config(const Config &) = delete;
  Config & operator=(const Config &) = delete;
  Config(Config &&) = delete;
  Config & operator=(Config &&) = delete;

private:
  Config() = default;
};
```

コピーの 2 つを `= delete` すればムーブは暗黙に生成されなくなりますが、
**4 つ書いてください。** 「意図的に禁止した」ことが読む人に伝わります。

## 5.5 手元で試す

1 ファイルで完結します。**出力を予想してから**実行してください。

```cpp
#include <iostream>

class Config
{
public:
  static Config & instance()
  {
    static Config the_config;
    return the_config;
  }
  void set_max_duty(int duty) { max_duty_ = duty; }
  int max_duty() const { return max_duty_; }

private:
  Config() = default;
  int max_duty_ = 100;
};

int main()
{
  Config copied = Config::instance();     // コピーが作られる
  copied.set_max_duty(30);

  std::cout << "instance: " << &Config::instance()
            << " duty=" << Config::instance().max_duty() << "\n";
  std::cout << "copied  : " << &copied
            << " duty=" << copied.max_duty() << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: コンパイルは通るか。通るなら 2 行のアドレスと duty はどうなるか</summary>

**通ります。警告も出ません。** 実行結果（アドレスは環境で変わります）。

```
instance: 0x102b80000 duty=100
copied  : 0x16d285d48 duty=30
```

アドレスが違います。**`Config` が 2 個できています。**
`copied.set_max_duty(30)` はコピーの方だけを書き換えたので、
本体は 100 のままです。「設定したのに反映されない」というバグの正体がこれです。

`private:` の直前に次の 2 行を足してから、もう一度ビルドしてください。

```cpp
  Config(const Config &) = delete;
  Config & operator=(const Config &) = delete;
```

実際に出るエラーです（Apple clang。行番号は環境で変わります）。

```
error: call to deleted constructor of 'Config'
  Config copied = Config::instance();     // コピーが作られる
         ^        ~~~~~~~~~~~~~~~~~~
note: 'Config' has been explicitly marked deleted here
  Config(const Config &) = delete;
  ^
1 error generated.
```

**`= delete` を書いて初めて、コンパイラが事故を止めてくれます。**
書かない限り、これは実行時にしか気付けません。
</details>

## 5.6 もう一つの順序 — 破棄のときに壊れる

初期化順序を Meyers Singleton で片付けても、**破棄の順序**が残ります。
関数ローカル static は「構築された順の逆順」で破棄されます。
シングルトンが 2 つあって、片方のデストラクタがもう片方を使うと壊れます。

```cpp
class Logger
{
public:
  static Logger & instance()
  {
    static Logger the_logger;
    return the_logger;
  }
  ~Logger() { alive_ = false; }

  void log(const std::string & message)
  {
    std::cout << "[log alive=" << alive_ << "] " << message << "\n";
  }

private:
  Logger() = default;
  bool alive_ = true;
};

class Uart
{
public:
  static Uart & instance()
  {
    static Uart the_uart;
    return the_uart;
  }
  ~Uart() { Logger::instance().log("Uart を閉じました"); }   // 破棄済みかもしれない

private:
  Uart() = default;
};
```

`main` で `Uart::instance()` を先に触った場合の実際の出力です。

```
main 終了
[log alive=0] Uart を閉じました
```

`alive=0` は、**既にデストラクタが走った `Logger` を触っている**という意味です。
未定義動作です。今回はたまたま値が読めていますが、
`std::string` や `std::vector` を持っていれば解放済みメモリを触ります。

`Logger::instance()` を先に触った場合はこうなります。

```
main 終了
[log alive=1] Uart を閉じました
```

**コードは同じで、触った順序が違うだけ**です。5.3 と同じ構図が破棄側で再発しています。

### 対処

| 方法 | 書き方 | 代償 |
| --- | --- | --- |
| デストラクタで他のシングルトンを使わない | 後始末は明示的な `close()` に出す | 呼び忘れる |
| 使う側を先に構築しておく | `main` の先頭で `Logger::instance();` を呼ぶ | 規約。書き忘れると再発 |
| **わざと解放しない（意図的リーク）** | `static Logger * p = new Logger(); return *p;` | デストラクタが**走らない** |

3 番目は「リークでは」と思ったはずです。そのとおりですが、
**プロセス終了時に OS がまとめて回収する**ので実害はありません。
「破棄順序で未定義動作を踏む」よりずっとましだ、という判断です。
Meyers Singleton の提唱者本人も、この形（Nifty Counter や意図的リーク）を状況に応じて挙げています。

**採用の目安**: デストラクタで本当にやることがある（ファイルを閉じる、ハードウェアを止める）なら
1 番か 2 番。デストラクタが実質空なら 3 番でよいです。
**「デストラクタで何をするか」を先に決めてから選んでください。**

## 5.7 テスタビリティ — 状態がテストに漏れる

シングルトンの一番実務的な害はここです。

```cpp
TEST(UartTest, ボーレートを設定できる)
{
  UartPort::instance().set_baud_rate(9600);
  EXPECT_EQ(UartPort::instance().baud_rate(), 9600u);
}

TEST(UartTest, 既定のボーレートは115200)
{
  EXPECT_EQ(UartPort::instance().baud_rate(), 115200u);   // 落ちる
}
```

2 つ目が落ちます。1 つ目が書いた 9600 が残っているからです。
**テストが独立していません。** 実行順を変えると結果が変わります。
gtest は `--gtest_shuffle` で順序を混ぜられるので、これは CI で不定期に落ちます。

手はざっくり 2 つです。

### 手1: `reset()` を用意する（対症療法）

```cpp
class UartPortTest : public ::testing::Test
{
protected:
  void SetUp() override { UartPort::instance().reset(); }
};
```

課題ではこちらを実装します。ただし **`reset()` は本番コードに要らない関数**です。
「テストのために公開 API が 1 つ増える」という代償を払っています。
さらに、`reset()` を書き忘れたメンバがあると漏れは残ります。

### 手2: そもそもインタフェースを切って参照で渡す（本命）

```cpp
class ISerialPort
{
public:
  virtual ~ISerialPort() = default;
  virtual void write_line(const std::string & line) = 0;
};

// 使う側はシングルトンを知らない
void report_status(ISerialPort & port, int battery_mv);
```

本番では `report_status(UartPort::instance(), mv);`、
テストでは `FakeSerialPort` を渡します。**`reset()` は要りません。**
テストごとに新しい `FakeSerialPort` を作るだけです。

ただし [0. 使う前に](00_使う前に.md) の 0.1 に戻ってください。
**実装が 2 つ（本番とフェイク）あるので、これは正当な抽象化です。**
「テスト用に差し替えたい」という理由を書けるなら入れてよい、というのがあの節の条件でした。

**まとめると**: シングルトンにするのは「唯一のインスタンスを配る仕組み」だけにして、
**使う側はインタフェース越しに受け取る**。これが両立の形です。

## 5.8 マイコンでの結論

**ペリフェラルは実質シングルトンです。使ってよい。ただし 4 つ守ってください。**

### 1. ヒープを使わない

```cpp
// 悪い: ヒープを使う。malloc が要る。失敗しうる
Uart & Uart::instance()
{
  static Uart * port = new Uart();
  return *port;
}

// 良い: .bss に置かれる。ヒープゼロ
Uart & Uart::instance()
{
  static Uart the_uart;
  return the_uart;
}
```

5.6 の「意図的リーク」はマイコンでは基本的に採りません。
そもそもプロセスが終了しないので、破棄順序の問題自体が起きないからです。

なお関数ローカル static には**ガード変数**（初期化済みフラグ）が付き、
毎回の呼び出しで 1 回分岐が入ります。本当に削りたい場合は
`constexpr` コンストラクタにして名前空間スコープに置く（定数初期化なので順序問題が起きない）という手があります。
ただし**まず測ってから**にしてください。分岐 1 つです。

### 2. コンストラクタでハードウェアを触らない

これが一番やられます。

```cpp
// 悪い
Uart::Uart()
{
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;   // クロックはもう有効か？
  USART1->CR1 |= USART_CR1_UE;
  NVIC_EnableIRQ(USART1_IRQn);            // 割り込みが今すぐ飛んでくる
}
```

`instance()` を最初に呼ぶのがいつかは、**呼ぶ側次第**です。
クロック設定前かもしれません。割り込みを有効にした瞬間に ISR が走り、
その ISR が `Uart::instance()` を呼ぶと、**まだ構築中の関数ローカル static に再入します**。
（規格上、初期化中の再入は未定義動作です。）

```cpp
// 良い: 構築とハードウェア初期化を分ける
class Uart
{
public:
  static Uart & instance()
  {
    static Uart the_uart;
    return the_uart;
  }

  void open(std::uint32_t baud_rate);   // ここで初めてレジスタを触る
  void close();

private:
  Uart() = default;                     // メモリ上の状態を作るだけ
};
```

`main` の先頭で、クロック設定のあとに `Uart::instance().open(115200);` を呼びます。
**「いつ初期化されるか」を人間が決められる形**に戻しました。

### 3. レジスタは `volatile`。ただしシングルトンとは別の話

```cpp
volatile std::uint32_t * const uart_dr =
  reinterpret_cast<volatile std::uint32_t *>(0x40011004);
```

`volatile` が言うのは「コンパイラよ、このアクセスを消したり並べ替えたりするな」だけです。
**`volatile` はスレッド安全でも割り込み安全でもありません。**
ここを混同するのが定番の事故です。同期が要るなら 4 番です。

### 4. ISR から触るならデータ競合を考える

```cpp
void USART1_IRQHandler()
{
  Uart::instance().on_rx_byte(...);      // メインループも触っている
}
```

`instance()` が返すのは 1 個のオブジェクトです。
**メインループと ISR が同じメンバを触れば競合します。**

| 状況 | 手 |
| --- | --- |
| 1 バイトのフラグを立てるだけ | `std::atomic<bool>`（ロックフリーであることを確認） |
| リングバッファ（書き手 1・読み手 1） | インデックスを `std::atomic` にした SPSC キュー |
| それ以上 | クリティカルセクション（該当割り込みだけ一時禁止） |

割り込みを全部禁止するのは最後の手段です。制御周期に直接効きます。

**`std::mutex` はマイコンでは基本使えません**（OS が要る、ISR から取れない）。
5.3 で「マジックスタティックがスレッドセーフ」と書きましたが、
これは初期化だけの話で、**ISR との競合は別に自分で守る**必要があります。

## 5.9 ROS 2 での結論（補足）

**避けられます。避けてください。**

rclcpp のノードはオブジェクトです。設定はパラメータで、依存はコンストラクタで渡せます。
「1 個しかない」ものは実行時に決まる話であって、型で強制するものではありません。

```cpp
// 悪い: ノードをシングルトンにする
auto & node = MyNode::instance();

// 良い: 普通に作って渡す
auto node = std::make_shared<MyNode>();
```

コンポーネント（`rclcpp_components`）では**同じプロセスに同じノードが 2 つ載る**ことが普通にあります。
シングルトンにしていると、その瞬間に破綻します。

一方 `rclcpp::init()` / `shutdown()` が触るグローバルなコンテキストは、実質シングルトンです。
ただし rclcpp はこれを `rclcpp::Context` というオブジェクトとして公開していて、
**複数コンテキストを作れる**ようにしてあります。既定のものが 1 つあるだけです。
**「1 個で足りるが 1 個に強制はしない」** という設計で、参考になります。

## 5.10 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| 設定したはずの値が反映されない | `auto config = Config::instance();` でコピーしている。`auto &` にする。`= delete` を書けばコンパイルエラーになる |
| ある値が 0 のまま。リンク順を変えると直る | static initialization order fiasco。5.3。Meyers Singleton にする |
| プログラム終了時だけ落ちる / 変な値が出る | 破棄順序。5.6。デストラクタで他のシングルトンを使っている |
| テストを単体で回すと通るが、全部回すと落ちる | 前のテストの状態が漏れている。`SetUp()` で `reset()` |
| `--gtest_shuffle` で不定期に落ちる | 同上。テストがシングルトンの状態に依存している |
| `error: call to deleted constructor` | 値で受けている。`Config &` で受ける |
| `instance()` の中で `std::mutex` を使っていて遅い | C++11 以降は不要。関数ローカル static だけでスレッドセーフ |
| マイコンで起動直後にハングする | コンストラクタでハードウェアを初期化している。`open()` に分ける |
| ISR から触ると値が化ける | 初期化はスレッドセーフでも、**中身の競合は自分で守る** |

## 5.11 対応する課題

```bash
./drill run dp05
```

`exercises/dp05_singleton/src/uart_port.cpp` に、

1. `UartPort::instance()` — Meyers Singleton
2. `UartPort::construction_count()` — 初期化が一度だけであることを見せる
3. `UartPort::reset()` — **オブジェクトを作り直さず状態だけ戻す**
4. `LazyProbe::instance()` / `LazyProbe::was_constructed()` — 遅延初期化の確認

を実装します。テストは、同じアドレスが返ること、コピー・ムーブが
`static_assert` で禁止されていること、コンストラクタが 1 回しか走らないこと、
`reset()` でテスト間の状態が切れることを見ます。

## 5.12 この章のまとめ

- **まず引数で渡せないか考える。** シングルトンはグローバル変数の言い換えになりやすい
- 使ってよいのは **「2 個目を構築できたら壊れるもの」** だけ。マイコンのペリフェラルがそれ
- Java の `private static` フィールドをそのまま C++ のグローバルにすると、
  **翻訳ユニットをまたいだ初期化順序が未定義**。リンク順で結果が変わる
- 標準解は **Meyers Singleton**（関数ローカル static）。使う瞬間に初期化されるので順序問題が消える
- **C++11 以降、関数ローカル static の初期化はスレッドセーフ。** 自分でロックを書かない。
  ただし**守られるのは初期化だけ**
- **コピー・ムーブを `= delete` する。** 書かないと `auto config = Config::instance();` で複製される。
  Java では起きない事故
- 破棄順序も未定義。デストラクタで他のシングルトンを使わない。
  やることが無いなら**意図的リーク**も選択肢
- シングルトンは**状態がテスト間で漏れる**。`reset()` は対症療法で、
  本命は**インタフェースを切って参照で渡す**こと
- マイコン: ヒープを使わない / コンストラクタでハードウェアを触らない / ISR との競合は別途守る
- ROS 2: 避けられる。コンポーネントで同じノードが 2 つ載ると破綻する

---

前: [4. Factory Method](04_FactoryMethod.md) ／ 次: 6. Prototype（準備中）
