# 16. Mediator

> **結城本 第16章 対応。** `Mediator` / `Colleague` インタフェースと、
> `LoginFrame`・`ColleagueCheckbox`・`ColleagueButton` を手元に開いてください。
>
> **この章のねらい**: 結城本の `LoginFrame` は `Colleague` を配列で持ち、
> 各 `Colleague` は `setMediator()` で `LoginFrame` を持ち返します。**互いに指し合う構造**です。
> Java ではこれで何も起きません。C++ で同じ形を `std::shared_ptr` で書くと、
> **参照カウントが 0 にならず、両方とも一生解放されません**。
> この章はそれを実際にリークさせて、デストラクタのログが出ないことを見るところから始めます。
> 加えて、Mediator パターンは**入れると必ず 2 段階初期化になる**という構造上の副作用があります。

## 16.1 Java 版をそのまま C++ にすると

結城本の 2 つのインタフェースはこうです。

```java
public interface Mediator {
    public abstract void createColleagues();
    public abstract void colleagueChanged();
}

public interface Colleague {
    public abstract void setMediator(Mediator mediator);
    public abstract void setColleagueEnabled(boolean enabled);
}
```

C++ に移すとこうなります。

```cpp
class PanelWidget;                       // 前方宣言

class PanelMediator
{
public:
  virtual ~PanelMediator() = default;
  virtual void widget_changed(PanelWidget * widget) = 0;
};
```

Java 版から変えた点が 3 つあります。

### 変更点1: `createColleagues()` を消した

Java 版の `Mediator` には `createColleagues()` があります。C++ では**インタフェースに置きません**。
生成は `ControlPanel` のコンストラクタの仕事で、外から呼ぶものではないからです。

Java 版がインタフェースに置いているのは、`LoginFrame` のコンストラクタから
`createColleagues()` を呼ぶという流れを型で表明したかったためですが、
C++ でこれをやると**構築中のオブジェクトの仮想関数を呼ぶ**ことになります。
基底クラスのコンストラクタ実行中は派生の vtable がまだ入っていないので、
**派生の実装ではなく基底の実装が呼ばれます**（Java は逆に派生が呼ばれます。ここは挙動が違います）。

**ルール**: コンストラクタとデストラクタから仮想関数を呼ばない。この章はその典型例です。

### 変更点2: `colleagueChanged()` に「誰が」を足した

Java 版は引数なしです。`LoginFrame` が自分のフィールドを直接見て判断します。
C++ でも同じにできますが、この課題では `widget_changed(PanelWidget * widget)` にしました。
理由は 2 つです。

- テストから「Mediator を経由したか」「何回経由したか」を観測できる
- 送り主で分岐したくなったときに、**キャストではなく名前**で判断できる

`dynamic_cast` で送り主の型を判定したくなりますが、**マイコンでは `-fno-rtti` で使えません**。
名前か ID で判別する形にしておくと、そのまま持っていけます。

### 変更点3: `Colleague` の引数を `PanelMediator *` にした（`shared_ptr` にしない）

ここがこの章の本題です。**`std::shared_ptr<PanelMediator>` にしてはいけません。**
理由は 16.3 で実測します。

## 16.2 誰が誰を所有するのか

Mediator パターンは、**所有の向きを決めないと必ず壊れます**。決め方は 1 つだけです。

```
ControlPanel --(std::unique_ptr で所有)--> PanelWidget
PanelWidget  --(生ポインタで指すだけ)-----> PanelMediator
```

**所有は 1 方向。逆向きは「指すだけ」。** これが答えです。

```cpp
class ControlPanel : public PanelMediator
{
private:
  std::unique_ptr<ToggleWidget> emergency_stop_;   // 所有する
  std::unique_ptr<ToggleWidget> auto_mode_;
  // ...
};

class PanelWidget
{
private:
  PanelMediator * mediator_ = nullptr;             // 所有しない
};
```

生ポインタが出てくると身構えますが、ここでは**生ポインタこそが正しい表明**です。
「所有しない」を型で言う手段が生ポインタ（または `std::weak_ptr`）だからです。
`unique_ptr` と `shared_ptr` は所有を意味するので、ここに置くと嘘になります。

**寿命の約束**も要ります。「Mediator は Colleague より長生きする」。
これは自動的に守られます。Mediator が Colleague を所有しているので、
Mediator が死ぬときに Colleague も一緒に死ぬからです。**逆の順序が起きません。**

## 16.3 C++ 固有の危険 — 相互に `shared_ptr` を持つと解放されない

「所有関係が読めないから、とりあえず `shared_ptr` にしておこう」が最悪手です。
実際に動かして確かめてください。

```cpp
#include <iostream>
#include <memory>
#include <vector>

struct Colleague;

// --- 悪い例: Mediator も Colleague も shared_ptr で持ち合う -------------------
struct BadMediator
{
  ~BadMediator() { std::cout << "~BadMediator\n"; }
  std::vector<std::shared_ptr<Colleague>> members;
};

struct Colleague
{
  ~Colleague() { std::cout << "~Colleague\n"; }
  std::shared_ptr<BadMediator> mediator;   // ここが循環の元
};

// --- 良い例: 所有は Mediator → Colleague の 1 方向だけ ----------------------
struct GoodMediator;

struct GoodColleague
{
  ~GoodColleague() { std::cout << "~GoodColleague\n"; }
  GoodMediator * mediator = nullptr;       // 指すだけ。所有しない
};

struct GoodMediator
{
  ~GoodMediator() { std::cout << "~GoodMediator\n"; }
  std::vector<std::unique_ptr<GoodColleague>> members;
};

int main()
{
  std::cout << "--- bad ---\n";
  {
    auto mediator = std::make_shared<BadMediator>();
    auto colleague = std::make_shared<Colleague>();
    mediator->members.push_back(colleague);
    colleague->mediator = mediator;
    std::cout << "use_count: mediator=" << mediator.use_count()
              << " colleague=" << colleague.use_count() << "\n";
  }
  std::cout << "--- bad ここまで ---\n";

  std::cout << "--- good ---\n";
  {
    auto mediator = std::make_unique<GoodMediator>();
    mediator->members.push_back(std::make_unique<GoodColleague>());
    mediator->members.back()->mediator = mediator.get();
  }
  std::cout << "--- good ここまで ---\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: <code>~BadMediator</code> と <code>~Colleague</code> は出るか</summary>

**出ません。** 実行結果はこうです。

```
--- bad ---
use_count: mediator=2 colleague=2
--- bad ここまで ---
~GoodMediator
~GoodColleague
--- good ここまで ---
```

`bad` のブロックでは、スコープを抜けてもデストラクタが**1 つも呼ばれていません**。
`use_count` が 2 になっているのが原因です。ローカル変数の `shared_ptr` が死んでも
カウントが 1 残る（互いが持ち合っているぶん）ので、0 にならず解放されません。
**これがリークです。**

しかもエラーも警告も出ません。`-Wall -Wextra -Wpedantic` は無言で通ります。
実行しても落ちません。**気づく手段はデストラクタのログか、メモリ使用量の監視だけ**です。

`good` のブロックでは、`unique_ptr` が 1 方向に持つだけなので両方きちんと死んでいます。
</details>

### `weak_ptr` を使う形

Mediator 自身が `shared_ptr` で管理されていて、生ポインタでは寿命が保証できない場合は、
Colleague 側を `std::weak_ptr` にします。

```cpp
class PanelWidget
{
public:
  void set_mediator(std::weak_ptr<PanelMediator> mediator) { mediator_ = std::move(mediator); }

protected:
  void notify_changed()
  {
    // lock() で一時的に shared_ptr へ昇格する。既に死んでいれば nullptr。
    if (const std::shared_ptr<PanelMediator> mediator = mediator_.lock()) {
      mediator->widget_changed(this);
    }
  }

private:
  std::weak_ptr<PanelMediator> mediator_;
};
```

`weak_ptr` は**カウントを増やさない**ので循環しません。加えて
「Mediator が先に死んでいたら何もしない」が安全に書けます。

ただし代償があります。`lock()` は毎回**アトミックなカウンタ操作**を行い、
`shared_ptr` を 1 個構築します。ボタンを押すたびに走る程度なら誤差ですが、
制御ループの中で毎周回叩くなら測ってください。**マイコンでは使いません**（16.7）。

| 手段 | 循環するか | コスト | いつ使うか |
| --- | --- | --- | --- |
| 生ポインタ | しない | ゼロ | **Mediator が Colleague を所有している**とき（＝ほぼ常に） |
| `std::weak_ptr` | しない | `lock()` のたびにカウンタ操作 | Mediator の寿命が外部管理で読めないとき |
| `std::shared_ptr` | **する** | — | 使わない |

課題では 1 番を使います。**Mediator が Colleague を所有しているなら、生ポインタで正しい**からです。

## 16.4 Mediator を入れると必ず 2 段階初期化になる

これは Java でも同じですが、C++ では**より痛い**話です。

Colleague は Mediator を必要とし、Mediator は Colleague を必要とします。
コンストラクタの初期化子リストで両方向を繋ぐことはできません。**片方が先に完成していないと相手を渡せない**からです。

```cpp
ControlPanel::ControlPanel()
: emergency_stop_(std::make_unique<ToggleWidget>("emergency_stop")),
  auto_mode_(std::make_unique<ToggleWidget>("auto_mode"))
{
  // ここに来て初めて Colleague が存在する。ここから逆向きを繋ぐ。
  emergency_stop_->set_mediator(this);
  auto_mode_->set_mediator(this);
  update_enabled_states();
}
```

**危険が 2 つ**あります。

**危険1: Mediator 未設定の Colleague が存在しうる。**
`std::make_unique<ToggleWidget>(...)` が返った直後から `set_mediator(this)` を呼ぶまでの間、
その Colleague は「誰にも報告できない」状態です。この間に何かが `notify_changed()` を呼ぶと
`mediator_` は `nullptr` です。だから `notify_changed()` は必ず nullptr チェックを持ちます。

```cpp
void PanelWidget::notify_changed()
{
  if (mediator_ == nullptr) {
    return;          // 未結線。落とさない
  }
  mediator_->widget_changed(this);
}
```

「未結線なら例外を投げる」は選べません。マイコンでは `-fno-exceptions` です。
**未結線を「壊れた状態」ではなく「まだ何もしない状態」として設計します。**

**危険2: 構築中の `this` を配っている。**
`set_mediator(this)` の時点で `ControlPanel` はまだ構築中です。
`set_mediator()` が**ポインタを保存するだけ**だから安全なのであって、
ここで Colleague が `mediator->widget_changed(...)` を呼び返したら、
まだ初期化されていない `ControlPanel` のメンバに触ることになります。

**ルール**: 2 段階初期化の 1 段目で渡す `this` は、**保存されるだけ**でなければならない。
そのあとに `update_enabled_states()` を呼ぶ順序も、この理由で入れ替えられません。

なお `notify_changed()` を**ヘッダのインライン定義にはできません**。
`PanelMediator` は前方宣言しかされていないためです。

```cpp
class PanelMediator;

class PanelWidget
{
public:
  void notify_changed() { mediator_->widget_changed(this); }
  // ...
};
```

```
error: member access into incomplete type 'PanelMediator'
note: forward declaration of 'PanelMediator'
```

相互参照する 2 つのクラスは、**片方の実装を必ず `.cpp` に追い出す**ことになります。
これも Mediator パターンに付いてくる構造上の副作用です。

## 16.5 無限再帰に注意 — 「呼ぶ入口」と「呼ばれる入口」を分ける

Colleague には 2 種類のメソッドがあります。**混ぜると即座に無限再帰します。**

| 種類 | 例 | Mediator に報告するか |
| --- | --- | --- |
| ユーザ操作の入口（Colleague → Mediator） | `set_checked()` / `press()` | **する** |
| Mediator からの指示の入口（Mediator → Colleague） | `set_enabled()` | **しない** |

`set_enabled()` が `notify_changed()` を呼ぶと、
`widget_changed()` → `update_enabled_states()` → `set_enabled()` → `widget_changed()` …
と戻ってこなくなります。結城本の Java 版でも `setColleagueEnabled()` は通知しません。同じ理由です。

もう 1 つ、**値が変わっていないなら報告しない**ことも要ります。

```cpp
void ToggleWidget::set_checked(bool checked)
{
  if (!is_enabled()) { return; }
  if (checked == is_checked_) { return; }   // 変化なし。報告しない
  is_checked_ = checked;
  notify_changed();
}
```

これが無いと、`set_checked(true)` を 2 回呼ぶだけで Mediator が 2 回走ります。
状態が同じなら無害ですが、Mediator が「変化のたびにログを吐く」「モータに指令を出す」なら害です。

## 16.6 標準ライブラリ／言語機能に同じものが無いか

**ありません。** C++ 標準ライブラリに Mediator に相当するものは存在しません。

近いものとして「シグナル・スロット」がありますが、標準ではありません。

| 仕組み | どこにあるか | 関係 |
| --- | --- | --- |
| Qt の signal/slot | Qt（標準ではない） | 通知の配線を宣言的に書く。Mediator というより Observer 寄り |
| `boost::signals2` | Boost（標準ではない） | 同上 |
| `std::function` のコールバック表 | 標準 | Mediator の中身を関数の表で持つ実装手段。パターンそのものではない |

**`std::function` を並べただけでは Mediator になりません。** Mediator の本質は
「調停ロジックが 1 箇所に集まっていること」であって、通知の配線ではありません。
配線だけが欲しいなら、それは第 17 章 Observer です。**この 2 つを混同しないでください。**

## 16.7 Mediator は肥大化する — 入れる前の目安

`00_使う前に.md` の姿勢どおりです。**Mediator は放っておくと God object になります。**

理由は構造的です。Colleague が増えるたびに調停ロジックが増え、
それが全部 1 つの `widget_changed()` に集まるからです。
Colleague が 8 個あれば、その 8 個すべての事情を知っている関数が 1 つできます。
**それは「複雑さを消した」のではなく「1 箇所に集めた」だけ**です。

集めることに価値があるのは、**もともと N×N の線が引かれていた場合**だけです。

| Colleague の数 | 直接つないだ場合の線 | Mediator を入れた場合 | 判断 |
| --- | --- | --- | --- |
| 2 | 1 本 | 2 本 + Mediator 1 クラス | **入れない。増えている** |
| 3 | 3 本 | 3 本 + Mediator 1 クラス | **入れない。ほぼ同じ** |
| 5 | 10 本 | 5 本 + Mediator 1 クラス | 検討する |
| 8 | 28 本 | 8 本 + Mediator 1 クラス | 入れる価値がある |

**目安: Colleague が 3 個以下なら入れない。**
「ボタンを押したらランプが点く」だけなら、ボタンがランプを直接知っていて構いません。

さらに 2 つ聞いてください。

1. **Colleague どうしの関係は本当に相互か。** 一方向（A が変わったら B に伝わるだけ）なら
   Observer で足ります。Mediator は「A も B も互いに影響する」ときのものです
2. **調停ルールを 1 つの関数に書き下せるか。** 書き下せないなら、
   それは Mediator ではなく**状態機械**です。第 19 章 State を見てください

肥大化しはじめたときの逃げ道は 1 つです。**送り主で分岐せず、「今の状態から全部決め直す」形にする。**
課題の `update_enabled_states()` がそれです。

```cpp
// 悪い: 送り主ごとに分岐する。Colleague が増えるたびに if が増える
void widget_changed(PanelWidget * widget)
{
  if (widget == auto_mode_.get()) { /* ... */ }
  else if (widget == emergency_stop_.get()) { /* ... */ }
  // 8 個になると 8 分岐。しかも組み合わせの漏れが出る
}

// よい: 誰から来たかに関係なく、現在の状態から全部を導出する
void widget_changed(PanelWidget *) { update_enabled_states(); }
```

分岐が消えるうえ、**状態の食い違いが起きません**。
「非常停止中に自動モードを切ったときだけ手動ボタンが有効に戻ってしまう」といったバグは、
分岐で書いたときにだけ出ます。

## 16.8 マイコンでの結論

**ヒープを使いません。** `unique_ptr` も `shared_ptr` も `vector` も `string` も使わず、
Mediator と Colleague を**静的に持って、起動時に一度だけ結線**します。

```cpp
#include <cstdint>

class PanelMediator;

// Colleague。ヒープを使わない。Mediator は生ポインタで指すだけ。
class Widget
{
public:
  constexpr explicit Widget(std::uint8_t id) : id_(id) {}

  std::uint8_t id() const { return id_; }
  bool is_on() const { return is_on_; }
  bool is_enabled() const { return is_enabled_; }
  void set_enabled(bool enabled) { is_enabled_ = enabled; }
  void set_mediator(PanelMediator * mediator) { mediator_ = mediator; }

  void set_on(bool on);

private:
  std::uint8_t id_;
  PanelMediator * mediator_ = nullptr;
  bool is_on_ = false;
  bool is_enabled_ = true;
};

// Mediator。仮想関数を 1 つに絞る（vtable は 1 個で済む）。
class PanelMediator
{
public:
  virtual ~PanelMediator() = default;
  virtual void widget_changed(Widget * widget) = 0;
};

void Widget::set_on(bool on)
{
  if (!is_enabled_ || on == is_on_) {
    return;
  }
  is_on_ = on;
  if (mediator_ != nullptr) {
    mediator_->widget_changed(this);
  }
}

constexpr std::uint8_t kEmergencyStop = 0;
constexpr std::uint8_t kAutoMode = 1;
constexpr std::uint8_t kManualForward = 2;

// ConcreteMediator。メンバは値で持つ。new も vector も string も使わない。
class ControlPanel : public PanelMediator
{
public:
  ControlPanel()
  {
    // 2 段階初期化。起動時に一度だけ結線する。
    for (Widget & widget : widgets_) {
      widget.set_mediator(this);
    }
    update_enabled_states();
  }

  Widget & widget(std::uint8_t id) { return widgets_[id]; }

  void widget_changed(Widget *) override { update_enabled_states(); }

private:
  void update_enabled_states()
  {
    const bool is_emergency = widgets_[kEmergencyStop].is_on();
    widgets_[kEmergencyStop].set_enabled(true);
    widgets_[kAutoMode].set_enabled(!is_emergency);
    widgets_[kManualForward].set_enabled(!is_emergency && !widgets_[kAutoMode].is_on());
  }

  // 固定長配列。要素数はコンパイル時に決まる。
  Widget widgets_[3] = {Widget{kEmergencyStop}, Widget{kAutoMode}, Widget{kManualForward}};
};

// 静的に確保する。起動時に 1 度だけ構築され、以降ヒープは一切触らない。
ControlPanel g_panel;

int main()
{
  g_panel.widget(kAutoMode).set_on(true);
  return g_panel.widget(kManualForward).is_enabled() ? 1 : 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti micro.cpp -o micro
./micro; echo "exit=$?"
```

`exit=0` になります。自動モードを入れたことで `manual_forward` が無効になったからです。
**`-fno-exceptions -fno-rtti` で警告ゼロで通ります。** 変更点を並べます。

| ホスト版 | マイコン版 | 理由 |
| --- | --- | --- |
| `std::unique_ptr<Widget>` × 4 | `Widget widgets_[3]` | 動的確保をしない。要素数はコンパイル時に確定 |
| `std::string name_` | `std::uint8_t id_` | `std::string` は確保が走る |
| `std::vector<std::string> change_log_` | 持たない | ログは固定長リングバッファか、そもそも取らない |
| 送り主で `dynamic_cast` | ID で判別 | `-fno-rtti` では `dynamic_cast` が使えない |
| 未結線で例外 | 未結線なら何もしない | `-fno-exceptions` |

**`ControlPanel g_panel;` をグローバルに置いています。** 第 5 章 Singleton で見た
静的初期化順序の問題は、`ControlPanel` が**他の翻訳単位のグローバルに依存していない**ので起きません。
依存させるなら Meyers Singleton（関数内 static）にしてください。

**vtable のコスト**も見ておきます。仮想関数は `PanelMediator::widget_changed()` と
仮想デストラクタの 2 つだけです。`Widget` 側には仮想関数を 1 つも置いていません。
Colleague を仮想にすると、**Colleague の個数ぶん vtable ポインタが RAM に乗ります**。
種類が本当に必要なとき以外、Colleague は具体型で持ってください。

## 16.9 ROS 2 での結論（補足）

ROS 2 では `shared_ptr` が至るところに出るので、**循環参照が現実の事故になります**。

`rclcpp::Node` がコールバックのラムダに `this` ではなく `shared_from_this()` を
キャプチャして自分のメンバに保存すると、**ノードが一生解放されません**。
`std::weak_ptr` にして、コールバックの先頭で `lock()` するのが定石です。

```cpp
std::weak_ptr<MyNode> weak_self = shared_from_this();
timer_ = create_wall_timer(100ms, [weak_self]() {
  if (const auto self = weak_self.lock()) {
    self->on_timer();
  }
});
```

ただし、**ノード間の調停を Mediator クラスで書くことは普通ありません**。
ROS 2 ではトピック・サービス・パラメータが既に「直接つながせない仕組み」だからです。
Mediator を書くのは**1 つのノードの中**、あるいは**ライブラリの中**の話に限ります。

## 16.10 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| デストラクタが 1 つも呼ばれない。落ちもしない | 相互に `shared_ptr`。16.3 の循環参照 |
| スタックオーバーフローで落ちる | `set_enabled()` が `notify_changed()` を呼んでいる。16.5 |
| `error: member access into incomplete type 'PanelMediator'` | 前方宣言しかないヘッダで `mediator_->` している。実装を `.cpp` へ |
| コンストラクタ内で `set_mediator(this)` した直後に落ちる | 1 段目で `this` を配った先が、すぐ呼び返している。16.4 の危険2 |
| Colleague の初期状態が調停ルールと食い違う | コンストラクタ末尾で `update_enabled_states()` を呼び忘れ |
| Colleague を 1 つ追加したら他の組み合わせが壊れた | 送り主で分岐している。「現在の状態から全部決め直す」形へ。16.7 |
| `mediator_->` で nullptr 参照 | 2 段階初期化の途中。`notify_changed()` の nullptr チェック漏れ |
| 派生の `createColleagues()` が呼ばれない | 基底のコンストラクタから仮想関数を呼んでいる。Java とは挙動が違う。16.1 |

## 16.11 対応する課題

```bash
./drill run dp16
```

`exercises/dp16_mediator/src/control_panel.cpp` に、ロボットの操作パネルを実装します。

1. `PanelWidget::set_mediator()` — 生ポインタで保存するだけ。**所有しない**
2. `PanelWidget::notify_changed()` — nullptr チェックのうえ `widget_changed(this)`
3. `ToggleWidget::set_checked()` / `ButtonWidget::press()` — 無効なら何もしない。変化したときだけ報告
4. `ControlPanel` のコンストラクタ — 2 段階初期化で 4 つの Colleague を結線
5. `ControlPanel::widget_changed()` / `update_enabled_states()` — 調停ルール

テストは調停ルールに加えて、**Mediator を外すと Colleague 間に影響が伝わらないこと**
（＝直接つながっていないこと）、**`use_count()` が 1 のまま**であること（＝所有していないこと）、
**`ControlPanel` を破棄すると 4 つの Colleague すべてのデストラクタが呼ばれること**を見ます。

## 16.12 この章のまとめ

- Mediator は Colleague と**互いに指し合う**。C++ で両方 `shared_ptr` にすると**循環参照でリークする**
- リークしても**警告も例外も出ない**。デストラクタのログを見るしか気づく手段がない
- 所有は **Mediator → Colleague の 1 方向**。逆向きは生ポインタ、寿命が読めないときだけ `weak_ptr`
- ここでは**生ポインタが「所有しない」の正しい表明**。`shared_ptr` にすると嘘になる
- Mediator は必ず **2 段階初期化**になる。「Mediator 未設定の Colleague」を落ちない状態として設計する
- 相互参照するクラスは、**片方の実装を `.cpp` に追い出す**しかない（不完全型）
- `set_enabled()` から通知してはいけない。**呼ぶ入口と呼ばれる入口を分ける**
- 標準ライブラリに Mediator は**無い**。シグナル・スロットは Qt / Boost であって標準ではない
- **Colleague が 3 個以下なら入れない。** 集める価値があるのは線が N×N になってから
- 送り主で分岐せず「現在の状態から全部決め直す」と、肥大化とバグの両方が減る
- マイコンでは**静的に持って起動時に一度だけ結線**。`-fno-exceptions -fno-rtti` で通る

---

前: [15. Facade](15_Facade.md) ／ 次: 17. Observer（準備中）
