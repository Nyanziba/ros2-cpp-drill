# 19. State

> **結城本 第19章 対応。** `State` インタフェースと `DayState` / `NightState`、
> そして `Context` としての `SafeFrame` を手元に開いてください。
>
> **この章のねらい**: Java 版の `State` は `doClock(Context context, int hour)` の中で
> `context.changeState(NightState.getInstance())` を呼びます。
> **これを C++ でそのまま書くと、`this` が破棄されたあとにメンバ関数の続きが走ります。**
> 実際に SIGSEGV を出して確かめます。そのうえで、C++ には状態機械の書き方が
> `enum` + `switch` / State クラス / `std::variant` の **3 つ**あるので、
> 全部書いて同じ遷移列が出ることを確認し、どれを選ぶかを決めます。
> 結論を先に言うと、**ロボットの状態機械は `enum` + `switch` で足りることが多い**です。

## 19.1 Java 版をそのまま C++ にすると

結城本の `State` インタフェースはこうです。

```java
public interface State {
    public abstract void doClock(Context context, int hour);
    public abstract void doUse(Context context);
}
```

C++ に素直に移すとこうなります。

```cpp
class Context;

class State
{
public:
  virtual ~State() = default;                         // 変更点1
  virtual void do_clock(Context & context, int hour) = 0;
  virtual void do_use(Context & context) = 0;
};
```

### 変更点1: 仮想デストラクタ

第1章と同じです。基底のポインタで破棄する以上、無いと未定義動作です。
この講習の全章で守ります。

### 変更点2: `getInstance()` は Java の static フィールドではなく**関数内 static**

結城本の `DayState` はシングルトンです。

```java
public class DayState implements State {
    private static DayState singleton = new DayState();
    public static State getInstance() { return singleton; }
}
```

C++ で `static DayState singleton;` をクラスの static メンバとして書くと、
**翻訳単位をまたいだ初期化順序が未定義**という第5章の地雷を踏みます。
関数内 static にします。

```cpp
const State * day_state()
{
  static const DayStateObject instance;   // 初めて呼ばれたときに 1 回だけ初期化される
  return &instance;
}
```

### 変更点3: **状態オブジェクトが状態を持たないなら、`unique_ptr` は要らない**

これが Java 版との一番大きな設計差です。19.2 で扱います。

### 変更点4: `doClock(context, hour)` の `context` を**消す**

Java 版は状態オブジェクトが `context` を受け取り、自分で `context.changeState(...)` を呼びます。
**C++ ではこれをやりません。** 19.3 が本章の核心です。

## 19.2 誰が状態オブジェクトを所有するのか

Java 版の `Context` は `State state` を持ち替えるだけです。GC が面倒を見ます。
C++ では「持ち替える」の意味を決めなければいけません。判断は 1 点だけです。

> **状態オブジェクトはメンバ変数を持つか。**

| 状態オブジェクトが | 保持の仕方 | ヒープ確保 |
| --- | --- | --- |
| メンバを 1 つも持たない | `const State *` を `static` 実体に向ける | **ゼロ** |
| 状態ごとに固有のデータを持つ | `std::unique_ptr<State>` を差し替える | 遷移のたびに 1 回 |

結城本の `DayState` / `NightState` は**メンバを 1 つも持ちません**。だから
Java 版もシングルトンにしています。C++ でも同じ判断が使えます。

```cpp
class ClassStateMachine
{
public:
  MachineState state() const { return current_->id(); }

private:
  const State * current_ = state_object(MachineState::Stopped);   // ただのポインタ
};
```

`state_object()` は 4 つの `static` 実体のどれかを返すだけなので、
**遷移は「ポインタの代入 1 回」**です。ロボットの制御ループの中で
`make_unique` が走らないのは、これだけで価値があります。

逆に、状態ごとにデータを持たせたくなったら（走行状態だけデューティを持つ、など）
`unique_ptr` の差し替えになります。**そうなった時点で `std::variant` を検討してください**（19.5）。

## 19.3 この章で最も重要な落とし穴 — 遷移中に自分自身を差し替える

Java 版の `doClock` は、状態オブジェクト自身が `Context` を書き換えます。

```java
public void doClock(Context context, int hour) {
    if (hour < 9 || 17 <= hour) {
        context.changeState(NightState.getInstance());
    }
}
```

Java ではこれで問題ありません。`DayState` はシングルトンなので誰も破棄しませんし、
仮に破棄対象でも GC が参照が残っている間は生かします。

**C++ で状態オブジェクトを `unique_ptr` で持つと、この行が `delete this` と同じ意味になります。**

```cpp
void Running::handle(Context & context) override
{
  context.set_state(std::make_unique<Faulted>());  // ここで this が delete される
  std::cout << "退場処理: " << name() << "\n";     // 既に死んだ this のメンバ関数
  ticks_ = ticks_ + 1;                             // 既に解放されたメモリへの書き込み
  std::cout << "ticks=" << ticks_ << "\n";
}
```

`context.set_state()` の中で古い `unique_ptr` が破棄され、`this` が消えます。
そのあとの 3 行は**解放済みメモリの上**で走ります。

これを 1 ファイルにして実行した結果です。

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic try_uaf.cpp -o uaf && ./uaf
echo $?
```

```
139
```

**警告は 1 つも出ません。**`-Wall -Wextra -Wpedantic` は素通りします。
出力も 1 行も出ずに SIGSEGV（128 + 11 = 139）で落ちます。
`std::cout` に積んだ文字がフラッシュされる前に死ぬからです。

たちが悪いのは、**状態オブジェクトが小さくてメンバが無いと、たまたま動いてしまう**ことです。
上の例から `ticks_` を消せば、多くの環境で「動いているように見える」コードになります。
メンバを 1 つ足した日に、無関係に見える場所で落ちます。

### 対処: 遷移を戻り値で返し、差し替えは呼び出し側がやる

`handle()` から `Context` を消します。**状態オブジェクトに Context を渡さなければ、
自分を差し替えるという操作がそもそも書けません。**

```cpp
class State
{
public:
  virtual ~State() = default;
  virtual MachineState id() const = 0;

  /// 遷移先の State を返す。遷移しないなら this を返すこと。Context には触れません。
  virtual const State * handle(MachineEvent event) const = 0;

  virtual void on_enter(TransitionLog & log) const;
  virtual void on_exit(TransitionLog & log) const;
};
```

Context 側はこうなります。

```cpp
bool ClassStateMachine::handle(MachineEvent event)
{
  const State * const next = current_->handle(event);   // まず受け取る
  if (next == current_) {
    return false;                                       // 遷移しないなら何もしない
  }

  current_->on_exit(log_);    // 古い状態はまだ生きている
  current_ = next;            // ここで初めて差し替える
  current_->on_enter(log_);
  return true;
}
```

**「差し替えるのは呼び出し側だけ」**という規約を、規約ではなく**型**で表明しました。
`handle()` が `const` メンバ関数なのも同じ理由です。自分の中身も書き換えません。

`unique_ptr` で状態を持つ設計にしても、この形なら安全です。

```cpp
std::unique_ptr<State> next = current_->handle(event);  // 別実体を作って返す
current_->on_exit(log_);
current_ = std::move(next);   // 古い実体はこの行で破棄される。もう誰も使っていない
current_->on_enter(log_);
```

**これが C++ 版 State の設計の核です。** Java 版の写経ではここが逆になります。

## 19.4 入場・退場アクション — ロボットでは必須

`on_enter` / `on_exit` は結城本には出てきませんが、実機では**無いと危険**です。

```
走行 --非常停止--> 異常停止
```

この遷移で、**モータを止めてから状態を抜ける**必要があります。
遷移先ごとに「ここでもモータを止める」と書いていくと、遷移が増えるたびに書き漏れます。
**Running から出るときは必ず止める**を 1 か所に書きます。

```cpp
class RunningStateObject : public State
{
public:
  void on_exit(TransitionLog & log) const override
  {
    State::on_exit(log);        // 基底の既定動作
    log.record("motor:stop");   // 実機ではここでモータ出力を 0 にする
  }
};
```

順序は**退場が先、入場が後**で固定します。逆にすると、
「新しい状態のブレーキをかけた直後に、古い状態のモータ停止が走る」という並びになります。

もう 1 つ決めておくことがあります。**遷移しない入力ではアクションを走らせない**、です。

```cpp
if (next == current_) {
  return false;      // on_exit も on_enter も呼ばない
}
```

`Running` で `Start` をもう一度受けたときに `on_exit` → `on_enter` が走ると、
**モータが一瞬止まって再起動します**。実機では分かりやすく事故ります。
「自己遷移」を意図的に使いたい場合だけ、別の API（`force_reenter()` など）にしてください。

## 19.5 標準ライブラリ／言語機能に同じものが無いか

State パターンそのものは標準ライブラリにありません。
ただし **`std::variant` + `std::visit` が、State の「状態ごとに型が違う」という要求を
継承なしで満たします。**

```cpp
struct StoppedState {};
struct IdleState {};
struct RunningState { std::uint8_t duty_percent = 60; };   // 状態固有のデータ
struct FaultedState { MachineEvent cause; };

using StateVariant = std::variant<StoppedState, IdleState, RunningState, FaultedState>;
```

遷移も「戻り値で返す」形で書きます。

```cpp
std::optional<StateVariant> VariantStateMachine::next_state(
  const StateVariant & current, MachineEvent event)
{
  return std::visit(
    [event](const auto & concrete) -> std::optional<StateVariant> {
      using T = std::decay_t<decltype(concrete)>;
      if constexpr (std::is_same_v<T, IdleState>) {
        if (event == MachineEvent::Start) {
          return StateVariant{RunningState{kCruiseDutyPercent}};
        }
      }
      // ... 他の状態
      return std::nullopt;      // 遷移しない
    },
    current);
}
```

得られるもの:

- **ヒープ確保ゼロ**。`variant` は中身を自分の中に置きます
- **vtable ゼロ**。`std::is_polymorphic_v<StoppedState>` は `false`
- **状態ごとに固有のデータを持てる**。State クラス版で `unique_ptr` が要る場面がこれで消える
- **書き漏れをコンパイラが見る**。状態の型を足すと `visit` のラムダが全型を扱えず落ちる

失うもの:

- **状態の種類がコンパイル時に固定**される。実行時にプラグインで状態を足す、はできません
- `variant` の型リストが長くなると、エラーメッセージが読みづらくなります

### 遷移をコンパイル時に検査する

`variant` 版まで来たなら、もう一歩進めて**許されない遷移を型で弾く**ことができます。

```cpp
template <typename From, Ev E>
struct Transition;                                          // 宣言のみ。定義しない

template <> struct Transition<Stopped, Ev::PowerOn> { using To = Idle; };
template <> struct Transition<Idle,    Ev::Start>   { using To = Running; };
template <> struct Transition<Running, Ev::Stop>    { using To = Idle; };
template <> struct Transition<Faulted, Ev::Reset>   { using To = Stopped; };
template <typename From> struct Transition<From, Ev::EStop> { using To = Faulted; };

template <Ev E, typename From>
typename Transition<From, E>::To go(const From &)
{
  return typename Transition<From, E>::To{};
}
```

許された遷移だけが書けます。

```cpp
Stopped stopped;
auto idle    = go<Ev::PowerOn>(stopped);
auto running = go<Ev::Start>(idle);
auto faulted = go<Ev::EStop>(running);
auto back    = go<Ev::Reset>(faulted);

auto bad = go<Ev::Start>(faulted);   // 異常状態から走行へ。書けない
```

最後の 1 行のコンパイルエラー（実際の出力）:

```
error: no matching function for call to 'go'
   43 |   auto bad = go<Ev::Start>(faulted);
      |              ^~~~~~~~~~~~~
note: candidate template ignored: substitution failure [with E = Ev::Start,
      From = typename Transition<Running, (Ev)3>::To]:
      implicit instantiation of undefined template 'Transition<Faulted, Ev::Start>'
```

**「異常停止から走行に入るコードは、コンパイルが通らない」**が実現できました。
ただし現実の状態機械は、遷移のきっかけがセンサ値や通信で**実行時に決まります**。
そのときは `go<...>` を呼ぶ側で `switch` が要り、結局実行時の判定に戻ります。
この技法が効くのは、**遷移の並びがコードに直書きされている手順**
（キャリブレーション手順、起動シーケンスなど）です。そこでは強力です。
状態機械全体をこれで書こうとしないでください。

## 19.6 3 つの手段の比較

同じ遷移規則を 3 通りに書き、同じイベント列を流して**同じログが出る**ことを課題で確認します。

| 手段 | 状態の表現 | ヒープ | vtable | 状態ごとのデータ | 実行時に種類を足せるか |
| --- | --- | --- | --- | --- | --- |
| `enum` + `switch` | 値（1 バイト） | ゼロ | ゼロ | 持てない | 不可 |
| State クラス（GoF） | 派生クラス | 状態が空ならゼロ | あり | 持てる | **可能** |
| `std::variant` + `visit` | 直和型 | ゼロ | ゼロ | 持てる | 不可 |

課題のヘッダで実測したサイズです（Apple clang, arm64）。

```
MachineState=1  StateVariant=8
```

**選び方は次の 1 問で決まります。**

> **状態ごとに固有のデータがあるか。**

- **無い** → `enum` + `switch`。ロボットの状態機械はここに落ちることが多い
- **ある。種類は固定** → `std::variant`
- **ある。種類を実行時に差し替えたい**（プラグイン、シナリオ読み込み） → State クラス

「State パターンを習ったから State クラスで書く」は、この章がいちばん警戒してほしい動きです。
[0. 使う前に](00_使う前に.md) の「実装が 1 つしかないのに抽象化する」と同じ話が、
状態の数だけ増幅されます。**4 状態のために 4 ファイルと 4 クラスと vtable を増やす前に、
`switch` 1 個で足りないかを見てください。**

`switch` を書くときは `default:` を書かないでください。
全 `enum` 値を列挙しておくと、**状態を足した日にコンパイラが書き漏れを教えてくれます**。
`default:` を書くとその警告が消えます。これは `switch` 版の大きな取り柄です。

## 19.7 手元で試す

19.3 の SIGSEGV を自分の手で出してください。**この章はこれが本体です。**

```cpp
#include <iostream>
#include <memory>

class Context;

class State
{
public:
  virtual ~State() = default;
  virtual const char * name() const = 0;
  virtual void handle(Context & context) = 0;
};

class Context
{
public:
  Context();
  void set_state(std::unique_ptr<State> next) { state_ = std::move(next); }
  void request() { state_->handle(*this); }

private:
  std::unique_ptr<State> state_;
};

class Faulted : public State
{
public:
  const char * name() const override { return "Faulted"; }
  void handle(Context &) override {}
};

class Running : public State
{
public:
  const char * name() const override { return "Running"; }

  void handle(Context & context) override
  {
    context.set_state(std::make_unique<Faulted>());  // ここで this が delete される
    std::cout << "退場処理: " << name() << "\n";
    ticks_ = ticks_ + 1;
    std::cout << "ticks=" << ticks_ << "\n";
  }

private:
  int ticks_ = 0;
};

Context::Context()
: state_(std::make_unique<Running>())
{
}

int main()
{
  Context context;
  context.request();
  std::cout << "done\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try; echo $?
```

<details>
<summary>予想: 何が出力されるか。警告は何個出るか</summary>

**警告は 0 個です。** そして出力も 0 行で落ちます。

```
139
```

（`139` は `echo $?` の値。128 + SIGSEGV(11)。手元の Apple clang, arm64 で 3 回とも同じでした）

`std::cout` に積んだ「退場処理: ...」はフラッシュ前に死ぬので出ません。
**「出力が途中で切れる」のではなく「何も出ない」**のがこの手のバグの見え方です。
ログを頼りに原因を探すと、`handle()` に入ったことすら分からず迷います。

`ticks_ = ticks_ + 1;` を消すと、環境によっては**動いているように見えます**。
それが一番危険な状態です。`this` が死んでいる事実は変わりません。

対処は 19.3 の「遷移先を戻り値で返す」です。`handle()` から `Context &` を消せば、
このコードは**そもそも書けなくなります**。
</details>

## 19.8 マイコンでの結論

**`enum` + テーブル駆動**です。ヒープゼロ、vtable ゼロ、例外なし。
遷移表を `constexpr` 配列で持つと、状態機械が RAM 1 バイトになります。

```cpp
#include <cstddef>
#include <cstdint>
#include <cstdio>

enum class St : std::uint8_t { Stopped, Idle, Running, Faulted, Count };
enum class Ev : std::uint8_t { PowerOn, Start, Stop, EStop, Reset, Count };

constexpr std::size_t kStates = static_cast<std::size_t>(St::Count);
constexpr std::size_t kEvents = static_cast<std::size_t>(Ev::Count);

// [現在の状態][イベント] = 遷移先。自分自身なら「無視」。
constexpr St kTable[kStates][kEvents] = {
  //            PowerOn      Start        Stop         EStop        Reset
  /* Stopped */ {St::Idle,    St::Stopped, St::Stopped, St::Faulted, St::Stopped},
  /* Idle    */ {St::Idle,    St::Running, St::Idle,    St::Faulted, St::Idle},
  /* Running */ {St::Running, St::Running, St::Idle,    St::Faulted, St::Running},
  /* Faulted */ {St::Faulted, St::Faulted, St::Faulted, St::Faulted, St::Stopped},
};

// 入場/退場アクションも表で持てる。関数ポインタは C 言語編 9 章と同じ道具。
using Action = void (*)();

void motor_stop() { std::printf("motor:stop\n"); }
void brake_engage() { std::printf("brake:engage\n"); }
void nothing() {}

constexpr Action kOnExit[kStates]  = {nothing, nothing, motor_stop, nothing};
constexpr Action kOnEnter[kStates] = {nothing, nothing, nothing, brake_engage};

class Machine
{
public:
  bool handle(Ev event)
  {
    const St next = kTable[static_cast<std::size_t>(state_)][static_cast<std::size_t>(event)];
    if (next == state_) {
      return false;
    }
    kOnExit[static_cast<std::size_t>(state_)]();
    state_ = next;
    kOnEnter[static_cast<std::size_t>(state_)]();
    return true;
  }

  St state() const { return state_; }

private:
  St state_ = St::Stopped;
};

// 表そのものをコンパイル時に検査できる。
static_assert(kTable[static_cast<std::size_t>(St::Faulted)][static_cast<std::size_t>(Ev::Start)] ==
                St::Faulted,
              "異常状態は Reset 以外で抜けてはいけない");
static_assert(sizeof(Machine) == 1, "状態機械は 1 バイト");
```

`-fno-exceptions -fno-rtti` を付けてビルドし、`Start / PowerOn / Start / EStop / Start / Reset`
を流した実際の出力です。

```
--- Start
(無視)
--- PowerOn
exit:Stopped
enter:Idle
--- Start
exit:Idle
enter:Running
--- EStop
exit:Running
motor:stop
enter:Faulted
brake:engage
--- Start
(無視)
--- Reset
exit:Faulted
enter:Stopped
sizeof(Machine)=1 sizeof(kTable)=20
```

**RAM 1 バイト、ROM 20 バイト（+ 関数ポインタ表）。** 動的確保も vtable もありません。

テーブル駆動のもう 1 つの利点は、**遷移規則が「表」として一望できる**ことです。
「非常停止の列が全部 `Faulted` になっているか」を目で確認できます。
State クラス版だと 4 つのファイルに散って、確認に 4 か所を見に行くことになります。

入場/退場アクションを関数ポインタの表で持つ部分は、
C言語編の「9. 関数ポインタ」とまったく同じ道具です。
**GoF の State は、C の「関数ポインタの表」に vtable という名前を付けたものだ**と思えば、
マイコンでどちらを選ぶかの判断が付きやすくなります。

State クラス版をマイコンで使うのは、**状態を実行時に差し替える必要が本当にあるとき**だけです。
その場合も状態オブジェクトは `static` 実体にして、`unique_ptr` は使わないでください。

## 19.9 ROS 2 での結論（補足）

ROS 2 のライフサイクルノード（`rclcpp_lifecycle::LifecycleNode`）は、
**State パターンの完成品**が標準で用意されています。

```
Unconfigured -> Inactive -> Active -> Inactive -> Finalized
```

遷移は `on_configure()` / `on_activate()` / `on_deactivate()` / `on_cleanup()` という
**入場・退場アクションのコールバック**として書きます。19.4 でやったことそのものです。
戻り値で成否を返す形（`CallbackReturn::SUCCESS` / `FAILURE`）なのも、
19.3 の「差し替えは呼び出し側」と同じ発想です。

**ノード自体の起動・停止状態を自作の状態機械で管理しようとしないでください。**
そこは既に用意されています。自作するのは「ロボットの動作状態」の方です。

ノードの中の動作状態については、`enum` + `switch` で書いて
`std_msgs::msg::String` か自作メッセージで publish するのが実務では一番読みやすくなります。
ROS 2 側では動的確保も例外も自由なので、State クラス版を選んでもコスト面の問題はありません。
**それでも `enum` を勧めるのは、遷移規則が 1 か所にまとまるからです。**

## 19.10 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| 遷移した直後に落ちる。ログは何も出ていない | `handle()` の中で自分自身を差し替えている（19.3） |
| 状態オブジェクトにメンバを足した日から落ち始めた | 同上。メンバが無いうちは「たまたま」動いていた |
| モータが一瞬止まって再起動する | 遷移しない入力で `on_exit` → `on_enter` を走らせている（19.4） |
| ブレーキがかかった後にモータ停止が走る | 入場と退場の順序が逆。**退場が先** |
| 状態を 1 つ足したら、書き漏れに気づかず出荷した | `switch` に `default:` を書いている。消せば警告が出る |
| 制御ループで確保が走る | 状態を `unique_ptr` で持っている。状態が空なら `static` 実体で足りる（19.2） |
| `std::visit` のラムダがコンパイルエラー | 分岐ごとに戻り値の型が違う。戻り値型を明示する（`-> std::optional<StateVariant>`） |
| 状態が増えて State クラスが 10 個になった | 状態ごとにデータが無いなら `enum` に戻す（19.6） |

## 19.11 対応する課題

```bash
./drill run dp19
```

題材はロボットの動作状態です。

```
Stopped --PowerOn--> Idle --Start--> Running --Stop--> Idle
どの状態からでも EmergencyStop --> Faulted
Faulted --Reset--> Stopped （手動リセットでしか抜けられない）
表に無い組み合わせは無視。状態は変わらず、入場/退場アクションも走らない
```

`exercises/dp19_state/src/state_machine.cpp` に、**同じ規則を 3 通り**実装します。

1. `EnumStateMachine::next_state()` / `handle()` — `enum` + `switch`
2. 4 つの `State` 派生クラスの `handle()` と `state_object()`、`ClassStateMachine::handle()` — GoF 版。
   `RunningStateObject::on_exit()` と `FaultedStateObject::on_enter()` を override
3. `id_of()` / `VariantStateMachine::next_state()` / `handle()` — `std::variant` + `std::visit`

テストが見るもの（13 個）:

- 全 4 状態 × 全 5 イベント（20 通り）が遷移表どおりであること
- 許されない入力で状態が変わらず、`handle()` が `false` を返し、**アクションも走らない**こと
- 入場・退場が正しい順序で走ること（`exit:Running` → `motor:stop` → `enter:Faulted` → `brake:engage`）
- **遷移しても遷移元の状態オブジェクトが生きていること**（19.3 の安全設計の検証）。
  `State::handle` の型が `const State * (State::*)(MachineEvent) const` であることも `static_assert` で見ます
- `variant` 版が非多態であること（`static_assert(!std::is_polymorphic_v<...>)`）
- **3 つの実装が同じ遷移列・同じ戻り値・同じログを返すこと**

## 19.12 この章のまとめ

- Java 版の `context.changeState(...)` を `handle()` の中で呼ぶ形は、**C++ では `delete this` になる**。
  実測で SIGSEGV。警告は 0 個
- 対処は**遷移先を戻り値で返し、差し替えは呼び出し側だけがやる**。
  `handle()` から `Context` を消せば、危険なコードは**そもそも書けない**
- 状態オブジェクトが**メンバを持たないなら `static` 実体へのポインタ**で足りる。ヒープ確保ゼロ
- 入場・退場アクションはロボットでは必須。**退場が先、入場が後**。
  遷移しない入力ではどちらも走らせない
- C++ には状態機械の手段が 3 つある。**選ぶ基準は「状態ごとに固有のデータがあるか」**
- **無いなら `enum` + `switch`。ロボットの状態機械はたいていこれで足りる**。
  `default:` を書かないと、状態を足したときの書き漏れをコンパイラが教えてくれる
- `std::variant` は「状態ごとにデータがあり、種類が固定」のときの最適解。ヒープも vtable もゼロ
- マイコンは `constexpr` の遷移表。RAM 1 バイト。C の関数ポインタ表と同じもの
- ROS 2 のライフサイクルノードは State パターンの完成品。ノードの起動状態は自作しない

---

前: [18. Memento](18_Memento.md) ／ 次: 20. Flyweight（準備中）
