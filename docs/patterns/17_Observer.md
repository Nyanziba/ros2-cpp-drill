# 17. Observer

> **結城本 第17章 対応。** `Observer` インタフェースと `NumberGenerator` を手元に開いてください。
>
> **この章のねらい**: Java 版の `addObserver(observer)` は `void` を返して終わりです。
> **C++ で同じことを書くと壊れます。** 購読者が Subject より先に死ぬと、
> Subject の中に宙に浮いたポインタが残るからです。Java では GC が生かしておくので落ちません。
> この章では、その壊れ方を**実際に走らせて観測**してから、
> 解決策を 3 つ実装して比べ、最後に「購読を表す RAII トークンを返す」形に着地します。
> **この講習で一番長い章です。** 所有権と寿命の話がここに全部集まります。

## 17.1 Java 版をそのまま C++ にすると

結城本の `Observer` と `Subject` はこうです。

```java
public interface Observer {
    public abstract void update(NumberGenerator generator);
}

public abstract class NumberGenerator {
    private ArrayList<Observer> observers = new ArrayList<Observer>();
    public void addObserver(Observer observer)    { observers.add(observer); }
    public void deleteObserver(Observer observer) { observers.remove(observer); }
    public void notifyObservers() {
        for (Observer o : observers) { o.update(this); }
    }
}
```

C++ に素直に移すとこうなります。部活のライブラリらしく、距離センサの値を配る形にしておきます。

```cpp
class SensorObserver
{
public:
  virtual ~SensorObserver() = default;          // 変更点1
  virtual void on_sample(int value_mm) = 0;     // 変更点2
};

class SensorHub
{
public:
  void add_observer(SensorObserver * observer)  // 変更点3
  {
    observers_.push_back(observer);
  }

  void notify(int value_mm)
  {
    for (SensorObserver * observer : observers_) {
      observer->on_sample(value_mm);
    }
  }

private:
  std::vector<SensorObserver *> observers_;
};
```

### 変更点1: 仮想デストラクタ

1 章から 16 章まで毎回書いている話なので短く。純粋仮想関数を書いたら仮想デストラクタも書きます。
（この章の 17.9「マイコンでの結論」で、**唯一の例外**が出てきます）

### 変更点2: `update(this)` を `on_sample(value_mm)` にした

結城本は `update(NumberGenerator generator)` で、観測者が `generator.getNumber()` を
呼びに行く**プル型**です。C++ でプル型にすると、

```cpp
virtual void update(const SensorHub & hub) = 0;
```

となり、**`SensorObserver` が `SensorHub` を知っている**ことになります。
ヘッダが相互に依存し、`SensorHub` の型を変えるだけで全観測者が再コンパイルになります。
値を引数で渡す**プッシュ型**にすれば、この依存が消えます。

```cpp
virtual void on_sample(int value_mm) = 0;   // SensorHub を知らなくていい
```

Java では「参照を渡すだけ」なので気になりませんが、
C++ では**ヘッダ依存とコンパイル時間**が実際のコストとして効きます。この章はプッシュ型で書きます。

### 変更点3: `Observer` を `SensorObserver *` にした

Java の `ArrayList<Observer>` は参照の配列です。C++ で `std::vector<SensorObserver>` と書くと、

- 抽象クラスなので**そもそもコンパイルが通らない**（`std::vector` は要素を値で持つ）
- 通ったとしてもコピーが走り、**スライシング**する

ので、ポインタか参照か `shared_ptr` のどれかにするしかありません。
上のコードでは**生ポインタ**にしました。**そしてここから壊れます。**

## 17.2 本題 — 購読者が先に死ぬ

[0. 使う前に](00_使う前に.md) の 0.4 で予告した話です。

```cpp
{
  Display display;
  hub.add_observer(&display);
}                      // display が死ぬ。hub はまだ &display を持っている
hub.notify(200);       // 未定義動作
```

**Java なら GC が `display` を生かしておくので落ちません**（代わりに永久にリークします。
Java でも `deleteObserver` の呼び忘れはメモリリークの定番です）。
**C++ では `display` は本当に消えます。**

問題は、**たいてい落ちない**ことです。実際に走らせます。

```cpp
class Display : public SensorObserver
{
public:
  void on_sample(int value_mm) override { std::printf("display %d\n", value_mm); }
};

class Logger : public SensorObserver
{
public:
  void on_sample(int value_mm) override { std::printf("logger  %d\n", value_mm); }
};

int main()
{
  SensorHub hub;

  Display * display = new Display{};
  hub.add_observer(display);
  std::printf("display addr = %p\n", static_cast<void *>(display));
  hub.notify(100);
  delete display;                     // display はここで死ぬ

  Logger * logger = new Logger{};     // 同じ番地が再利用される
  std::printf("logger  addr = %p\n", static_cast<void *>(logger));
  hub.notify(200);                    // display のつもりで logger を呼ぶ
  delete logger;
  return 0;
}
```

実行結果です（Apple clang / macOS）。

```
display addr = 0x1034fa470
display 100
logger  addr = 0x1034fa470
logger  200
```

`display` に通知したつもりが、**`logger` が呼ばれています。**
`delete` した番地に別のクラスが載り、そこにある vtable ポインタをたどった結果です。

**プログラムは落ちません。例外も出ません。エラーメッセージも出ません。**
ただ、通知先が知らないうちに別のクラスにすり替わります。
ロボットの制御ループでこれが起きたときに何が起きるかは、想像がつくと思います。

サニタイザ（`-fsanitize=address`）が使える環境なら
`heap-use-after-free` として即座に報告されます。使えるなら必ず使ってください。
**この種のバグは、道具を入れないと見えません。**

## 17.3 誰が購読を所有するのか — 解決策 3 つ

`add_observer` が `void` を返すのが根本原因です。
**「この購読は誰のものか、いつ終わるのか」がコードのどこにも書かれていません。**
1 章から繰り返している「所有権を型で表明する」がここでも効きます。

対処は 3 つあります。

| 方法 | 書き方 | 代償 |
| --- | --- | --- |
| 購読解除を義務にする | 観測者のデストラクタで `hub.remove_observer(this)` | **書き忘れる。** しかも Subject の寿命も要る |
| `weak_ptr` で持つ | Subject が `std::vector<std::weak_ptr<SensorObserver>>` を持つ | 観測者が **`shared_ptr` で管理されている必要がある** |
| トークン方式（RAII） | `subscribe()` が購読を表す RAII オブジェクトを返し、破棄で自動解除 | 実装が要る。**最も安全** |

順に見ます。

### 方法1: 購読解除を義務にする

```cpp
class Display : public SensorObserver
{
public:
  explicit Display(SensorHub & hub) : hub_(hub) { hub_.add_observer(this); }
  ~Display() override { hub_.remove_observer(this); }   // 忘れたら 17.2 が起きる

  void on_sample(int value_mm) override { /* ... */ }

private:
  SensorHub & hub_;
};
```

一見これで解決です。しかし 2 つ問題が残ります。

1. **書き忘れる。** 観測者を 1 つ書き足すたびにデストラクタを書く必要があり、
   忘れてもコンパイルは通ります。しかも忘れたことは 17.2 のとおり実行しても分かりません
2. **`SensorHub` が先に死ぬと、今度は `~Display()` の側が壊れます。**
   `hub_` が参照している `SensorHub` はもう無いのに `remove_observer` を呼びます

2 番目が効きます。**寿命の前後関係を、人間が覚えておかないといけません。**
「Hub は必ず観測者より長生きさせること」というコメントで済ませる手もありますが、
それは規約であって、コンパイラは何も守ってくれません。

### 方法2: `weak_ptr` で持つ

Subject 側が `weak_ptr` で持てば、購読者が死んだことを Subject が検出できます。

```cpp
class SensorHub
{
public:
  void add_observer(const std::shared_ptr<SensorObserver> & observer)
  {
    observers_.push_back(observer);
  }

  void notify(int value_mm)
  {
    // 死んだ購読者を掃除しながら回す
    std::vector<std::weak_ptr<SensorObserver>> alive;
    alive.reserve(observers_.size());
    for (const std::weak_ptr<SensorObserver> & weak : observers_) {
      if (const std::shared_ptr<SensorObserver> observer = weak.lock()) {
        observer->on_sample(value_mm);
        alive.push_back(weak);
      }
    }
    observers_.swap(alive);
  }

private:
  std::vector<std::weak_ptr<SensorObserver>> observers_;
};
```

実行するとこうなります。

```
display 100
生き残った購読 = 0
```

購読者が死んだあとの `notify(200)` は**何も起きません。落ちません。**正しい挙動です。

`lock()` が肝です。通知している間だけ `shared_ptr` に昇格するので、
**`on_sample` の実行中に他のスレッドが観測者を解放しても安全**です。
16 章（Mediator）で `shared_ptr` の循環を切るために `weak_ptr` を使いましたが、
ここでは**寿命の検出**に使っています。同じ道具の別の使い方です。

代償ははっきりしています。

- **観測者が `shared_ptr` で管理されている必要があります。** スタックに置いた
  `Display display;` は登録できません。`std::make_shared<Display>()` が必須になります
- **通知のたびに `lock()` の参照カウント操作**（アトミック演算）が入ります
- マイコンでは `shared_ptr` 自体が使えないことが多い（制御ブロックのヒープ確保）

「観測者は全部 `shared_ptr` で作る」という方針を**ライブラリ全体に強制**できるなら、
これは十分実用的な選択です。ROS 2 側のコードならまず問題になりません。

### 方法3: トークン方式（この章の本命）

`subscribe()` が**購読を表す値**を返します。その値が死んだら購読が切れます。

```cpp
{
  Subscription sub = hub.subscribe(&display);   // 購読が始まる
  hub.publish(100);                             // display に届く
}                                               // sub が死ぬ → 購読が切れる
hub.publish(200);                               // display には届かない
```

`std::unique_ptr` や `std::lock_guard` と同じ考え方です。
**「後始末が要る操作は、後始末をするオブジェクトを返す」。**

これが優れている点は 3 つあります。

1. **書き忘れようがない。** 返り値を捨てれば購読は始まらない（＝その場で切れる）。
   観測者にデストラクタを書く必要が無い
2. **観測者を `shared_ptr` にしなくていい。** スタックに置いたオブジェクトでも使える
3. **Subject が先に死んでも安全にできる。** トークンが購読リストを `weak_ptr` で
   見ておけば、Subject が死んだあとの `~Subscription()` は何もしない

3 番目のために、購読リストの実体を `SensorHub` の中に直接置かず、
**`shared_ptr` で持った別のオブジェクト（`Registry`）に置きます。**
トークン側はそれを `weak_ptr` で見ます。課題のヘッダはこの形です。

```cpp
class SensorHub
{
  std::shared_ptr<detail::Registry> registry_;   // 実体はこちら
};

class Subscription
{
  std::weak_ptr<detail::Registry> registry_;     // 覗くだけ
  std::size_t id_ = 0;
};
```

| 誰が死ぬ | 何が起きるか |
| --- | --- |
| 観測者（トークンごと）が先に死ぬ | `~Subscription()` が `Registry` から自分の購読を消す |
| `SensorHub` が先に死ぬ | `Registry` も死ぬ → トークンの `lock()` が空を返す → **何もしない** |

**両方向とも安全です。** 方法1 が守れなかったのがここです。

### トークンは `unique_ptr` と同じ性質にする

購読は 1 つしかありません。コピーされたら、どちらのデストラクタで解除するのか決まりません。

```cpp
Subscription(const Subscription &) = delete;
Subscription & operator=(const Subscription &) = delete;
Subscription(Subscription && other) noexcept;
Subscription & operator=(Subscription && other) noexcept;
```

**ムーブコンストラクタでは、ムーブ元を必ず空にしてください。**

```cpp
Subscription::Subscription(Subscription && other) noexcept
: registry_(std::move(other.registry_)),
  id_(other.id_)
{
  other.registry_.reset();
  other.id_ = 0;          // ← これを忘れると、ムーブ元が死んだ瞬間に購読が切れる
}
```

`unique_ptr` のムーブでポインタを `nullptr` にするのと同じ話です。
`std::move` は**名前が動詞なだけで、何も動かしません**。空にするのは自分の仕事です。

## 17.4 通知中に購読リストが変わる

もう 1 つ、Java でも起きるが C++ では壊れ方が違う問題です。

```cpp
void notify(int value_mm)
{
  for (SensorObserver * observer : observers_) {   // ← ここ
    observer->on_sample(value_mm);                 // ← この中で remove_observer されたら？
  }
}
```

Java なら `ConcurrentModificationException` が飛びます。**例外なので気づけます。**
C++ は何も言いません。走らせます。3 つの観測者が、通知を受けたら自分を解除します。

```cpp
class SelfRemoving : public SensorObserver
{
public:
  void on_sample(int value_mm) override
  {
    std::printf("observer %d got %d\n", id_, value_mm);
    hub_->remove_observer(this);   // 通知の最中にリストを変更する
  }
  // ...
};
```

実行結果です。

```
observer 1 got 42
observer 3 got 42
observer 3 got 42
notify から戻ってきた
```

観測者 1・2・3 に 1 回ずつ届くはずが、**2 番は飛ばされ、3 番が 2 回呼ばれています。**
そして最後の 1 回は、すでに `erase` で縮んだ `vector` の**範囲外**を読んでいます。
これも落ちません。

原因は `erase` がイテレータを無効化することです。
`std::vector` の `erase` は後ろの要素を前に詰めるので、
range-based for が持っているイテレータが 1 つ先の要素を指してしまいます。

**対処は 2 つあります。**

| 方法 | やり方 | 代償 |
| --- | --- | --- |
| コピーしてから回す | `auto snapshot = observers_; for (auto * o : snapshot) { ... }` | **通知のたびにコピー**（ヒープ確保）。解除済みの観測者にも通知が飛ぶ |
| 遅延削除 | 解除は「印を付ける」だけ。実際の削除は通知ループが終わってから | フラグが 1 個増える。**確保はゼロ** |

課題では**遅延削除**を実装します。マイコンでも使える方（確保が無い方）を選びます。

```cpp
void Registry::remove(std::size_t id)
{
  for (Entry & entry : entries) {
    if (entry.id == id) {
      entry.observer = nullptr;   // 印を付けるだけ。erase しない
      break;
    }
  }
  if (!notifying) { compact(); }  // 通知中でなければ、その場で詰め直す
}
```

通知ループ側も**添字で回します**。

```cpp
const std::size_t count = entries.size();     // 件数はループ前に固定する
for (std::size_t i = 0; i < count; ++i) {
  SensorObserver * observer = entries[i].observer;
  if (observer != nullptr) { observer->on_sample(value_mm); }
}
```

参照やイテレータを取り置きしないのは、**通知の最中に `subscribe()` されると
`std::vector` が再確保される**からです。添字なら再確保されても正しく動きます。
件数を先に固定しているのは、**通知中に増えた購読をその回で呼ばない**ためです
（呼んでしまうと、まだ準備が終わっていない観測者に通知が飛びます）。

## 17.5 通知が循環する

0.4 で予告したもう 1 つです。A の通知先が B に通知し、B の通知先が A に通知します。

```cpp
a.add_observer(&to_b);   // a が更新されたら b に通知
b.add_observer(&to_a);   // b が更新されたら a に通知
a.notify(1);             // 戻ってこない
```

無限ループなので、そのまま走らせると観測できません。
深さを数えて 8 段で強制的に止める仕掛けを入れて測りました。

```
depth 1
depth 1
depth 2
depth 2
depth 3
depth 3
...
depth 8
depth 8
depth 9 に到達。止めます
```

**止める仕掛けが無ければスタックを食い尽くすまで進みます。**
Java でも同じことが起きますが、C++ ではスタックオーバーフローが
（多くの環境で）**エラーメッセージ無しの即死**になる点が違います。

対処は再入防止フラグ 1 つです。

```cpp
void SensorHub::publish(int value_mm)
{
  if (registry_->notifying) { return; }   // 通知中の再入は無視する
  // ...
}
```

**フラグの戻し忘れが次の事故**です。`on_sample` が例外を投げると `notifying` が
true のまま残り、その Subject は**二度と通知しなくなります**。
デストラクタで戻す小さな RAII をその場で書いてください。

```cpp
struct NotifyingGuard
{
  explicit NotifyingGuard(detail::Registry & target) : registry(target)
  {
    registry.notifying = true;
  }
  ~NotifyingGuard() { registry.notifying = false; }

  NotifyingGuard(const NotifyingGuard &) = delete;
  NotifyingGuard & operator=(const NotifyingGuard &) = delete;

  detail::Registry & registry;
};
```

Java の `try` / `finally` に相当するものを、C++ ではデストラクタで書きます。
**`finally` が無い代わりに RAII があります。** これは C++ の方が強い部分です。

なお、再入を「無視する」以外に「キューに積んで通知後に流す」設計もあります。
実装は増えますが通知が落ちません。部活のライブラリなら、まず無視で十分です。
**落とすなら、落としたことがログに残るようにしてください。**

## 17.6 `std::function` を使うコールバック方式との比較

「観測者クラスを毎回書くのは面倒だ。ラムダを登録させればいいのでは」と思ったはずです。

```cpp
class SensorHub
{
public:
  void add_callback(std::function<void(int)> callback)
  {
    callbacks_.push_back(std::move(callback));
  }

private:
  std::vector<std::function<void(int)>> callbacks_;
};

hub.add_callback([&display](int mm) { display.show(mm); });   // 確かに楽
```

登録は圧倒的に楽です。継承も要りません。**しかし解除できません。**

```cpp
auto cb = [](int v) { (void)v; };
callbacks_.push_back(cb);
for (auto it = callbacks_.begin(); it != callbacks_.end(); ++it) {
  if (*it == cb) { callbacks_.erase(it); break; }   // これを書きたい
}
```

```
error: invalid operands to binary expression
      ('std::function<void (int)>' and '(lambda at fn.cpp:7:13)')
```

**`std::function` は同値比較できません。**
`nullptr` との比較しか定義されていないからです（中に入っているものが
何であるかを標準は知らないので、比較のしようがありません）。
つまり `std::function` を貯めた瞬間に、**「誰が登録したか」がコードから消えます。**

さらに、ラムダがキャプチャしている参照の寿命は誰も見張っていません。

```cpp
{
  Display display;
  hub.add_callback([&display](int mm) { display.show(mm); });
}                     // display が死ぬ。ラムダは死んだ display を掴んだまま
hub.publish(200);     // 17.2 と全く同じ未定義動作
```

**問題は 1 ミリも解決していません。** ただ、宙に浮いた参照が
クラスのメンバからラムダのキャプチャに移動して、**より見えにくくなっただけ**です。

だから**トークンが要ります**。`add_callback` もトークンを返すようにすれば、
`std::function` 方式でも安全になります。実際、まともなシグナル/スロットの
ライブラリ（Boost.Signals2 など）は必ず「接続」を表すオブジェクトを返します。

| | 仮想関数（`SensorObserver`） | `std::function` |
| --- | --- | --- |
| 登録の手軽さ | クラスを書く必要がある | ラムダ 1 行 |
| 誰が登録したか | ポインタで分かる | **分からない**（比較できない） |
| ヒープ確保 | 無し | **キャプチャが大きいと走る** |
| マイコン | 使える | 実質使えない |
| 解除 | ポインタか id で消せる | **トークンが無いと不可能** |

課題は仮想関数版で書きます。`std::function` 版は 22 章（Command）でもう一度出てきます。

## 17.7 標準ライブラリに同じものが無いか

**C++ の標準ライブラリに Observer はありません。**
`std::observer_ptr` は名前が似ていますが**全く関係ありません**（非所有ポインタを
表すだけの型で、C++17 にも入っていません）。

近いものとしては、

- **`std::weak_ptr`** — 「相手が生きているか確認してから触る」部品。17.3 の方法2 そのもの
- **`std::function`** — 通知先を型消去して保持する部品。17.6 のとおり解除は自前
- **`std::condition_variable`** — スレッド間の「待つ / 起こす」。用途が違いますが、
  「状態が変わったことを他に知らせる」という点は同じです

標準に無いので、**シグナル/スロットは外部ライブラリを使うのが実務では普通**です。
Boost.Signals2、Qt の `signals` / `slots`、`sigslot` などが該当します。
いずれも**接続オブジェクト（＝トークン）を返す**設計になっています。

部活のライブラリで、Observer が 1 か所しか要らないなら**自分で書いた方が速い**です
（課題のコードで 150 行程度）。3 か所以上に増えて、スレッド安全性まで欲しくなったら
ライブラリを検討してください。**その判断のために、一度自分で書いておきます。**

## 17.8 手元で試す

課題を解く前に、これをコンパイルして**出力を予想してから**実行してください。
17.4 の「通知中にリストが変わる」を最小の形にしたものです。

```cpp
#include <algorithm>
#include <cstdio>
#include <vector>

class SensorObserver
{
public:
  virtual ~SensorObserver() = default;
  virtual void on_sample(int value_mm) = 0;
};

class SensorHub
{
public:
  void add_observer(SensorObserver * observer) { observers_.push_back(observer); }

  void remove_observer(SensorObserver * observer)
  {
    observers_.erase(
      std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
  }

  void notify(int value_mm)
  {
    for (SensorObserver * observer : observers_) {
      observer->on_sample(value_mm);
    }
  }

private:
  std::vector<SensorObserver *> observers_;
};

class SelfRemoving : public SensorObserver
{
public:
  SelfRemoving(SensorHub * hub, int id)
  : hub_(hub),
    id_(id)
  {
  }

  void on_sample(int value_mm) override
  {
    std::printf("observer %d got %d\n", id_, value_mm);
    hub_->remove_observer(this);
  }

private:
  SensorHub * hub_;
  int id_;
};

int main()
{
  SensorHub hub;
  SelfRemoving a{&hub, 1};
  SelfRemoving b{&hub, 2};
  SelfRemoving c{&hub, 3};
  hub.add_observer(&a);
  hub.add_observer(&b);
  hub.add_observer(&c);

  hub.notify(42);
  std::puts("notify から戻ってきた");
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 何行出るか。どの観測者が何回呼ばれるか</summary>

3 行出て 1・2・3 が 1 回ずつ、ではありません。手元ではこうなりました。

```
observer 1 got 42
observer 3 got 42
observer 3 got 42
notify から戻ってきた
```

**2 番が飛ばされ、3 番が 2 回呼ばれています。**

1 番が自分を消した時点で `vector` は `{2, 3}` に縮み、
ループのイテレータは 2 番目（＝ 3）を指したまま先に進みます。
3 番目の反復では、すでに縮んだ `vector` の**範囲外**を読んでいます。

コンパイル警告は 1 つも出ません。実行も正常終了します。
**この壊れ方は、テストを書かないと絶対に見つかりません。**
課題のテスト「通知中に他の観測者を解除しても落ちない」がこれを見ています。

なお、この出力は環境によって変わります（範囲外を読んでいるので当然です）。
**「手元では違う結果が出た」は、未定義動作の証拠であって反論ではありません。**
</details>

## 17.9 マイコンでの結論

課題のコードは**そのままではマイコンに載りません**。使えないものが 3 つあります。

| 使っているもの | なぜ載らないか |
| --- | --- |
| `std::vector` | 購読のたびにヒープ確保。断片化する |
| `std::shared_ptr` / `std::weak_ptr` | 制御ブロックのヒープ確保。アトミックなカウンタ操作も乗る |
| `std::function` | キャプチャが大きいとヒープ確保 |

置き換えます。**固定長配列に生ポインタを並べます。**

```cpp
class SensorObserver
{
public:
  virtual void on_sample(int value_mm) = 0;

protected:
  // 非仮想 protected デストラクタ。
  // 「基底クラスのポインタで delete しない」と決めたので、vtable に
  // デストラクタのスロットを増やしません。
  ~SensorObserver() = default;
};

template <std::size_t Capacity>
class SensorHub
{
public:
  bool subscribe(SensorObserver * observer)
  {
    if (observer == nullptr) {
      return false;
    }
    for (std::size_t i = 0; i < Capacity; ++i) {
      if (observers_[i] == nullptr) {
        observers_[i] = observer;
        return true;
      }
    }
    return false;                 // 満杯。例外は投げられないので bool で返す
  }

  void unsubscribe(SensorObserver * observer)
  {
    for (std::size_t i = 0; i < Capacity; ++i) {
      if (observers_[i] == observer) {
        observers_[i] = nullptr;  // 詰め直さない。通知ループ中でも安全
        return;
      }
    }
  }

  void publish(int value_mm)
  {
    if (notifying_) {
      return;                     // 再入防止
    }
    notifying_ = true;
    for (std::size_t i = 0; i < Capacity; ++i) {
      SensorObserver * observer = observers_[i];
      if (observer != nullptr) {
        observer->on_sample(value_mm);
      }
    }
    notifying_ = false;
  }

private:
  SensorObserver * observers_[Capacity] = {};
  bool notifying_ = false;
};
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti micro.cpp -o micro
```

これは `-fno-exceptions -fno-rtti` で警告ゼロで通ります。**確保はゼロです。**

ポイントが 4 つあります。

**1. 仮想デストラクタを書かない（この章の唯一の例外）**

`protected` な非仮想デストラクタにしています。
「基底クラスのポインタで `delete` しない」と設計で決めたので、
vtable のスロットを 1 つ節約しました。`protected` にしてあるので、

```cpp
void f(SensorObserver * p) { delete p; }
```

と書くとコンパイルエラーになります。

```
error: calling a protected destructor of class 'SensorObserver'
note: declared protected here
```

**規約違反がコンパイルエラーになる**なら、規約ではなく仕組みです。
`unique_ptr<SensorObserver>` を作ろうとしてもエラーになります。
マイコンでは観測者は静的に置くので、それで困りません。

**この形を ROS 2 側のコードに持ち込まないでください。** 動的に生成して
基底ポインタで解放する場面が普通にあるので、そちらは仮想デストラクタが正解です。

**2. 削除は「穴を空ける」だけ。詰め直さない**

固定長なので詰め直す理由がありません。`nullptr` の穴を飛ばして回るだけです。
遅延削除の実装が、そもそも要らなくなります。

**3. 満杯を `bool` で返す**

`-fno-exceptions` なので `throw` できません。戻り値で返します。
`[[nodiscard]]` を付けて、無視したら警告が出るようにするとなお良いです。

**4. ISR から `publish()` を呼ばない**

ここが一番の事故です。**割り込みハンドラの中で通知ループを回さないでください。**

- 通知先が何をするか分からない。`printf` や I2C 送信をされたら割り込みが長くなる
- `subscribe` / `unsubscribe` とデータ競合する。
  メインループが `observers_[i]` に書いている途中で割り込むと、
  中途半端なポインタを読みます
- `notifying_` フラグも割り込みでは守れません

割り込みは**値を置いてフラグを立てるだけ**にして、通知はメインループで行います。

```cpp
static volatile int g_latest_mm = 0;
static volatile bool g_has_new = false;

extern "C" void EXTI0_IRQHandler(void)
{
  g_latest_mm = read_distance_mm();
  g_has_new = true;               // 32bit 整列アクセスなので単発の代入は分割されない
}

int main()
{
  SensorHub<4> hub;
  Display display;
  hub.subscribe(&display);

  for (;;) {
    if (g_has_new) {
      g_has_new = false;
      const int value_mm = g_latest_mm;   // ローカルにコピーしてから使う
      hub.publish(value_mm);              // 通知はメインループ側
    }
  }
}
```

**`volatile` について 1 つ念を押します。**

`volatile` は「コンパイラに最適化で消させない・並べ替えさせない」だけの指定です。
**アトミック性は保証しません。同期にも使えません。**
上のコードが成立しているのは、

- 書くのは ISR だけ、読むのはメインループだけ、と**役割が分かれている**
- `int` と `bool` の**単発の読み書き**しかしていない（Cortex-M の整列した
  32bit / 8bit アクセスは分割されません）

の 2 つが揃っているからです。`g_counter++` のような
読み・加算・書きの 3 段は**割り込みで割れます**。
そこは割り込み禁止で囲むか、`std::atomic` を使ってください。

購読リスト（`observers_`）を割り込み中に触る可能性があるなら、
`subscribe` / `unsubscribe` を割り込み禁止で囲む必要があります。
**そうしなくて済むように、購読は起動時に全部済ませてください。**
これが一番安全で、一番速い設計です。

## 17.10 ROS 2 での結論（補足）

**ROS 2 の pub/sub は Observer そのものです。**
自分で Observer を書く前に、まずそれで足りないかを考えてください。

| Observer パターン | ROS 2 |
| --- | --- |
| Subject | Publisher（＋ミドルウェア） |
| Observer | Subscription のコールバック |
| `add_observer` | `create_subscription` |
| 購読解除 | Subscription オブジェクトを捨てる |

`create_subscription` が返すのは `rclcpp::Subscription<T>::SharedPtr` です。
**まさに 17.3 のトークン方式**で、`shared_ptr` の参照カウントが購読の寿命です。

```cpp
sub_ = this->create_subscription<sensor_msgs::msg::Range>(
  "range", 10, std::bind(&MyNode::on_range, this, std::placeholders::_1));
```

戻り値をメンバに保持しないと、その場で購読が切れます。
**返り値を捨てると購読が始まらない**のは課題の `Subscription` と全く同じ話です。

ノードの中で「センサ値を複数の内部クラスに配りたい」だけなら、
トピックを 1 本増やすより自前の Observer の方が軽いことがあります
（シリアライズもミドルウェアも通らないので）。
**ノードをまたぐならトピック、ノードの中なら自前**、が目安です。

なお、`rclcpp` のコールバック内から同じノードの API を呼ぶと
Executor の再入で詰まることがあります。**再入の問題は 17.5 と同種**です。
自分で書いても、フレームワークを使っても、同じところで転びます。

## 17.11 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| 観測者を消したあとの通知で、別のクラスの関数が呼ばれた | 17.2。生ポインタが宙に浮いている。トークン方式にする |
| 通知で観測者が 1 つ飛ばされ、別の 1 つが 2 回呼ばれた | 17.4。通知ループ中に `erase` している。遅延削除にする |
| 通知したらプログラムが戻ってこない | 17.5。通知が循環している。再入防止フラグを入れる |
| 一度例外が出たあと、二度と通知されなくなった | 再入防止フラグを戻す前に例外が抜けた。RAII で戻す |
| 通知の途中で `subscribe` したら落ちた | `vector` が再確保された。参照やイテレータを取り置きしない。添字で回す |
| 通知中に登録した観測者に、その回の通知が届いた | 件数をループ前に固定していない |
| `Subscription` をムーブしたら購読が切れた | ムーブ元を空にしていない。`unique_ptr` と同じ |
| `SensorHub` を破棄したあと、トークンの破棄で落ちた | トークンが Subject を生ポインタで持っている。`weak_ptr` にする |
| ラムダで登録したものを解除できない | 17.6。`std::function` は比較できない。トークンを返す設計にする |
| `std::vector<SensorObserver>` がコンパイルできない | 抽象クラスは値で持てない。ポインタか `shared_ptr` にする |
| 購読者が 2 回通知を受け取る | 同じ観測者を 2 回 `subscribe` している。id で数える |

## 17.12 対応する課題

```bash
./drill run dp17
```

`exercises/dp17_observer/src/sensor_hub.cpp` に、

1. `detail::Registry::remove()` / `compact()` — **遅延削除**
2. `Subscription` のデストラクタ・ムーブ・`reset()` / `active()` — **RAII トークン**
3. `SensorHub::subscribe()` / `publish()` / `observer_count()` — 再入防止つきの通知

を実装します。テストは 10 個で、通知が届くことだけでなく、

- **観測者が先に死んでも Subject が壊れない**（トークンが自動で解除する）
- **通知中に解除しても落ちない**（解除済みには通知が来ない）
- **通知が循環しても無限ループしない**
- **Subject が先に死んでもトークンの破棄が安全**

まで見ます。**17.2・17.4・17.5 で観測した壊れ方が、そのままテストになっています。**

## 17.13 この章のまとめ

- Java の `addObserver()` をそのまま移すと、**購読者が先に死んだ瞬間に壊れる**。
  しかも**落ちない**ので気づけない
- 原因は `add_observer` が `void` を返すこと。
  **「この購読は誰のもので、いつ終わるのか」が型に書かれていない**
- 対処は 3 つ。デストラクタで解除（書き忘れる・Subject の寿命が要る）、
  `weak_ptr` で持つ（観測者が `shared_ptr` 前提）、
  **トークン方式（RAII）が最も安全**
- トークンは `unique_ptr` と同じ性質にする。**コピー禁止・ムーブ可・ムーブ元は空**
- 購読リストの実体を `shared_ptr` で持ち、トークンは `weak_ptr` で見る。
  こうすると**どちらが先に死んでも安全**になる
- **通知ループ中に購読リストを `erase` しない。** 印を付けて、終わってから消す
- **参照やイテレータを取り置きしない。** 添字で回す。件数はループ前に固定する
- **再入防止フラグは RAII で戻す。** 例外で抜けても戻るようにする
- **`std::function` は比較できない**ので、登録したものを特定して消せない。
  だからトークンが要る
- マイコンでは固定長配列に生ポインタ。**確保ゼロ**。
  **ISR からは通知しない**。値を置いてフラグを立てるだけにする
- **`volatile` はアトミックではない。** 最適化を止めるだけ
- ROS 2 の pub/sub は Observer そのもの。`create_subscription` の戻り値が**トークン**

---

前: [16. Mediator](16_Mediator.md) ／ 次: 18. Memento（準備中）
