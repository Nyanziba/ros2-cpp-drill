# 21. Proxy

> **結城本 第21章 対応。** `Printer` / `PrinterProxy` / `Printable` を手元に開いてください。
>
> **この章のねらい**: 結城本の `PrinterProxy` は `Printable` を実装（継承）することで
> 本人になりすまします。C++ でも同じことは書けますが、**それは C++ の Proxy の主流ではありません。**
> C++ には `operator->` があり、`std::unique_ptr` も `std::shared_ptr` も
> **`operator->` を持つだけの Proxy** です。この章では `operator->` を自分で書きます。
> そして、`operator->` は**ポインタが返るまで繰り返し呼ばれる**という C++ 独自の規則を使って、
> アクセスの前後に処理を挟みます。

## 21.1 Java 版をそのまま C++ にすると

結城本の構造はこうです。

```java
public interface Printable {
    public abstract void setPrinterName(String name);
    public abstract String getPrinterName();
    public abstract void print(String string);
}

public class PrinterProxy implements Printable {
    private String name;
    private Printer real = null;      // 「本人」
    public synchronized void print(String string) {
        realize();
        real.print(string);
    }
    private synchronized void realize() {
        if (real == null) { real = new Printer(name); }
    }
}
```

C++ に素直に移すとこうなります。

```cpp
class Printable
{
public:
  virtual ~Printable() = default;                       // 変更点1
  virtual void set_printer_name(std::string name) = 0;
  virtual const std::string & printer_name() const = 0;  // 変更点2
  virtual void print(const std::string & text) = 0;
};

class PrinterProxy : public Printable
{
public:
  void print(const std::string & text) override
  {
    realize();
    real_->print(text);
  }

private:
  void realize() const                                  // 変更点3
  {
    if (!real_) { real_ = std::make_unique<Printer>(name_); }
  }

  std::string name_;
  mutable std::unique_ptr<Printer> real_;               // 変更点4
  mutable std::mutex mutex_;                            // 変更点5
};
```

変更点が 5 つあります。

### 変更点1: `virtual ~Printable() = default;`

`Printable` を `unique_ptr<Printable>` で持って捨てるので、仮想デストラクタが要ります。
第 1 章から毎章同じです。

### 変更点2: `String getPrinterName()` を `const std::string &` にした

Java は参照が返るので毎回コピーは起きません。C++ で `std::string printer_name()` と書くと
**呼ぶたびに文字列がコピーされます**。`const &` を付けます。
`const` メンバ関数にするのも C++ 側の仕事です（Java にこの区別はありません）。

### 変更点3: `realize()` が `const` メンバ関数になる

`printer_name()` は `const` です。しかし結城本の `getPrinterName()` は
**`realize()` を呼びません**（名前は Proxy が知っているので）。
一方 `print()` は `const` ではないので問題は出ない……と思うと、実務ではすぐ壊れます。

「本体を見ないと答えられない `const` な問い合わせ」が必ず出てくるからです。
`entry_count() const` のような。そのとき `realize()` が `const` でないと詰みます。
**最初から `const` にしておきます。**

### 変更点4: `real_` に `mutable` を付ける

`const` メンバ関数から `real_` に代入するので `mutable` が要ります。
外すとこうなります（Apple clang での実測）。

```
error: no viable overloaded '='
note: 'this' argument has type 'const std::unique_ptr<Real>', but method is not marked const
```

`mutable` は「論理的には `const`、物理的には書き換える」ときのための道具です。
**遅延生成はその代表例**で、キャッシュも同じです。乱用は禁物ですが、ここは正当な用途です。

### 変更点5: `synchronized` は `std::mutex` になる

Java の `synchronized` はメソッドに付ければ済みます。C++ には無いので
`mutable std::mutex mutex_;` を持って `std::lock_guard` を書きます。
`mutex_` も `mutable` です。`const` メンバ関数でもロックするからです。

**ここまでは「本の構造をそのまま移した」版です。C++ ではもっと良い書き方があります。**

## 21.2 C++ の Proxy は継承ではなく `operator->`

結城本の `PrinterProxy` は `Printable` を実装しています。
つまり「本人と同じインタフェースを持つ」ことでなりすましています。

C++ には、**インタフェースを 1 つも共有せずになりすます**方法があります。

```cpp
class CalibrationProxy
{
public:
  const CalibrationTable * operator->() const;   // これだけ
};
```

これで `proxy->entry(3)` と書けます。`CalibrationTable` のメソッドが 100 個あっても、
Proxy 側に 1 つも書きません。**`Printable` に相当する基底クラスも要りません。**

なぜ動くのか。`proxy->entry(3)` はコンパイラによって

```
proxy.operator->()->entry(3)
```

に書き換えられるからです。Proxy が返したポインタに `->` が続くだけです。

### あなたは既にこれを毎日使っている

```cpp
std::unique_ptr<Motor> motor = std::make_unique<Motor>();
motor->set_duty(0.5);
```

`std::unique_ptr` は `Motor` を継承していません。それでも `motor->set_duty(...)` と書けます。
`operator->` と `operator*` を持っているからです。

> **`std::unique_ptr` も `std::shared_ptr` も Proxy パターンの実装です。**
> `shared_ptr` は「参照カウントというアクセス制御」を挟む Proxy、
> `weak_ptr` は「生きているか検査してから通す」Proxy（`lock()` が明示的なだけ）です。

第 20 章で `shared_ptr` を「共有の道具」として使いました。同じものを、
今度は「アクセスを挟む場所」として見ます。

### 継承版と `operator->` 版の使い分け

| | 継承版（結城本の形） | `operator->` 版 |
| --- | --- | --- |
| 本人と同じ型として渡せるか | 渡せる（`Printable &` として） | 渡せない |
| 本体のメソッドが増えたら | Proxy にも足す | **何もしなくてよい** |
| 仮想関数のコスト | かかる | ゼロ |
| メソッドごとに違う処理を挟めるか | 挟める | 挟めない（全部同じ処理） |
| マイコンで使えるか | vtable のコストがかかる | かかる要素が無い |

**「本人として渡す必要があるか」で決まります。** 渡す必要が無いなら `operator->` 版です。
そして部活のライブラリでは、渡す必要が無いことの方が多い。

## 21.3 `operator->` の連鎖（drill-down）— C++ 独自の規則

ここが `operator->` の一番おもしろいところです。

> **`operator->` の戻り値がポインタでなければ、その戻り値に対してもう一度 `operator->` を呼ぶ。
> ポインタが返るまで繰り返す。**

他の演算子にこんな規則はありません。`operator+` の戻り値にもう一度 `+` は掛かりません。

何が嬉しいのか。**「本体にアクセスする直前と直後」に処理を挟めます。**

```cpp
class Guard
{
public:
  explicit Guard(Real * real) : real_(real) { /* 入口の処理 */ }
  ~Guard() { /* 出口の処理 */ }
  Real * operator->() const { return real_; }   // ここでポインタ。連鎖が止まる
private:
  Real * real_;
};

class Proxy
{
public:
  Guard operator->() const { return Guard{real_}; }   // ポインタではない。連鎖が続く
private:
  Real * real_;
};
```

`proxy->work()` はこう展開されます。

1. `Proxy::operator->()` → `Guard`（一時オブジェクト。ここでコンストラクタが走る）
2. `Guard` はポインタではないので、もう一度 `->` を掛ける
3. `Guard::operator->()` → `Real *`（ポインタ。止まる）
4. `Real::work()` を呼ぶ
5. **式が終わるので `Guard` が破棄される**（デストラクタが走る）

一時オブジェクトの寿命は**式の終わり（セミコロン）まで**です。
つまり **`work()` の呼び出しは必ずコンストラクタとデストラクタに挟まれます。**

## 21.4 誰が本体を所有するのか

Proxy には 2 種類あり、所有権が正反対です。

| 種類 | 例 | 本体の持ち方 |
| --- | --- | --- |
| **Virtual Proxy**（遅延生成） | 重い較正テーブル | `mutable std::unique_ptr<Real>`。Proxy が所有する |
| **Protection Proxy**（検査・記録） | レジスタアクセス | `Real &` か `Real *`。**Proxy は所有しない** |

Virtual Proxy は自分で作るので所有します。`unique_ptr` です。
Protection Proxy は既にあるものを包むだけなので、参照を持ちます。**所有しません。**

そして参照を持つ側には、第 1 章から繰り返している制約が付きます。

```cpp
SafeRegisterProxy make_proxy()
{
  RegisterFile file;              // ローカル
  return SafeRegisterProxy{file}; // file はここで死ぬ
}                                 // 返った Proxy は死んだ本体を指している
```

**Java なら GC が生かしてくれます。C++ では落ちます。**
Proxy を返す関数を書くときは、本体の寿命を必ず確認してください。

### `friend` を使うところ

課題の `RegisterAccess` は、`SafeRegisterProxy` が持つ非 `const` の本体に届く必要があります。
`file()` アクセサは `const RegisterFile &` を返すので、そこからは書き込めません。

選択肢は 3 つです。

| 方法 | 評価 |
| --- | --- |
| `const_cast` する | **やらない。** `const` を外すのは最後の手段 |
| 非 `const` の `raw_file()` を public に足す | 誰でも検査を迂回できる。Proxy の意味が消える |
| `RegisterAccess` を `friend` にする | **これ。** 迂回できるのは Proxy 自身が作った一時オブジェクトだけ |

`friend` は「カプセル化を壊す」と嫌われがちですが、
**Proxy と、その Proxy が作る補助オブジェクトのペア**は `friend` の正当な用途です。
標準ライブラリも同じことをしています。

## 21.5 Proxy と Decorator（第12章）は何が違うのか

構造はほとんど同じです。どちらも本体を包んで、呼び出しを転送します。**違うのは目的です。**

| | Decorator（第12章） | Proxy（第21章） |
| --- | --- | --- |
| 目的 | **振る舞いを足す** | 振る舞いは同じまま、**アクセスを制御する** |
| 呼ぶ側から見た結果 | 変わる（ログが増える、暗号化される） | 変わらない（ように見せる） |
| 本体の生成 | 外から渡される | **Proxy が決める**（遅延生成できる） |
| 何個も重ねるか | 重ねるのが前提 | ふつう 1 枚 |

第 12 章の `LogSink` にタイムスタンプを付ける Decorator は、**出力が変わります**。
この章の `CalibrationProxy` は、`entry(3)` の戻り値を 1 ミリも変えません。
変えるのは「いつ本体を作るか」だけです。

判断に迷ったら「**呼ぶ側から見て結果が変わるか**」で分けてください。
変わるなら Decorator、変わらないなら Proxy です。

## 21.6 標準ライブラリ／言語機能に同じものが無いか

**山ほどあります。この章は「標準にあるものを自分で書いて理解する」章です。**

| 標準のもの | どういう Proxy か |
| --- | --- |
| `std::unique_ptr` / `std::shared_ptr` | `operator->` を持つ最小の Proxy |
| `std::weak_ptr` | 生きているか検査してから通す。`lock()` が明示的な Proxy |
| `std::vector<bool>::reference` | ビット 1 個を `bool` に見せかける Proxy。**有名な地雷** |
| `std::optional` の `operator->` | 中身があるときだけ通す |
| `std::reference_wrapper` | 参照を値のように持ち回る |
| `std::lock_guard` | 「アクセス中だけロックする」の入れ物。21.3 の `Guard` そのもの |

### `std::vector<bool>` の話

```cpp
std::vector<bool> flags(4);
auto flag = flags[0];        // bool ではない。Proxy が返ってくる
flag = true;                 // flags[0] が書き換わる（コピーしたつもりなのに）
```

`std::vector<bool>` はビットを詰めて持っているので、`operator[]` が
`bool &` を返せません。代わりに Proxy を返します。
これが `auto` と組み合わさると挙動が直感に反します。
**Proxy を返す設計の落とし穴**として有名です。

自分で Proxy を書くときは、**「`auto` で受けたときに何が起きるか」を必ず確認**してください。

### `operator->` が一時オブジェクトを返す実用イディオム

21.3 の構造は、そのままスレッド安全なアクセサになります。

```cpp
template <typename T>
class Locked
{
public:
  class Handle
  {
  public:
    Handle(T & object, std::mutex & mutex) : object_(object), lock_(mutex) {}
    Handle(const Handle &) = delete;
    T * operator->() const { return &object_; }

  private:
    T & object_;
    std::lock_guard<std::mutex> lock_;
  };

  Handle operator->() { return Handle{object_, mutex_}; }

private:
  T object_;
  std::mutex mutex_;
};

Locked<SensorBuffer> buffer;
buffer->push(10);        // ロック → push → アンロック。書き忘れようがない
```

`buffer->push(10)` と書くだけで、`push` の間だけロックが取れています。
**ロックを取り忘れることが構文上できません。**
`std::mutex` を直接持ってメンバ関数ごとに `lock_guard` を書くより安全です。

ただし落とし穴が 1 つあります。

```cpp
if (buffer->size() > 0) {     // ここでロック→解放
  buffer->pop();              // 別のロック。この間に他スレッドが空にできる
}
```

**式ごとにロックが切れます。** 複数の操作をまとめて排他したいときは、
`Handle` を明示的に変数に受けてください。

```cpp
auto handle = buffer.operator->();   // handle が生きている間ロックが続く
```

## 21.7 手元で試す

`operator->` の連鎖と、一時オブジェクトの寿命を目で見ます。
**出力の順番を予想してから**実行してください。

```cpp
#include <iostream>
#include <memory>

class Real
{
public:
  void work() { std::cout << "  Real::work\n"; }
};

// 1 段目の Proxy。ポインタではなく Guard を返す。
class Guard
{
public:
  explicit Guard(Real * real) : real_(real) { std::cout << "  Guard 生成\n"; }
  ~Guard() { std::cout << "  Guard 破棄\n"; }
  Guard(const Guard &) = delete;
  Real * operator->() const { return real_; }

private:
  Real * real_;
};

class Proxy
{
public:
  explicit Proxy(Real * real) : real_(real) {}
  Guard operator->() const { return Guard{real_}; }

private:
  Real * real_;
};

int main()
{
  Real real;
  Proxy proxy{&real};

  std::cout << "式の前\n";
  proxy->work();
  std::cout << "式の後\n";

  // std::unique_ptr も operator-> を持つ Proxy でしかない
  std::unique_ptr<Real> owned = std::make_unique<Real>();
  owned->work();
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: <code>proxy-&gt;work()</code> の 1 行で、何が何回呼ばれるか</summary>

```
式の前
  Guard 生成
  Real::work
  Guard 破棄
式の後
  Real::work
```

`proxy->work()` という **1 行**で、`Guard` の生成と破棄が起きています。
`Guard` を書いた覚えは呼ぶ側にはありません。`operator->` の連鎖が勝手にやっています。

注目すべきは順番です。`Guard 生成` → `Real::work` → `Guard 破棄`。
**本体の呼び出しが必ず挟まれています。** ここに `lock()` / `unlock()` を置けば、
呼ぶ側が何も書かなくてもロックが掛かります。

`Guard` はコピーもムーブもできないのに、`operator->` が値で返せています。
C++17 の**保証されたコピー省略**のおかげです。C++11 でコンパイルするとこうなります。

```
error: call to deleted constructor of 'A'
note: 'A' has been explicitly marked deleted here
```
</details>

## 21.8 マイコンでの結論

**Virtual Proxy（遅延生成）はマイコンでは使えません。** 理由は 1 つです。

```cpp
real_ = std::make_unique<CalibrationTable>(source_);   // ループの途中でヒープ確保
```

いつ確保が走るか分からないコードは、ベアメタルでは書けません。
確保に失敗したときの逃げ道も（`-fno-exceptions` なので）ありません。
較正テーブルは**起動時に固定領域へ読み込みます**。遅延させません。

**マイコンで実用になるのは、レジスタアクセスを包む Proxy です。**
`volatile` な番地への読み書きに、範囲チェックや単位変換を挟みます。

```cpp
#include <cstdint>
#include <cstdio>

// PC で動かすためのニセのペリフェラル。実機ではデータシートの番地。
volatile std::uint16_t g_pwm_duty = 0;

// レジスタ 1 本を包む Proxy。動的確保なし、例外なし、vtable なし。
// 読み書きの見た目は生のレジスタと同じ。挟まるのは範囲の丸めだけ。
class DutyRegister
{
public:
  static constexpr std::uint16_t kMax = 999;

  explicit DutyRegister(volatile std::uint16_t * reg) : reg_(reg) {}

  // 読み出し: 値のように使うと呼ばれる
  operator std::uint16_t() const { return *reg_; }

  // 書き込み: reg = 値 と書くと呼ばれる。範囲外は kMax に丸める
  DutyRegister & operator=(std::uint16_t value)
  {
    *reg_ = (value > kMax) ? kMax : value;
    return *this;
  }

  DutyRegister & operator=(const DutyRegister &) = delete;

private:
  volatile std::uint16_t * reg_;
};

int main()
{
  DutyRegister duty{&g_pwm_duty};

  duty = 500;
  std::printf("duty=%u raw=%u\n", static_cast<unsigned>(duty), static_cast<unsigned>(g_pwm_duty));

  duty = 5000;  // 範囲外。Proxy が丸める
  std::printf("duty=%u raw=%u\n", static_cast<unsigned>(duty), static_cast<unsigned>(g_pwm_duty));

  std::printf("sizeof(DutyRegister)=%zu\n", sizeof(DutyRegister));
  return 0;
}
```

実行結果です。

```
duty=500 raw=500
duty=999 raw=999
sizeof(DutyRegister)=8
```

**`duty = 5000;` が黙って 999 に丸められています。**
生のレジスタに直接書いていたら、上位ビットが別の意味を持つ場合に暴走します。

ここでの `operator->` は使っていません。**レジスタは 1 個の値なので、
`operator=` と `operator T()` の組み合わせが自然**です。
`operator->` は「複数のメンバを持つ本体」を包むときに使います。

### `volatile` を落とさないこと

```cpp
std::uint16_t * reg_;             // volatile を書き忘れた
```

これをやると、コンパイラは「同じ番地を 2 回読む意味は無い」と判断して
読み出しをまとめてしまいます。**ハードウェアが勝手に値を変える**という前提が消えます。
Proxy の中に `volatile` を閉じ込めるのは、**書き忘れを 1 か所に減らす**という効果もあります。

### 番地をテンプレート引数にすると 1 バイトになる

上の例は `volatile std::uint16_t *` を持つので 8 バイトです。
番地がコンパイル時に決まっているなら、テンプレート引数にできます。

```cpp
template <std::uint32_t Address, std::uint16_t Mask = 0xffffu>
class RegisterProxy
{
  static volatile std::uint16_t * reg()
  {
    return reinterpret_cast<volatile std::uint16_t *>(static_cast<std::uintptr_t>(Address));
  }
  // ...
};
```

この形なら `sizeof` は **1**（空クラスの最小サイズ）です。RAM を 1 バイトも使いません。
実測しました。

```
sizeof(RegisterProxy) = 1
```

**マイコンで Proxy を書くなら、まずこの形を検討してください。**

## 21.9 ROS 2 での結論（補足）

rclcpp には「Proxy」という名前のクラスは出てきませんが、Proxy そのものは大量にあります。

- `rclcpp::Node::SharedPtr` — `shared_ptr` なので `operator->` を持つ Proxy
- `rclcpp::Publisher<T>::SharedPtr` — 同上。`pub->publish(msg)` は Proxy 越しの呼び出し
- `rclcpp::LoanedMessage` — DDS が貸したメモリを包み、デストラクタで返す。
  **21.3 の `Guard` と同じ構造**です（第 14 章 zero copy の課題で触れました）

ROS 2 では動的確保も例外も使えるので、Virtual Proxy（重いリソースの遅延生成）も選べます。
たとえば「大きな地図データを、最初に問い合わせが来たときだけ読む」といった用途です。

ただしノードのコールバックはシングルスレッドとは限りません。
**遅延生成する Proxy はスレッド安全にしてください。** 21.1 の `mutable std::mutex` が要ります。
2 つのスレッドが同時に最初のアクセスをすると、本体が 2 回作られます。

## 21.10 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `error: no viable overloaded '='`（`unique_ptr` で） | `operator->` が `const` なのに `real_` が `mutable` でない |
| `proxy->method()` がコンパイルできない | `operator->` の戻り値がポインタでも Proxy でもない。`int` などを返している |
| `operator->` が無限に呼ばれてコンパイルが終わらない | 連鎖の終端がポインタになっていない。自分自身を返している |
| 遅延生成したのに毎回作り直される | `if (!real_)` の判定が抜けている |
| 一時オブジェクトのロックが効いていない | 式ごとに解放されている（21.6）。`Handle` を変数で受ける |
| `auto x = proxy[0];` の挙動がおかしい | Proxy を `auto` で受けている。`std::vector<bool>` と同じ罠 |
| Proxy を返す関数で落ちる | Protection Proxy が本体より長生きしている（21.4） |
| マイコンで最適化すると読み出しが消える | `volatile` を落としている |
| Proxy に機能を足したくなってきた | それは Proxy ではなく Decorator（21.5）。混ぜない |

## 21.11 対応する課題

```bash
./drill run dp21
```

`exercises/dp21_proxy/src/calibration_proxy.cpp` に、

1. **`CalibrationProxy::operator->()`** — `mutable std::unique_ptr` による遅延生成
2. **`RegisterAccess`** のコンストラクタ／デストラクタ／`operator->` — 連鎖の 2 段目
3. **`SafeRegisterProxy::read()` / `write()`** — 範囲検査と記録

を実装します。テストは、

- 最初のアクセスまで本体が作られないこと（生成カウンタで確認）
- 二度目以降に作り直されないこと（アドレスと生成カウンタで確認）
- **`const` な Proxy からも遅延生成できること**（`mutable` の確認）
- 範囲外のアクセスが**本体に届いていない**こと（本体側のアクセス回数で確認）
- `operator->` の連鎖が起きていること（`static_assert` で戻り値の型を確認）
- 一時オブジェクトの寿命が本体アクセスを挟んでいること（記録の順番で確認）

を見ます。

## 21.12 この章のまとめ

- **`std::unique_ptr` / `std::shared_ptr` は Proxy そのもの。** `operator->` を持つだけ
- C++ の Proxy は継承ではなく **`operator->` を書く**。本体のメソッドが増えても Proxy は無変更
- **`operator->` はポインタが返るまで繰り返し呼ばれる**（drill-down）。C++ 独自の規則
- その規則と一時オブジェクトの寿命で、**本体アクセスの前後に処理を挟める**（ロック、記録）
- 遅延生成は `mutable std::unique_ptr` +  `const` な `operator->`。`mutable` の正当な用途
- Virtual Proxy は本体を**所有する**。Protection Proxy は**所有しない**（寿命に注意）
- **Decorator は振る舞いを足す。Proxy は同じに見せかけてアクセスを制御する**
- マイコンでは遅延生成は使わない。**`volatile` なレジスタを包む Proxy** が実用
- Proxy を `auto` で受けると挙動が変わる。`std::vector<bool>` の罠と同じ

---

前: [20. Flyweight](20_Flyweight.md) ／ 次: 22. Command（準備中）
