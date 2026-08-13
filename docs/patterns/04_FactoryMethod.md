# 4. Factory Method

> **結城本 第4章 対応。** `Factory` / `Product` と `IDCardFactory` / `IDCard` を手元に開いてください。
>
> **この章のねらい**: Java 版の `createProduct()` は `new IDCard(owner)` を返すだけです。
> C++ で同じことを書くと、**「返ってきたこれは誰が `delete` するのか」**が型に書かれません。
> 答えは `std::unique_ptr<Product>` を返すこと。ここまではすぐ納得できます。
> 本題はその先で、**共変戻り値型が `unique_ptr` では効かない**という C++ 固有の落とし穴と、
> **ヒープが使えないマイコンで生成をどう書くか**です。
> そしてもう 1 つ。**そもそも Factory を入れるべきでない場面**を先に決めます。

第 3 章を読んだ直後なら、この章の構造には見覚えがあるはずです。
**Factory Method は Template Method の一種**です。結城本もそう位置づけています。

| | Template Method（第3章） | Factory Method（第4章） |
| --- | --- | --- |
| 基底クラスが決めるもの | **処理の手順** | **生成の手順** |
| 派生クラスに任せるもの | 各ステップの中身 | **何を作るか** |
| C++ での主題 | NVI と仮想デストラクタ | **所有権の受け渡し** |

## 4.1 Java 版をそのまま C++ にすると

結城本の `Factory` はこうです。

```java
public abstract class Factory {
    public final Product create(String owner) {
        Product p = createProduct(owner);
        registerProduct(p);
        return p;
    }
    protected abstract Product createProduct(String owner);
    protected abstract void registerProduct(Product product);
}
```

C++ に素直に移すとこうなります。

```cpp
class Factory
{
public:
  virtual ~Factory() = default;

  // 手順は固定。virtual を書かない（= Java の final と同じ）
  std::unique_ptr<Product> create(const std::string & owner);

private:
  virtual std::unique_ptr<Product> create_product(const std::string & owner) = 0;
  virtual void register_product(const Product & product) = 0;
};
```

Java 版から変えた点が 4 つあります。

### 変更点1: `virtual ~Factory() = default;` を足した

第 1 章・第 3 章と同じです。**純粋仮想関数を 1 つでも書いたら仮想デストラクタ**。
23 章すべてで守ります。

### 変更点2: `final` を消し、`private virtual` にした

`public final void create()` → `public` かつ `virtual` を書かない、で同じ意味になります。
`protected abstract` → **`private virtual`**。第 3 章の NVI と同じ理由です。
「派生クラスから呼ばれる必要はない、オーバーライドされればよい」からです。

### 変更点3: `Product` を `std::unique_ptr<Product>` にした

**この章の本題です。** 4.2 で扱います。

### 変更点4: `registerProduct(Product p)` を `register_product(const Product & product)` にした

Java の引数はすべて参照なので、`registerProduct(p)` と書いても
**同じオブジェクトが渡ります**。C++ で `void register_product(Product product)` と書くと
コピーが走ります（`Product` が抽象クラスならそもそもコンパイルエラーです）。

そして `const Product &` にしたことで、**「登録する側は所有権を受け取らない」**が
型に書かれました。ここが `std::unique_ptr<Product>` を渡す形との違いです。

| 引数の書き方 | 意味 |
| --- | --- |
| `const Product & p` | **見るだけ。所有しない**（この章はこれ） |
| `std::unique_ptr<Product> p` | **所有権をもらう**。呼び出し側の手からは消える |
| `std::shared_ptr<Product> p` | 共同所有する |
| `Product * p` | **不明。C++ では設計の欠陥** |

## 4.2 誰が所有するのか

Java 版の `createProduct()` はこうです。

```java
protected Product createProduct(String owner) {
    return new IDCard(owner);
}
```

`new` して返して終わり。GC が回収します。C++ で同じ形を書くと、

```cpp
Product * create_product(const std::string & owner);   // 呼んだ人が delete する？
```

**誰が `delete` するかが型に書かれていません。** ドキュメントに書いても、読まれません。
返り値を `auto` で受けた人は、そこにポインタが入っていることにすら気づきません。

```cpp
auto card = factory.create("motor");   // Product * だった。delete し忘れてリーク
```

`std::unique_ptr` を返せば、この事故は起こせません。

```cpp
std::unique_ptr<Product> create(const std::string & owner);
```

これで **「受け取った人が唯一の所有者。スコープを抜ければ勝手に消える」**が型に書かれました。

```cpp
{
  auto card = factory.create("motor");   // std::unique_ptr<Product>
  card->use();
}                                        // ここで自動的に解放される
```

**Factory Method に限らず、「Java が `new` して返しているところは C++ では
`std::unique_ptr` を返す」**が原則です。第 1 章の `iterator()` と同じ判断です。

所有権の詳しい話は [C++編 6. スマートポインタ](../cpp/06_スマートポインタ.md) にあります。
ここでは繰り返しません。

### `return logger;` に `std::move` を書かない

`create()` の最後はこうなります。

```cpp
std::unique_ptr<Product> Factory::create(const std::string & owner)
{
  std::unique_ptr<Product> product = create_product(owner);
  if (product == nullptr) {
    return nullptr;
  }
  register_product(*product);
  return product;        // std::move は書かない
}
```

`product` はローカル変数なので、`return` のときに**自動的にムーブされます**。
`return std::move(product);` と書くと、コンパイラの最適化（NRVO）を邪魔するうえに
`-Wpessimizing-move` / `-Wredundant-move` で警告が出る場合があります。

### `std::make_unique` を使う理由

```cpp
return std::make_unique<IDCard>(owner);              // これ
return std::unique_ptr<Product>(new IDCard(owner));  // 動くが書かない
```

1. 型を 1 回しか書かない
2. `new` がコードから消える。**`delete` を探す必要が構造的になくなる**
3. 例外安全（引数の評価中に例外が飛んでもリークしない）

## 4.3 C++ 固有の危険

### 危険1: 共変戻り値型が `unique_ptr` では効かない

C++ には**共変戻り値型 (covariant return type)** という規則があります。
基底が `Base *` を返す仮想関数を、派生では `Derived *` を返すように狭められます。

```cpp
struct Creator
{
  virtual Product * create_raw() = 0;
};
struct FileCreator : Creator
{
  FileProduct * create_raw() override { return new FileProduct(); }   // 通る
};
```

**`unique_ptr` にした瞬間、これが使えなくなります。**

```cpp
struct Creator
{
  virtual std::unique_ptr<Product> create() = 0;
};
struct FileCreator : Creator
{
  std::unique_ptr<FileProduct> create() override { ... }   // 通らない
};
```

実際にコンパイルするとこうなります（`g++ -std=c++17 -Wall -Wextra -Wpedantic`、
Apple clang 17）。

```
covariant.cpp:22:32: error: virtual function 'create' has a different return type ('unique_ptr<FileProduct>') than the function it overrides (which has return type 'unique_ptr<Product>')
   22 |   std::unique_ptr<FileProduct> create() override { return std::make_unique<FileProduct>(); }
      |   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^
covariant.cpp:16:36: note: overridden virtual function is here
   16 |   virtual std::unique_ptr<Product> create() = 0;
      |           ~~~~~~~~~~~~~~~~~~~~~~~~ ^
1 error generated.
```

**理由**: 共変が許されるのは「ポインタまたは参照」だけ、と規格が決めています。
`std::unique_ptr<Derived>` と `std::unique_ptr<Base>` は**まったく無関係な 2 つのクラス**で、
継承関係がありません。派生関係があるのは中身の `Derived` と `Base` であって、
それを包む `unique_ptr` ではないのです。

**結論**: **ファクトリメソッドの戻り値型は、派生側でも `std::unique_ptr<Base>` のまま書きます。**
第 6 章（Prototype）の `clone()` で同じ壁にもう一度当たります。

### 危険2: 変換は片道である

戻り値型を `unique_ptr<Base>` に固定しても、中で `make_unique<Derived>()` するのは通ります。

```cpp
std::unique_ptr<Product> create_product() override
{
  return std::make_unique<IDCard>(owner);   // unique_ptr<IDCard> → unique_ptr<Product>
}
```

`unique_ptr<Derived>` から `unique_ptr<Base>` への**暗黙変換は認められています**。
これがあるので、共変が効かなくても実用上はほとんど困りません。

**逆はできません。**

```cpp
std::unique_ptr<Base> b = std::make_unique<Derived>();   // 通る
std::unique_ptr<Derived> d = std::move(b);               // 通らない
```

```
conv.cpp:7:28: error: no viable conversion from '__libcpp_remove_reference_t<unique_ptr<Base, default_delete<Base>> &>' (aka 'std::unique_ptr<Base>') to 'std::unique_ptr<Derived>'
    7 |   std::unique_ptr<Derived> d = std::move(b);               // 通らない
      |                            ^   ~~~~~~~~~~~~
```

「本当は `Derived` なのだから戻せるはず」と思うでしょうが、コンパイラには分かりません。
どうしても必要なら `dynamic_cast` を使いますが、**それが必要になった時点で設計を疑ってください。**
Factory を通した意味（具体型を隠す）が消えています。

### 危険3: 生成に失敗しうるとき、どう返すか

Java なら例外を投げるか `null` を返します。C++ には 3 択あります。

| 方法 | 書き方 | 使いどころ |
| --- | --- | --- |
| `nullptr` を返す | `std::unique_ptr<Product>` のまま | **マイコン。例外が使えない環境** |
| 例外を投げる | `throw std::runtime_error{...}` | PC / ROS 2 で、失敗が本当に異常なとき |
| `std::optional` | `std::optional<std::unique_ptr<Product>>` | **使わない**（4 行下） |

`std::optional<std::unique_ptr<T>>` は**状態が 3 つ**（無 / 有かつ null / 有かつ非 null）になり、
呼ぶ側のチェックが増えるだけです。`unique_ptr` は**それ自体が「無いかもしれない」を表せる**ので、
`nullptr` で足ります。

この講習では **`nullptr` を返す**で統一します。マイコンと ROS 2 で書き分けなくて済むからです。
**ただし `nullptr` チェックを呼び出し側に強制できません。**
だからテンプレートメソッド側（`create()`）で `nullptr` を早期 return して、
**「失敗したら登録もされない」を基底クラスが保証**します。

### 危険4: 仮想関数をコンストラクタから呼ぶ

第 3 章 3.4 と同じ話です。`Factory` のコンストラクタで `create_product()` を呼んでも、
**派生クラスの版は呼ばれません**（まだ派生部分が構築されていないため）。
純粋仮想なら実行時に落ちます。**ファクトリの初期化を `create()` の外に出さないでください。**

## 4.4 標準ライブラリ／言語機能に同じものが無いか

`std::make_unique<T>()` と `std::make_shared<T>()` が、**引数から `T` を作って所有権を返す**
という点で最小の Factory です。ただし「何を作るかを実行時に切り替える」機能はありません。

「実行時の切り替え」が本当に要るなら、C++ では**クラスの階層を作らずに関数で書けます**。

```cpp
using LoggerMaker = std::unique_ptr<Logger> (*)(const std::string &);   // 関数ポインタ

std::unique_ptr<Logger> make_uart(const std::string & tag);
std::unique_ptr<Logger> make_memory(const std::string & tag);

const std::map<std::string, LoggerMaker> kMakers = {
  {"uart", &make_uart},
  {"memory", &make_memory},
};
```

**Java では関数を単体で持ち回れないのでクラスにする必要がありましたが、
C++（と現代の Java）では関数ポインタ / ラムダ / `std::function` で足ります。**
`Factory` クラスを作るのは、**生成の前後に共通処理（登録・採番・ログ）がある**ときだけです。
それが無いなら、`create_product()` 1 個のためだけに継承階層を作っていることになります。

## 4.5 いつ Factory を入れるべきでないか

[0. 使う前に](00_使う前に.md) の 0.3「生成が 3 段になる」がこの章の裏面です。
輪読の直後にいちばん壊れるのがここなので、先に線を引きます。

**次のどれかに当てはまるなら、Factory を入れないでください。**

1. **コンパイル時に型が決まっている。** `Imu sensor{0x68};` で済みます。
   実行時に文字列でセンサを選ぶ場面が、部活のコードに本当にありますか
2. **ConcreteCreator が 1 つしかない。** 0.1 と同じで、ファイルが増えただけです
3. **生成の前後に共通処理が無い。** それは Factory ではなくただの関数です（4.4）
4. **生成物がヒープに要らない。** マイコンではこれが普通です（4.7）

**入れてよいのは、次が両方成り立つときだけです。**

- 作るものが**実行時に決まる**（設定ファイル、通信で来た指示、テスト時の差し替え）
- 生成の**前後に共通の手順がある**（登録、採番、初期化順序の保証）

課題 `dp04` の `LoggerFactory` は、**「作ったものをタグ付きで登録する」**という共通手順があるので
2 番目を満たします。そこが無ければ `std::make_unique<MemoryLogger>(...)` を直接書くのが正解です。

## 4.6 手元で試す

1 ファイルで完結します。**出力を予想してから**実行してください。

```cpp
// try.cpp
#include <iostream>
#include <memory>
#include <string>

// ---- Product ----
class Logger
{
public:
  virtual ~Logger() { std::cout << "  Logger 破棄\n"; }
  virtual void write(const std::string & message) = 0;
};

class ConsoleLogger : public Logger
{
public:
  explicit ConsoleLogger(std::string tag) : tag_(std::move(tag)) {}
  void write(const std::string & message) override
  {
    std::cout << "  [" << tag_ << "] " << message << "\n";
  }

private:
  std::string tag_;
};

// ---- Creator（Template Method そのもの）----
class LoggerFactory
{
public:
  virtual ~LoggerFactory() = default;

  // 手順は固定。作るものだけサブクラスに任せる
  std::unique_ptr<Logger> create(const std::string & tag)
  {
    std::unique_ptr<Logger> logger = create_logger(tag);
    if (logger == nullptr) {
      return nullptr;                 // 生成失敗。例外を投げない
    }
    ++created_count_;
    return logger;                    // 所有権が呼び出し側に移る
  }

  int created_count() const { return created_count_; }

private:
  virtual std::unique_ptr<Logger> create_logger(const std::string & tag) = 0;
  int created_count_ = 0;
};

class ConsoleLoggerFactory : public LoggerFactory
{
private:
  std::unique_ptr<Logger> create_logger(const std::string & tag) override
  {
    if (tag.empty()) {
      return nullptr;
    }
    // unique_ptr<ConsoleLogger> → unique_ptr<Logger> は暗黙に変換できる
    return std::make_unique<ConsoleLogger>(tag);
  }
};

int main()
{
  ConsoleLoggerFactory factory;

  {
    auto logger = factory.create("motor");
    logger->write("duty=0.5");
    std::cout << "スコープを抜けます\n";
  }

  auto ng = factory.create("");
  std::cout << "空タグ: " << (ng == nullptr ? "nullptr" : "生成された") << "\n";
  std::cout << "created_count = " << factory.created_count() << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 「Logger 破棄」はどこに出るか。<code>created_count</code> はいくつか</summary>

```
  [motor] duty=0.5
スコープを抜けます
  Logger 破棄
空タグ: nullptr
created_count = 1
```

**「Logger 破棄」は「スコープを抜けます」の後です。** `factory` は生成物を所有していません。
`logger` という `unique_ptr` が唯一の所有者で、内側のブロックを抜けたときに解放されます。

`created_count` が 1 なのは、**失敗したときに `create()` がカウント前に return している**からです。
`create_logger()` を直接呼べる設計にしていたら、この保証は書けません。
**手順を基底クラスに固定する（Template Method）意味がここに出ています。**

`create_logger()` の戻り値型を `std::unique_ptr<ConsoleLogger>` に変えてみてください。
4.3 のコンパイルエラーが出ます。
</details>

## 4.7 マイコンでの結論

**ヒープを使う Factory Method は、そのままでは使えません。**
`make_unique` は `new` です。動的確保は原則、起動時のみ。ループ中は禁止です。
断片化して、いずれ確保に失敗します。

選択肢は 3 つあります。**上から順に検討してください。**

### 案1（第一候補）: そもそも Factory を使わない

作るものがコンパイル時に決まっているなら、**静的に置いて参照を配ります**。

```cpp
static UartLogger g_uart_logger{1};      // .bss に 1 個。確保ゼロ
Logger & logger() { return g_uart_logger; }
```

部活のマイコンコードの 9 割はこれで済みます。`unique_ptr` も vtable の切り替えも要りません。
「ハードウェアが 1 つしかない」ものは第 5 章（Singleton）の話になります。

### 案2: 固定プール + placement new

実行時に種類を選ぶ必要が本当にあるなら、**メモリだけ先に静的確保しておいて、
そこに構築します**。ヒープは 1 バイトも使いません。

```cpp
// micro.cpp
#include <cstddef>
#include <cstdio>
#include <new>
#include <type_traits>

class Logger
{
public:
  virtual ~Logger() = default;
  virtual void write(const char * message) = 0;
};

class UartLogger : public Logger
{
public:
  explicit UartLogger(int channel) : channel_(channel) {}
  void write(const char * message) override
  {
    std::printf("  UART%d: %s\n", channel_, message);
  }

private:
  int channel_;
};

class NullLogger : public Logger
{
public:
  void write(const char *) override {}
};

// 固定プール。ヒープは一切使わない
template <typename Base, std::size_t Capacity, std::size_t SlotSize>
class LoggerPool
{
public:
  template <typename Derived, typename... Args>
  Base * create(Args &&... args)
  {
    static_assert(sizeof(Derived) <= SlotSize, "スロットが小さすぎます");
    static_assert(alignof(Derived) <= alignof(std::max_align_t), "アラインメント不足");
    if (used_ >= Capacity) {
      return nullptr;                       // 失敗は nullptr。例外は投げない
    }
    Base * p = new (storage_[used_]) Derived(static_cast<Args &&>(args)...);
    ++used_;
    return p;
  }

  std::size_t used() const { return used_; }

private:
  alignas(std::max_align_t) unsigned char storage_[Capacity][SlotSize] = {};
  std::size_t used_ = 0;
};

int main()
{
  LoggerPool<Logger, 2, 32> pool;

  Logger * a = pool.create<UartLogger>(1);
  Logger * b = pool.create<NullLogger>();
  Logger * c = pool.create<UartLogger>(2);   // 3 個目。プールが尽きている

  a->write("boot ok");
  b->write("これは捨てられる");
  std::printf("  3 個目: %s\n", c == nullptr ? "nullptr" : "生成された");
  std::printf("  used = %u\n", static_cast<unsigned>(pool.used()));
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions micro.cpp -o micro && ./micro
```

```
  UART1: boot ok
  3 個目: nullptr
  used = 2
```

`-fno-exceptions` を付けて通ることを確認してください。**例外は 1 つも使っていません。**
失敗は `nullptr` です。

このコードで意図的に**やっていない**ことが 3 つあります。マイコン向けの判断です。

1. **`unique_ptr` を返していない。** 生ポインタを返しています。
   起動時に作って一生使うものなので、**解放しない**（プールは `used_` を減らしません）
2. **デストラクタを呼んでいない。** 呼ぶなら `p->~Base();` を自分で書きます。
   途中で解放するなら空きスロットの管理が必要で、それは Factory ではなくアロケータの仕事です
3. **`std::string` を使っていない。** `const char *` です。確保が走らないため

**プールから返したポインタを `delete` すると壊れます**（`new` で取ったメモリではないため）。
どうしても `unique_ptr` で扱いたいなら、`std::unique_ptr<Logger, PoolDeleter>` のように
**カスタムデリータ**を付けて「デストラクタを呼ぶだけでメモリは返さない」を書きます。
ただし型が長くなるので、**まず案1で済まないかを疑ってください。**

### 案3: 呼び出し側がバッファを渡す

「生成した」という体裁すら要らないことも多いです。

```cpp
void init_logger(UartLogger & out, int channel);   // 置き場所は呼ぶ側が決める
```

第 1 章 1.7 と同じ考え方です。**確保する側と使う側を一致させる**のがマイコンの基本です。

## 4.8 ROS 2 での結論（補足）

ヒープが使えるので、`std::unique_ptr` を返す形をそのまま書けます。

ただし rclcpp には GoF 版の `Factory` クラス階層はほとんど出てきません。
代わりに **`Node` のメンバ関数が生成を担っています**。

```cpp
auto publisher = this->create_publisher<std_msgs::msg::String>("topic", 10);
```

`create_publisher` は `shared_ptr` を返します。`unique_ptr` でないのは、
**rclcpp 側も弱参照で観測する**からです（[C++編 6.5](../cpp/06_スマートポインタ.md)）。
「生成の前後に共通処理（QoS の解決、Executor への登録）がある」という
4.5 の条件をきれいに満たしている例です。

`pluginlib` の `ClassLoader` は本物の実行時 Factory ですが、
**共有ライブラリを実行時にロードする**という要求があって初めて元が取れる仕組みです。
部活の自作ライブラリでそこまで要ることは、まずありません。

## 4.9 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `error: virtual function 'create' has a different return type` | 派生で `unique_ptr<Derived>` を返している。共変は効かない（4.3）。`unique_ptr<Base>` に戻す |
| `error: no viable conversion from 'std::unique_ptr<Base>' to 'std::unique_ptr<Derived>'` | 逆方向の変換はできない。`dynamic_cast` が要る時点で設計を疑う |
| `error: use of deleted function ... unique_ptr(const unique_ptr &)` | `unique_ptr` をコピーしている。返すか `std::move` する |
| `warning: moving a local object in a return statement prevents copy elision` | `return std::move(x);` を書いている。`return x;` にする |
| 生成物がいつまでも解放されない | ファクトリが `shared_ptr` で自分でも持っている。**Creator は生成物を所有しない** |
| ファクトリを消したら生成物が壊れた | 生成物がファクトリ内のメンバを生ポインタで参照している。寿命の逆転 |
| 生成に失敗したのにカウントが増える | `create_logger()` を外から直接呼んでいる。手順は `create()` に閉じる |
| マイコンでしばらく動いてから確保に失敗する | ループ中で `make_unique` している。4.7 |

## 4.10 対応する課題

```bash
./drill run dp04
```

`exercises/dp04_factory_method/src/logger_factory.cpp` に、

1. `LoggerFactory::create()` — テンプレートメソッド（作る → 失敗なら中断 → 登録 → 返す）
2. `MemoryLoggerFactory::create_logger()` — ファクトリメソッド。`std::make_unique`、失敗は `nullptr`
3. `MemoryLoggerFactory::register_logger()` — タグを記録する
4. `MemoryLogger::write()` と `Logger` のコンストラクタ／デストラクタ

を実装します。テストは 6 本で、うち 3 本は**所有権が呼び出し側に移ること**を見ます。
生存数カウンタを使って、**スコープを抜けたら破棄されること**・
**`std::move` で所有者が移ること**・**ファクトリと同時に消えても二重解放しないこと**を確認します。

## 4.11 この章のまとめ

- **Factory Method は Template Method の一種。** 生成の手順を基底に固定し、何を作るかだけ任せる
- Java の `new Product()` を返すところは **`std::unique_ptr<Product>` を返す**。
  生ポインタでは**誰が `delete` するかが型に書かれない**
- `return product;` に **`std::move` は書かない**
- **共変戻り値型は `unique_ptr` では効かない。** 派生でも戻り値型は `unique_ptr<Base>` のまま
- `unique_ptr<Derived>` → `unique_ptr<Base>` は暗黙変換できる。**逆はできない**
- 生成失敗は **`nullptr`**。`std::optional<unique_ptr<T>>` は状態が 3 つになるので使わない
- 登録側は `const Product &` で受け取る。**所有権を渡さないことを型で示す**
- **コンパイル時に型が決まるなら Factory は要らない。** ConcreteCreator が 1 つなら入れない
- マイコンでは**まず「Factory を使わない」を検討**。要るなら固定プール + placement new。
  `unique_ptr` も例外も使わない

---

前: [3. Template Method](03_TemplateMethod.md) ／ 次: [5. Singleton](05_Singleton.md)
