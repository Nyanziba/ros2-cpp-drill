# 22. Command

> **結城本 第22章 対応。** `Command` / `MacroCommand` / `DrawCommand` と、
> `MacroCommand.undo()` のところを手元に開いてください。
>
> **この章のねらい**: C++ には既に Command があります。**`std::function<void()>` です。**
> クラスも継承も要りません。ただし `std::function` には **undo が書けません**。
> 「実行」と「取り消し」という 2 つの操作が要るから、そこで初めてクラスにします。
> この判断基準を先に立てたうえで、undo / redo とマクロコマンドを書き、
> 最後に **`std::function` も `std::vector` も使えないマイコンで同じことをやります**。

## 22.1 Java 版をそのまま C++ にすると

結城本の `Command` インタフェースはこうです。

```java
public interface Command {
    public abstract void execute();
}
```

C++ に素直に移すとこうなります。

```cpp
class Command
{
public:
  virtual ~Command() = default;
  virtual void execute() = 0;
};
```

変更点は 1 つ、**仮想デストラクタ**だけです。第1章から同じです。
`std::unique_ptr<Command>` で持って破棄するので、これが無いと派生のデストラクタが呼ばれません。

問題は次の `MacroCommand` です。結城本はこう書いています。

```java
public class MacroCommand implements Command {
    private Deque<Command> commands = new ArrayDeque<>();
    public void append(Command cmd) { commands.push(cmd); }
}
```

C++ に移すときの判断はここに集中します。

### 変更点1: `Deque<Command>` は `std::vector<std::unique_ptr<Command>>` になる

```cpp
std::vector<Command> commands_;                    // コンパイルエラー。抽象クラスは値で持てない
std::vector<Command *> commands_;                  // 誰が delete する？
std::vector<std::unique_ptr<Command>> commands_;   // これ
```

1 行目は `Command` が純粋仮想なのでそもそも通りません。仮に具象型にしたとしても、
`std::vector<Command>` に派生クラスを入れると**スライシング**して派生部分が消えます。

2 行目は動きますが、**所有権が型に書かれていません**。
結城本の `MacroCommand` は `append` された `Command` を保持し続けます。
C++ では「保持し続ける = 所有する」なので、`unique_ptr` で書きます。

**Command パターンは Composite（第11章）とほぼ同じ構造です。**
`MacroCommand` は `Command` を実装しつつ `Command` を持ちます。
所有権の議論も第11章とそのまま同じです。

### 変更点2: `append(Command cmd)` は `add(std::unique_ptr<Command> command)` になる

引数の受け取り方が 3 通りあって、正解は 1 つです。

```cpp
void add(std::unique_ptr<Command> command);          // 値で受ける。これ
void add(const std::unique_ptr<Command> & command);  // コピーできないので中で move も push もできない
void add(std::unique_ptr<Command> && command);       // 動くが、呼び出し側が常に std::move を書く必要がある
```

**値で受けて `std::move` して積む**のが `unique_ptr` の作法です。
これで「呼んだ側は所有権を手放した」が呼び出し箇所の見た目（`std::move(cmd)`）に出ます。

### 変更点3: 受け手（Receiver）は所有しない

`RotateCommand` はロボットアームを操作しますが、**アームを所有しません**。
アームは他の誰かのものです。

```cpp
class RotateCommand : public Command
{
public:
  RotateCommand(RobotArm & arm, double delta_deg) : arm_(&arm), delta_deg_(delta_deg) {}
private:
  RobotArm * arm_;      // 所有しない。生ポインタなのは「所有しない」の表明
  double delta_deg_;
};
```

`unique_ptr` を持つと所有してしまい、`shared_ptr` を持つと寿命が延びてしまいます。
**所有しないなら生ポインタか参照です。** ただし代償があります（22.5）。

## 22.2 まず疑う — undo が要らないなら `std::function` で足りる

ここが C++ で一番大きい差分です。

Java で Command パターンを使う一番の動機は
「**メソッド呼び出しをオブジェクトとして持ち回りたい**」ことでした。
Java 8 より前は、それをやる手段がクラスしか無かったからです。

C++ には最初から手段があります。

```cpp
std::vector<std::function<void()>> queue;

queue.push_back([&arm]() { arm.rotate(30.0); });
queue.push_back([&arm]() { arm.set_gripper(true); });

for (const auto & action : queue) {
  action();
}
```

**これで Command パターンは完成しています。** `Command` 基底クラスも、
`RotateCommand` も `GripperCommand` も 1 つも書いていません。
「操作を値にして、あとで実行する」という Command の目的は全部果たしています。

では、いつクラスにするのか。

| 要件 | 手段 |
| --- | --- |
| 実行するだけ | `std::function<void()>` |
| 実行 + ログ用の名前が要る | `struct { std::string name; std::function<void()> action; }` |
| **実行 + 取り消し（undo）** | **クラス**。`execute()` と `undo()` の 2 つが要る |
| 実行 + 取り消し + 直列化（保存・通信） | クラス。または POD の構造体（22.8） |

判断基準は 1 行です。

> **`std::function` は操作を 1 つしか包めない。2 つ以上要るならクラス。**

`std::function<void()>` を 2 本持てばいいのでは、と思ったはずです。

```cpp
struct Action
{
  std::function<void()> execute;
  std::function<void()> undo;      // 動くが……
};
```

動きます。ただし `undo` 側のラムダは「実行前の状態」をキャプチャする必要があり、
そのキャプチャをどこでいつ取るかが `execute` の中に隠れます。
2 つの関数が同じ状態を共有するなら、それは**クラスにすべき状態**です。
`std::function` を 2 本並べるより、`execute()` と `undo()` を持つクラスの方が短くなります。

**「クラスにしない選択肢を毎回検討する」のがこの章の 0.5 節（使う前に）への対応です。**
部活のライブラリで Command クラスを 5 個書く前に、undo が本当に要るかを聞いてください。

## 22.3 コマンドキューと、誰が所有するのか

undo が要ると決まったら、キューはこうなります。

```cpp
class CommandHistory
{
public:
  void run(std::unique_ptr<Command> command)
  {
    command->execute();
    undone_.clear();                        // 歴史が分岐したので redo は捨てる
    done_.push_back(std::move(command));
  }

private:
  std::vector<std::unique_ptr<Command>> done_;
  std::vector<std::unique_ptr<Command>> undone_;
};
```

**所有権は履歴が持ちます。** 実行が終わってもコマンドは消せません。
undo するために取っておく必要があるからです。
ここが「実行したら終わり」の `std::function` キューとの構造上の違いです。

マクロコマンドも同じ形です。

```cpp
class MacroCommand : public Command
{
public:
  void add(std::unique_ptr<Command> command) { commands_.push_back(std::move(command)); }

  void execute() override
  {
    for (const auto & command : commands_) { command->execute(); }
  }

  void undo() override
  {
    for (std::size_t i = commands_.size(); i > 0; --i) { commands_[i - 1]->undo(); }
  }

private:
  std::vector<std::unique_ptr<Command>> commands_;
};
```

`undo()` が**逆順**なのが要点です。「掴んでから持ち上げた」を取り消すなら、
先に下ろしてから離します。正順で undo すると、依存のある操作で状態が壊れます。

`MacroCommand` 自身も `Command` なので、**マクロの中にマクロを入れられます**。
これは Composite（第11章）そのもので、`CommandHistory` から見れば
マクロは 1 個のコマンドです。undo 1 回で全部戻ります。

## 22.4 undo と redo — 第18章 Memento との使い分け

第18章で「Undo には 2 通りある」と書きました。ここが 2 つ目です。

| | Memento（第18章） | Command（この章） |
| --- | --- | --- |
| 何を持つか | 操作後の**状態のコピー** | 操作と、その**逆操作** |
| 戻し方 | 状態を書き戻す | 逆操作を実行する |
| 履歴 1 段のコスト | 状態全体のサイズ | コマンドのサイズ（普通は数バイト） |
| 任意の時点に飛べるか | 飛べる | 1 段ずつしか戻れない |
| 状態が巨大なとき | 苦しい | 得意 |

**逆操作が書けるなら Command、書けないなら Memento** です。

### 逆操作が書けないときはどうするか

3 つに分かれます。

**1. 逆操作が自明** — 相対的な操作です。何も保存しません。

```cpp
void RotateCommand::execute() { arm_->rotate(delta_deg_); }
void RotateCommand::undo()    { arm_->rotate(-delta_deg_); }
```

**2. 逆操作は書けるが、実行前の状態が要る** — 絶対値を設定する操作です。
`execute()` の中で直前の値を控えます。**Command の中に小さな Memento を持つ**形です。

```cpp
void GripperCommand::execute()
{
  previous_ = arm_->gripper_closed();   // 実行前を控える
  arm_->set_gripper(closed_);
}
void GripperCommand::undo() { arm_->set_gripper(previous_); }
```

「閉じる」の逆が「開く」ではないことに注意してください。
**もともと閉じていたなら、undo 後も閉じたままが正解です。**
`undo()` に `set_gripper(!closed_)` と書くと、そこがバグになります。

**3. そもそも取り消せない** — モータを回して機体が動いた、EEPROM に書いた、
CAN にフレームを送った。これらは undo できません。**書けないものは書かないでください。**
`undo()` を「何もしない」で実装して黙って通すのが一番危険です。選択肢は 2 つです。

- そのコマンドを履歴に積まない（`run()` ではなく直接 `execute()` する）
- `bool is_undoable() const` を持たせ、`CommandHistory::undo()` がそこで止まる

部活のライブラリなら前者で十分です。**「実機を動かす指令は undo できない」**を
設計として最初に決めておくと、あとで悩みません。

なお、**状態が巨大かつ逆操作が面倒**なら両者の併用が定石です。
N 回に 1 回 Memento を取り、あいだを Command で埋めます。

## 22.5 C++ 固有の危険

### コマンドが受け手より長生きする

第17章 Observer とまったく同じ問題です。

```cpp
CommandHistory history;
{
  RobotArm arm;
  history.run(std::make_unique<RotateCommand>(arm, 30.0));
}                                  // arm がここで死ぬ
history.undo();                    // 死んだアームを触る。未定義動作
```

`RotateCommand` は `RobotArm *` を持っています。
**Java なら GC がアームを生かしておくので落ちません。C++ では落ちます。**
しかも履歴は「実行が終わったあとも持ち続ける」ので、
Observer より**参照を持っている時間が長い**ぶん危険です。

対処は第1章・第17章と同じ 3 択です。

| 方法 | いつ使うか |
| --- | --- |
| 受け手をコマンドより長生きさせる（規約） | 受け手が `RobotArm` 1 個のように長寿命なとき。**普通はこれ** |
| `std::weak_ptr<RobotArm>` を持ち、`lock()` してから使う | 受け手の寿命が読めないとき |
| 受け手が死ぬときに履歴を空にする | 受け手と履歴を同じクラスが持てるとき |

**1 番が第一候補です。** ロボットのアームやモータは起動時に作って終了まで生きます。
コマンド履歴だけがそれより長生きする設計になっていたら、そちらが間違っています。

### `std::function` の落とし穴 3 つ

`std::function` を採ったら採ったで、C++ 固有の話が 3 つ付いてきます。

**1. ヒープを確保しうる。** キャプチャが小さければ内部バッファに収まりますが
（Small Object Optimization）、大きいと確保が走ります。何バイトまで入るかは**規格で決まっていません**。
実測は 22.7 でやります。

**2. コピー構築可能でなければ包めない。** `std::function` はコピー可能な型なので、
中身にもコピーを要求します。`unique_ptr` をキャプチャしたラムダは入りません。

```cpp
std::function<void()> action = [p = std::make_unique<int>(3)]() { (void)p; };
```

```
error: call to implicitly-deleted copy constructor of '(lambda at ...)'
```

C++23 の `std::move_only_function` がこれを解きますが、**C++17 では使えません**。
回避策は `shared_ptr` に持ち替えることです。

**3. 参照キャプチャは寿命を延ばさない。** これが一番よく踏みます。

```cpp
ActionQueue queue;
{
  RobotArm arm;
  queue.push([&arm]() { arm.rotate(30.0); });   // 参照キャプチャ
}
queue.run_all();                                 // 死んだ arm を触る
```

**ラムダの `[&]` はポインタを持っているだけ**です。GC も参照カウントもありません。
「あとで実行する」ものに `[&]` を書いた時点で、寿命の約束が必要になります。
キューに積むラムダは `[=]` か明示キャプチャにして、
外部のオブジェクトを触るなら 22.5 冒頭の 3 択で寿命を決めてください。

## 22.6 標準ライブラリ／言語機能に同じものが無いか

**あります。しかも複数あります。**

| 標準の道具 | Command のどの側面か |
| --- | --- |
| `std::function<void()>` | 操作を値にして持ち回る。**undo は無い** |
| ラムダ式 | その場でコマンドを作る（Java の匿名クラスに相当） |
| `std::bind` / `std::invoke` | 引数を束ねてから呼ぶ。ただし `std::bind` はラムダで書ける |
| `std::packaged_task<void()>` | コマンド + 結果の受け取り口（`std::future`）。スレッドに渡す用 |
| `std::thread` / `std::async` の引数 | 「あとで別のスレッドで実行する操作」＝ Command |

つまり **C++ で「Command クラスを書く」のは、undo か直列化が要るときだけ**です。
それ以外はラムダと `std::function` で終わります。
第0章のチェックリスト 4 番「標準ライブラリに同じものが無いか」の代表例がこれです。

## 22.7 手元で試す

`std::function` がいつヒープを確保するかを実測します。
`operator new` を差し替えて数えます。**予想してから実行してください。**

```cpp
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <new>

std::size_t g_alloc_count = 0;

void * operator new(std::size_t size)
{
  ++g_alloc_count;
  void * p = std::malloc(size);
  if (p == nullptr) {
    throw std::bad_alloc();
  }
  return p;
}

void operator delete(void * p) noexcept { std::free(p); }
void operator delete(void * p, std::size_t) noexcept { std::free(p); }

struct Big
{
  double a, b, c, d, e, f;
};

int main()
{
  int x = 1;
  Big big{1, 2, 3, 4, 5, 6};

  g_alloc_count = 0;
  std::function<void()> small = [x]() { (void)x; };
  std::cout << "small capture : " << g_alloc_count << " allocation(s)\n";

  g_alloc_count = 0;
  std::function<void()> large = [big]() { (void)big; };
  std::cout << "large capture : " << g_alloc_count << " allocation(s)\n";

  std::cout << "sizeof(std::function<void()>) = "
            << sizeof(std::function<void()>) << "\n";
  std::cout << "sizeof(Big) = " << sizeof(Big) << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: どちらが何回 <code>new</code> を呼ぶか</summary>

Apple clang 17（libc++）ではこうなりました。

```
small capture : 0 allocation(s)
large capture : 1 allocation(s)
sizeof(std::function<void()>) = 32
sizeof(Big) = 48
```

`int` 1 個のキャプチャは `std::function` の内部バッファに収まるので**確保ゼロ**、
48 バイトのキャプチャは収まらないので**ヒープ確保 1 回**でした。

大事なのは数字ではありません。**この境界は規格で決まっていない**ことです。
libstdc++ と libc++ で違いますし、バージョンでも変わります。
つまり「小さいラムダなら確保されないから大丈夫」は**移植性のある根拠になりません**。

制御ループの中で `std::function` に代入する（＝毎周作り直す）コードは、
確保が走る可能性を常に持っています。ROS 2 なら許容できますが、
マイコンでは書けません。次の節でそこを扱います。
</details>

## 22.8 マイコンでの結論

**`std::function` も `std::vector` も `std::unique_ptr` も使いません。** 理由は 22.7 のとおりです。
それに、Command が本当に効くのはマイコンでこそです。

> **割り込みハンドラ（ISR）は仕事をしてはいけない。仕事は「コマンド」にしてメインループへ渡す。**

ISR の中でモータを回したり I2C を叩いたりすると、他の割り込みを止めます。
ISR がやるのは「何をすべきか」を**固定長のリングバッファに積む**ことだけです。
これが組み込みで一番よく書かれる Command です。

```cpp
#include <cstddef>
#include <cstdint>

enum class MotorCommandKind : std::uint8_t
{
  kNone = 0,
  kRotate,
  kGrip,
  kRelease,
};

// vtable なし。ヒープなし。4 バイト。ISR から積める
struct MotorCommand
{
  MotorCommandKind kind = MotorCommandKind::kNone;
  std::int16_t argument = 0;
};

template <std::size_t Capacity>
class CommandRing
{
public:
  // 満杯なら最も古いものを 1 つ落として入れる。落としたら false
  bool push(const MotorCommand & command)
  {
    const bool was_full = (size_ == Capacity);
    buffer_[head_] = command;
    head_ = (head_ + 1) % Capacity;
    if (!was_full) {
      ++size_;
    }
    return !was_full;
  }

  bool pop(MotorCommand & out)
  {
    if (size_ == 0) {
      return false;
    }
    const std::size_t oldest = (head_ + Capacity - size_) % Capacity;
    out = buffer_[oldest];
    --size_;
    return true;
  }

  bool empty() const { return size_ == 0; }

private:
  MotorCommand buffer_[Capacity] = {};
  std::size_t head_ = 0;   // 次に書く位置
  std::size_t size_ = 0;
};
```

使う側はこうなります。

```cpp
CommandRing<8> g_queue;

extern "C" void EXTI0_IRQHandler()      // 割り込み。積むだけ
{
  g_queue.push(MotorCommand{MotorCommandKind::kGrip, 0});
}

int main()
{
  RobotArm arm;
  MotorCommand command;
  for (;;) {
    while (g_queue.pop(command)) {      // メインループで実行
      apply(arm, command);
    }
  }
}
```

`apply()` は `virtual` の代わりに `switch` です。

```cpp
void apply(RobotArm & arm, const MotorCommand & command)
{
  switch (command.kind) {
    case MotorCommandKind::kRotate:
      arm.rotate(static_cast<double>(command.argument));
      break;
    case MotorCommandKind::kGrip:
      arm.set_gripper(true);
      break;
    case MotorCommandKind::kRelease:
      arm.set_gripper(false);
      break;
    case MotorCommandKind::kNone:
      break;
  }
}
```

Java 版・C++ クラス版と比べて何を捨てて何を得たかを並べます。

| | クラス版 | POD + リングバッファ版 |
| --- | --- | --- |
| ヒープ確保 | コマンド 1 個ごとに 1 回 | **ゼロ**。バッファは静的に確保済み |
| 仮想関数呼び出し | 1 実行あたり 1 回 | ゼロ（`switch`） |
| コマンドの種類を増やす | クラスを 1 つ足す。既存のコードは触らない | `enum` と `switch` の両方を直す |
| 引数の型 | 自由 | `std::int16_t` 1 個に押し込む必要がある |
| ISR から積めるか | **積めない**（確保が走る） | 積める |

**「クラスを足すだけで済む」を捨てて、「確保ゼロ」を買っています。**
コマンドの種類はマイコンでは 5〜10 個で固定なので、この取引は成立します。

**設計判断を 2 つ、明示的にしてください。**

1. **満杯のときどうするか。** 上のコードは「最も古いものを落とす」です。
   テレオペの指令は最新が正義で、古い指令が残る方が危険だからです。
   逆に「1 つも落とせない」（コマンドが手順書になっている）なら、
   `push` は入れずに `false` を返し、呼び出し側に再送させます。**黙って落とさないこと。**
2. **ISR とメインループで共有する変数は `volatile` では守れません。**
   上の `size_` / `head_` は複数命令で更新されます。
   実際には割り込み禁止で囲むか、単一 producer / 単一 consumer 前提で
   `std::atomic` にします。ここは Command パターンではなく排他の話なので、
   課題では扱いません。**が、実機に載せる前に必ず考えてください。**

undo が要るなら、POD のコマンドに「実行前の値」を 1 つ足して
`std::int16_t previous` にすれば同じことができます。
Memento を丸ごと持つより圧倒的に安く済むので、**マイコンの Undo は Command 側**が定石です。

## 22.9 ROS 2 での結論（補足）

ROS 2 では `std::function` を使ってよいので、Command クラスを書く機会はほぼありません。
そのうえで、rclcpp の中に Command が 2 か所そのまま出てきます。

- **Executor はコマンドキューです。** サブスクライバのコールバック、タイマ、サービスは
  「あとで実行する操作」として待ち行列に入り、`spin()` が取り出して実行します。
  `create_subscription` に渡すラムダが、そのままコマンドです。
- **アクションのゴールがコマンドです。** `send_goal` は「この動作を実行せよ」という
  操作を値にしてネットワーク越しに渡します。`cancel_goal` があるのも Command 的で、
  ただしこれは undo ではなく**中断**です。実機の動作は元に戻せません（22.4 の 3 番）。

ROS 2 で自作の Command キューを書きたくなったら、まず
「Executor でよいのでは」「タイマでよいのでは」を疑ってください。

## 22.10 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| マクロを undo したら状態が変になる | `undo()` を正順で回している。**末尾から逆順**に戻す |
| undo したあと別の操作をしたら、redo で状態が壊れた | `run()` で redo 履歴を `clear()` していない |
| `undo()` のあとクラッシュする | `done_.back()` を `pop_back()` してから `move` している。順序が逆 |
| `add(cmd)` がコンパイルできない | `unique_ptr` を値で受けているのに `std::move` していない |
| `std::vector<Command>` が通らない | 抽象クラスは値で持てない。具象でもスライシングする。`unique_ptr` にする |
| 履歴を undo したら落ちる | コマンドが持っている受け手（`RobotArm`）が先に死んでいる（22.5） |
| `std::function` にラムダが入らない | `unique_ptr` をキャプチャしている。コピー構築可能でないと包めない |
| 制御周期がときどき伸びる | ループ内で `std::function` を作り直してヒープ確保が走っている |
| 「閉じる」を undo したら開いてしまう | `undo()` に `!closed_` と書いた。実行前の値を控えて戻す |
| ISR で積んだコマンドが取りこぼされる | リングバッファが満杯。容量か、満杯時の方針を見直す |

## 22.11 対応する課題

```bash
./drill run dp22
```

`exercises/dp22_command/src/robot_console.cpp` に、

1. `RotateCommand` — 逆操作が自明なコマンド
2. `GripperCommand` — 実行前の状態を控えるコマンド
3. `MacroCommand` — 複数のコマンドを 1 つとして扱う（`undo()` は逆順）
4. `CommandHistory` — undo / redo。`run()` で redo 履歴を捨てる
5. `ActionQueue` — `std::function` 版（undo なし）
6. `MotorCommandRing` / `apply()` — POD + 固定長リングバッファ

を実装します。テストは**積んだ順に実行されること**、**undo が逆順であること**、
マクロが履歴に 1 個として積まれること、
`std::function` 版とクラス版の実行ログが**一致する**こと、
リングバッファが容量を超えたら古いものから落とすことを見ます。

## 22.12 この章のまとめ

- **`std::function<void()>` が C++ 版の Command。** クラスを書く前に必ず検討する
- **`std::function` は操作を 1 つしか包めない。undo が要るならクラス**
- Java の `Deque<Command>` は `std::vector<std::unique_ptr<Command>>`。
  保持するなら所有する
- `MacroCommand` は Composite（第11章）。**`undo()` は必ず逆順**
- undo の実装は 3 通り。**逆操作が自明 / 実行前を控える / そもそも取り消せない**。
  3 番目を「何もしない `undo()`」で誤魔化さない
- 状態を丸ごと持つのが Memento（第18章）、逆操作を持つのがこの章。
  巨大な状態なら Command が圧倒的に安い
- **コマンドは受け手より長生きしうる。** Observer（第17章）より参照を持つ時間が長い
- `std::function` は**ヒープを確保しうる／コピー可能でないと包めない／
  `[&]` は寿命を延ばさない**
- マイコンでは **POD のコマンド + 固定長リングバッファ**。
  ISR は積むだけ、実行はメインループ。満杯時の方針を明示する

---

前: [21. Proxy](21_Proxy.md) ／ 次: 23. Interpreter（準備中）
