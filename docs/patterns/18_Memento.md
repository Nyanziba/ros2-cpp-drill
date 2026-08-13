# 18. Memento

> **結城本 第18章 対応。** `Gamer` / `Memento` / `Main` を手元に開いてください。
> とくに `Memento` クラスの「`getMoney()` は public、`getFruits()` は package private」という
> 作り分け（narrow interface / wide interface）を見てください。
>
> **この章のねらい**: **この章は Java 版より簡単になります。** 23 章のうち数少ない例です。
> Java の Memento は「オブジェクトへの参照を渡すと中身を共有してしまう」ことへの対策を
> 兼ねていますが、C++ には値セマンティクスがあるので、**状態を値で返した時点でスナップショットが完成**します。
> 代わりに 2 つ、C++ 固有の仕事が増えます。**`friend` によるアクセス制御**と、
> **「本当に値で持っているか」の確認**です。

## 18.1 Java 版をそのまま C++ にすると

結城本の `Memento` はこうです（要点だけ）。

```java
public class Memento {
    int money;
    ArrayList<String> fruits;

    public int getMoney() { return money; }          // narrow interface
    Memento(int money) { ... }                       // wide interface（package private）
    void addFruit(String fruit) { ... }              // wide interface
    List<String> getFruits() { ... }                 // wide interface
}
```

C++ に移すとこうなります（課題のヘッダから抜粋）。

```cpp
class GainTuner;   // 前方宣言

class GainSnapshot
{
public:
  const std::string & label() const { return label_; }   // narrow interface

private:
  friend class GainTuner;                                // ← Java の package private の代わり

  GainSnapshot(double kp, double ki, double kd, std::string label)
  : kp_(kp), ki_(ki), kd_(kd), label_(std::move(label))
  {
  }

  double kp_;
  double ki_;
  double kd_;
  std::string label_;
};
```

Java 版から変えた点が 3 つあります。

### 変更点1: package private が無いので `friend class GainTuner;` にした

結城本は「Memento の wide interface は `game` パッケージの中からしか見えない」という
Java のアクセス制御に**依存して**設計されています。
**C++ に package private に相当する仕組みはありません。** 名前空間はアクセス制御をしません。
同じ `namespace game` に入れても、中身は全部見えます。

C++ でこの意図を書ける道具は `friend` だけです。

```cpp
private:
  friend class GainTuner;   // GainTuner「だけ」が中身を見られる
```

`friend` は「カプセル化を破る危険な機能」と紹介されることが多いですが、逆です。

- `friend` を使わない場合 → 中身を `public` にするしかない。**全世界に見える**
- `friend` を使う場合 → **1 つのクラスにだけ見える**。しかもその 1 つが**ヘッダに名前で書いてある**

`friend` は**カプセル化の範囲を明示的に広げる道具**であって、穴を開ける道具ではありません。
Memento は `friend` が正しく効く数少ないパターンです
（`operator<<` の次に典型的な用途と言っていい）。

なお `friend` は**一方向**です。`GainTuner` は `GainSnapshot` の中身を見られますが、
`GainSnapshot` は `GainTuner` の中身を見られません。Java の package private は双方向なので、ここは差です。

### 変更点2: `getFruits()` / `addFruit()` に相当するものを作らなかった

Java 版は「Memento を作ったあとに中身を足す」形（`addFruit`）です。GC 前提なら自然です。
C++ では、**コンストラクタで全部渡して、あとは変えない**形にします。

```cpp
GainSnapshot create_snapshot() const
{
  return GainSnapshot{kp_, ki_, kd_, label_};   // ここで完成。以後 immutable
}
```

Memento は「ある時点の状態」です。**作ったあとに変わるなら、それは Memento ではありません。**
Java 版が `addFruit` を持っているのは、`Gamer` の状態が `ArrayList` で、
コンストラクタに全部渡しづらかった事情によるものです。C++ ではコピーを 1 回書くだけで済みます。

### 変更点3: 戻り値を `Memento` の**値**にした（ポインタでも参照でもなく）

Java 版の `createMemento()` は `new Memento(...)` を返します。参照が返ります。
C++ で `std::unique_ptr<GainSnapshot>` を返したくなりますが、**この章では要りません。**

```cpp
GainSnapshot create_snapshot() const;                     // ← これでいい
std::unique_ptr<GainSnapshot> create_snapshot() const;    // ← 過剰
```

理由は 18.2 です。

## 18.2 なぜ C++ では Memento が簡単になるのか

Java でこう書くと事故ります。

```java
// Java（悪い例）
public State createMemento() {
    return this.state;      // 参照が返る。呼んだ側が state をいじると Gamer の中身も変わる
}
```

**Java では「返す」＝「参照を渡す」なので、スナップショットになりません。**
だから Java の Memento は「専用の Memento クラスを作って、そこにコピーする」必要があります。

C++ では、

```cpp
GainSnapshot create_snapshot() const
{
  return GainSnapshot{kp_, ki_, kd_, label_};
}
```

`kp_` も `label_` も**値でコピーされます**。書いた瞬間にスナップショットです。
`std::string` も `std::vector` も、コピーすれば中身ごと複製されます（深いコピーが既定）。

> **この章の一番の差はこれです。**
> Java の Memento の仕事の半分は「参照が漏れないようにする」ことで、
> C++ ではその半分が**言語機能によって最初から済んでいます**。
> C++ に残るのは「Memento の中身を誰に見せるか」（＝`friend`）だけです。

## 18.3 C++ 固有の危険 — 「値で持ったつもり」が値になっていない

18.2 は「値で持てば深いコピーになる」でした。裏返すと、**値で持たなければ何も起きません。**
やりがちな 3 つを挙げます。

| Memento のメンバ | スナップショットになるか |
| --- | --- |
| `double` / `std::string` / `std::vector<double>` | **なる**（コピーで中身ごと複製される） |
| `const State &` / `State *` | **ならない**。元が変われば見えるものも変わる。元が死ねば宙に浮く |
| `std::shared_ptr<State>` | **ならない**。「共有」が仕事のスマートポインタなので当然 |

3 行目が本命の罠です。「スマートポインタを使ったから安全」という直感が、ここでは逆に働きます。

```cpp
class ByShared
{
public:
  explicit ByShared(std::shared_ptr<Trajectory> t) : t_(std::move(t)) {}
private:
  std::shared_ptr<Trajectory> t_;   // 元と同じオブジェクトを指している
};
```

`shared_ptr` が防ぐのは**寿命**の問題であって、**共有**ではありません。むしろ共有します。
Memento で `shared_ptr` を持っていいのは、指す先が**不変（immutable）**だと保証できるときだけです。
実測は 18.5 でやります。

もう 1 つ。**Memento のコピーを禁止しないでください。**

```cpp
class GainSnapshot
{
  GainSnapshot(const GainSnapshot &) = delete;   // ← やってはいけない
};
```

Undo 履歴は `std::vector<GainSnapshot>` に積みます。`vector` は再確保のときに
要素をコピーかムーブします。コピーもムーブも消すと `push_back` すら通りません。
課題のテストは `std::is_copy_constructible<GainSnapshot>` を `static_assert` で見ています。

## 18.4 ムーブで安く戻す — `restore` を 2 つ用意する

状態が `std::string` や `std::vector` を含むと、`restore` のたびにコピー（＝確保）が走ります。
最後の 1 回だけなら奪ってしまえます。

```cpp
void restore(const GainSnapshot & snapshot);   // コピー。snapshot は再利用できる
void restore(GainSnapshot && snapshot);        // 奪う。snapshot はもう使えない
```

呼び分けは呼ぶ側が決めます。

```cpp
tuner.restore(undo_stack[1]);                       // 履歴に残したいのでコピー版
tuner.restore(std::move(undo_stack.back()));        // 使い捨てるのでムーブ版
undo_stack.pop_back();
```

ムーブ版の実装で 1 つ注意があります。

```cpp
void GainTuner::restore(GainSnapshot && snapshot)
{
  label_ = std::move(snapshot.label_);
  snapshot.label_.clear();      // ← これが要る
}
```

**ムーブしたあとの `std::string` の中身は、規格上「有効だが未規定」です。**
空になる実装がほとんどですが、短い文字列は SSO（small string optimization）で
中身が残ることもあります。「ムーブ後は空」と使う側に**約束するなら、自分で `clear()` する**。
約束しないなら、ヘッダに「ムーブ後の Memento は再利用できません」とだけ書きます。
課題では前者を採り、ヘッダに書いてテストで確認しています。

これは Memento に限らず、**ムーブを受ける関数を書くときに毎回発生する判断**です。

## 18.5 手元で試す

「値で持つ」と「`shared_ptr` で持つ」を並べます。**出力を予想してから**実行してください。

```cpp
#include <iostream>
#include <memory>
#include <string>

struct Trajectory
{
  std::string name;
};

class ByValue
{
public:
  explicit ByValue(Trajectory t) : t_(std::move(t)) {}
  const std::string & name() const { return t_.name; }

private:
  Trajectory t_;
};

class ByShared
{
public:
  explicit ByShared(std::shared_ptr<Trajectory> t) : t_(std::move(t)) {}
  const std::string & name() const { return t_->name; }

private:
  std::shared_ptr<Trajectory> t_;
};

int main()
{
  Trajectory live{"起動時の軌道"};
  auto live_shared = std::make_shared<Trajectory>(Trajectory{"起動時の軌道"});

  const ByValue snapshot_value{live};
  const ByShared snapshot_shared{live_shared};

  // 保存したあとで、元をいじる。
  live.name = "調整後の軌道";
  live_shared->name = "調整後の軌道";

  std::cout << "値で持った Memento      : " << snapshot_value.name() << "\n";
  std::cout << "shared_ptr で持った Memento: " << snapshot_shared.name() << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 2 行それぞれ何が出るか</summary>

```
値で持った Memento      : 起動時の軌道
shared_ptr で持った Memento: 調整後の軌道
```

`ByShared` は**スナップショットになっていません**。
`const ByShared` にしても、`const` が付いているのは `shared_ptr` 自体であって、
指す先ではありません。`t_->name` は書き換え可能です。

どちらも `const` を付けて `explicit` にして、見た目は同じくらい丁寧に書いてあります。
**コンパイラは何も警告しません。** 気づけるのは「Memento のメンバは値か」を毎回見る人だけです。
</details>

もう 1 つ、`friend` が効いていることも確認できます。課題のヘッダを include して、

```cpp
return saved.kp_ > 0.0 ? 0 : 1;    // GainTuner の外から wide interface に触る
```

とすると、次で止まります（実際の出力）。

```
error: 'kp_' is a private member of 'GainSnapshot'
   return saved.kp_ > 0.0 ? 0 : 1;
                ^
note: declared private here
   double kp_;
          ^
```

Java 版の「`getFruits()` はパッケージ外から呼べない」に対応するものが、
C++ ではこのエラーです。

## 18.6 標準ライブラリ／言語機能に同じものが無いか

**Memento そのものは標準ライブラリにありません。** ただし材料は全部あります。

| やりたいこと | 標準の道具 |
| --- | --- |
| 状態をコピーして保存する | **コピーコンストラクタ**（自動生成される）。これが Memento の本体 |
| 状態が複数種類ありうる | `std::variant` |
| 「保存していない」状態を表す | `std::optional<Snapshot>` |
| 履歴を積む | `std::vector` / `std::deque` |

とくに 1 行目。**状態が単一のクラスにまとまっているなら、`Memento` クラスを作る必要はありません。**

```cpp
class GainTuner
{
public:
  GainTuner create_snapshot() const { return *this; }   // 自分のコピーが Memento
  void restore(const GainTuner & snapshot) { *this = snapshot; }
};
```

これで動きます。第 0 章の判断基準（「この抽象を消したら、どの変更が難しくなるか」）に
照らして、専用の Memento 型を作る理由は次のどちらかがあるときだけです。

1. **状態の一部だけを保存したい**（通信ハンドルやミューテックスは保存したくない）
2. **保存したものを外に渡すが、中身は見せたくない**（＝`friend` が要る）

課題は 2 に当たります。1 も 2 も無いなら `*this` のコピーで十分です。
「パターンを覚えたから型を増やす」のが第 0 章で言った壊し方です。

## 18.7 マイコンでの結論

Undo 履歴を `std::vector<Memento>` で持つと、こうなります。

- `push_back` のたびに確保が走りうる。**ループ中の確保は禁止**
- 履歴が伸び続ける。上限を書いていないので、いつメモリを使い切るか読めない
- 状態が `std::string` を含むと、1 件保存するたびにさらに確保

マイコンでは 3 つとも作り直します。

1. 状態を **POD**（trivially copyable）にする
2. 履歴を **固定長リングバッファ**にする。容量を超えたら古いものから捨てる
3. 保存は `memcpy` 1 回

```cpp
#include <cstddef>
#include <cstring>
#include <type_traits>

// 1. 状態は POD。std::string を入れない
struct GainState
{
  double kp;
  double ki;
  double kd;
};

static_assert(
  std::is_trivially_copyable<GainState>::value,
  "GainState は memcpy で保存するので trivially copyable でなければなりません");

// 2. 固定長リングバッファ。動的確保ゼロ
class GainHistory
{
public:
  static constexpr std::size_t kCapacity = 4;

  void push(const GainState & state)
  {
    std::memcpy(&buffer_[head_], &state, sizeof(GainState));   // 3. memcpy 1 回
    head_ = (head_ + 1) % kCapacity;
    if (size_ < kCapacity) {
      ++size_;
    }
  }

  std::size_t size() const { return size_; }

  // back_index = 0 が最新、1 が 1 つ前
  GainState recent(std::size_t back_index) const
  {
    const std::size_t index = (head_ + kCapacity - 1 - back_index) % kCapacity;
    return buffer_[index];
  }

private:
  GainState buffer_[kCapacity] = {};
  std::size_t head_ = 0;   // 次に書き込む位置
  std::size_t size_ = 0;
};
```

`static_assert` を書いておく意味は大きいです。あとから誰かが

```cpp
struct GainState
{
  double kp, ki, kd;
  std::string label;    // ← 足した
};
```

とすると、**`memcpy` が壊れる前にコンパイルが止まります**。
`memcpy` で `std::string` を運ぶと、同じヒープ領域を 2 つの `string` が指し、
二重解放します。`static_assert` が無ければ、テストでは通ってフィールドで落ちる類のバグです。

`friend` によるアクセス制御は、この版では**捨てています**。`GainState` の中身は public です。
`memcpy` で運ぶために POD であることを取ったので、カプセル化とは両立しません。
**マイコン版は「カプセル化」より「確保ゼロと壊れない保存」を優先する**、という判断です。
判断を書き残しておかないと、次に読む人が「なぜ public なのか」で迷います。

なお `kCapacity` を `4` のような固定値にしたのは、RAM の使用量を
**コンパイル時に確定させる**ためです。`sizeof(GainHistory)` を見れば何バイト使うか分かります。

## 18.8 ROS 2 での結論（補足）

ROS 2 側では、Memento を自作する場面はほとんどありません。

- パラメータの保存・復元は `rclcpp::Parameter` の値をコピーして持てば済みます。
  `get_parameters()` が返すのは値なので、それ自体がスナップショットです
- ノードの状態遷移を巻き戻したいなら、まず lifecycle node の状態機械
  （第 19 章 State の話）で表現できないかを見てください
- 「あとで再現したい」が目的なら、Memento より **rosbag に記録する**方が実務的です

自作するとすれば、キャリブレーション値やゲインの「試して、まずかったら戻す」用途です。
それは本質的にマイコン側と同じ話で、ROS 2 側では `std::vector<Snapshot>` で構いません。

## 18.9 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| 保存したはずの Memento が、元をいじると一緒に変わる | Memento が `shared_ptr` / 参照 / ポインタで持っている。値にする |
| `error: 'kp_' is a private member of 'GainSnapshot'` | 意図どおり。`friend class GainTuner;` が効いている。Originator 経由で触る |
| `friend` を書いたのに見えない | クラス名の綴り違い、または前方宣言が無い。`class GainTuner;` を先に書く |
| `std::vector<Memento>` に `push_back` できない | Memento のコピー／ムーブを `= delete` している |
| ムーブ版で戻したあと、Memento の中身が残っていたり空だったりで安定しない | ムーブ後の `std::string` は未規定。約束するなら明示的に `clear()` する |
| Undo が増えるほど遅くなる | 状態全体をコピーしている。差分だけ持つ（→ 第 22 章 Command） |
| マイコンで `memcpy` して保存したら二重解放した | 状態に `std::string` などが入った。`static_assert(is_trivially_copyable)` を置く |
| リングバッファの `recent()` が 1 つずれる | `head_` は「次に書く位置」。最新は `head_ - 1` |

## 18.10 状態が大きいときは Memento を使わない（第 22 章への橋）

Memento は**状態を丸ごとコピー**します。状態が 10 KB あって Undo を 100 段持てば 1 MB です。
マイコンでは即死、ROS 2 でも気持ちよくはありません。

代わりに「**何をしたか**」を記録します。

| 方式 | 記録するもの | 戻し方 | コスト |
| --- | --- | --- | --- |
| Memento | 状態のスナップショット | 上書き | 状態のサイズ × 段数 |
| Command（第 22 章） | 操作と、その逆操作 | 逆操作を実行 | 操作のサイズ × 段数 |

「kp を 1.0 から 2.0 にした」という差分は `double` 2 つです。状態全体より遥かに小さい。
実務の Undo（エディタ、CAD、パラメータ調整 GUI）はほぼ Command 側です。

**逆操作が定義できるとき**は Command、**逆操作が書けない／面倒なとき**は Memento、
と切り分けてください。両方使う実装もよくあります
（N 回に 1 回だけ Memento を取り、あいだは Command で埋める）。
第 22 章で Command 側を書きます。

## 18.11 対応する課題

```bash
./drill run dp18
```

`exercises/dp18_memento/src/gain_tuner.cpp` に、

1. **`GainTuner::create_snapshot()`** — 現在の状態を `GainSnapshot` として**値で**返す
2. **`GainTuner::restore(const GainSnapshot &)`** — コピーして戻す（Memento は再利用できる）
3. **`GainTuner::restore(GainSnapshot &&)`** — 奪って戻す。奪ったあと `clear()` する
4. **`GainTuner::capture_state()` / `restore_state()`** — マイコン版の POD 経路
5. **`GainHistory::push()` / `size()` / `recent()`** — 固定長リングバッファ

を実装します。テストは、スナップショットを取ったあとに Originator をいじっても
**Memento が変わらないこと**、`std::vector<GainSnapshot>` で任意の時点に戻せること、
`GainSnapshot` のコンストラクタが外から呼べないこと（`std::is_constructible` の `static_assert`）、
リングバッファが容量を超えたら古いものから捨てることを見ます。

## 18.12 この章のまとめ

- **C++ では Memento が Java より簡単になる。** 値でコピーした時点でスナップショットが完成する
- Java の package private に相当するものは C++ に無い。**`friend class Originator;` で書く**
- `friend` はカプセル化を破る道具ではなく、**範囲を明示的に、1 つだけ広げる道具**
- Memento のメンバが `shared_ptr` / 参照 / ポインタだと、**スナップショットにならない**。
  `shared_ptr` が防ぐのは寿命であって共有ではない
- Memento のコピーは禁止しない。`std::vector` に積めなくなる
- `restore` はコピー版とムーブ版を用意する。**ムーブ後の状態を約束するなら自分で `clear()` する**
- 状態が 1 つのクラスにまとまっているなら、**`*this` のコピーで足りる**。専用型を作る理由を言えるときだけ作る
- マイコンでは POD + 固定長リングバッファ + `memcpy`。
  **`static_assert(std::is_trivially_copyable<...>)` を必ず置く**
- 状態が大きいなら Memento ではなく Command（第 22 章）で差分を持つ

---

前: [17. Observer](17_Observer.md) ／ 次: 19. State（準備中）
