# 20. Flyweight

> **結城本 第20章 対応。** `BigChar` と `BigCharFactory` と `BigString` を手元に開いてください。
>
> **この章のねらい**: Flyweight は**唯一、最適化のためのパターン**です。設計を良くするためではありません。
> だから「入れるかどうか」は測ってから決めます。そのうえで C++ 固有の問題が 1 つあります。
> 結城本の `BigCharFactory` は `HashMap` にインスタンスを溜め続けます。Java でもこれは
> 「解放されない」と本文に書かれていますが、C++ で `std::map<K, std::shared_ptr<T>>` にすると
> **プロセスが終わるまで本当に解放されません**。`std::weak_ptr` に変える理由をここで実測します。

## 20.0 まず「入れない判断」から

[0. 使う前に](00_使う前に.md) のチェックリストの 4 番目、
「標準ライブラリに同じものが無いか」に Flyweight が挙がっていました。この章の本題はそこです。

Flyweight は**測ってから入れるもの**です。順番を間違えないでください。

1. メモリが足りない／確保が遅い、という**症状**が出ている
2. 測って、原因が「同じ内容のオブジェクトが大量にある」ことだと**分かっている**
3. そのオブジェクトの中身に**不変な部分**がある

この 3 つが揃って初めて Flyweight です。揃っていないのに入れると、得られるものはゼロで、
**共有による寿命の複雑さだけが増えます**。

- 「誰かがまだ使っているか」を気にしないと解放できない
- 誰かが書き換えると全員に波及する
- 複数スレッドから引くならロックが要る
- プールをいつ掃除するかを決めないといけない

つまり、**Flyweight を入れるとは、寿命の管理を 1 段複雑にすること**です。
削減できるメモリが数百バイトなら、割に合いません。

判断の目安を書いておきます。

| 状況 | 結論 |
| --- | --- |
| 種類が少なく、個数が多い（型番 4 種 × センサ 200 個） | 検討する価値がある |
| 個数が少ない（センサ 6 個） | **入れない** |
| 共有したい中身がコンパイル時に決まっている | Flyweight ではなく **`constexpr`**（20.7） |
| 共有したいのが文字列だけ | **`std::string_view`** か文字列インターン（20.5） |
| 中身が可変 | Flyweight にできない。共有は不変なものだけ |

この章の課題も、正直に言えば「実務なら `constexpr` で終わり」の題材です。
それでも一度手で書くのは、**`constexpr` で終わりだと判断できるようになるため**です。

## 20.1 Java 版をそのまま C++ にすると

結城本の `BigCharFactory` の中身はこうでした。

```java
private Map<String, BigChar> pool = new HashMap<>();

public synchronized BigChar getBigChar(char charname) {
    BigChar bc = pool.get("" + charname);
    if (bc == null) {
        bc = new BigChar(charname);
        pool.put("" + charname, bc);
    }
    return bc;
}
```

C++ に素直に移すとこうなります。

```cpp
std::map<std::string, std::shared_ptr<BigChar>> pool_;

std::shared_ptr<BigChar> get_big_char(char name)
{
  const std::string key(1, name);
  auto found = pool_.find(key);
  if (found != pool_.end()) {
    return found->second;
  }
  auto created = std::make_shared<BigChar>(name);
  pool_[key] = created;
  return created;
}
```

Java 版から変えた点が 3 つあります。

### 変更点1: 戻りは `std::shared_ptr<const T>` にする

Java 版は `BigChar` の参照を返します。C++ では**誰が解放するかを決めなければいけません**。
Flyweight は「複数の利用者が同じ実体を指す」パターンなので、所有権も共有です。
`std::unique_ptr` では表現できません。ここは `std::shared_ptr` の出番です。

そして `const` を付けます。

```cpp
std::shared_ptr<const CalibrationTable> get(const std::string & model_id);
```

**共有するものは const にする。** これが Flyweight の前提そのものです。
`shared_ptr<T>`（const なし）を返すと、利用者の 1 人が中身を書き換えたときに
**共有している全員に波及します**。Java 版の `BigChar` がたまたま不変なだけなのを、
C++ では型で強制できます。

`Iterator` の章で「`&` を付け忘れるとコピーになる」と書きましたが、
Flyweight では逆に**`const` を付け忘れると共有が事故になります**。

### 変更点2: コピーコンストラクタを消す

```cpp
class CalibrationTable
{
public:
  CalibrationTable(const CalibrationTable &) = delete;
  CalibrationTable & operator=(const CalibrationTable &) = delete;
  // ...
};
```

Java には値のコピーがありません。C++ では、何もしなければコピーコンストラクタが生成されます。

```cpp
CalibrationTable copy = *handle;   // 何も書かないとこれが通る
```

Flyweight をコピーした瞬間に、共有した意味が消えます。**`= delete` で塞いでください。**
「共有するものはコピーできない」と型に書くのが C++ のやり方です。

### 変更点3: `synchronized` をどうするか

Java 版には `synchronized` が付いています。C++ には対応する言語機能がありません。
これは 20.6 で扱います。**書き忘れが即データ競合**なので、後回しにしないでください。

## 20.2 誰がプールを解放するのか — Java との一番大きい差

ここがこの章の本題です。

結城本にもこう書いてあります。「プールに入れたインスタンスは GC の対象にならない」。
Java では**リークするだけ**です。C++ で `std::map<K, std::shared_ptr<T>>` にすると、
**同じことが、より確実に起きます**。プールが `shared_ptr` を握っているので、
参照カウントが 0 になりません。プールが死ぬまで解放されません。

`use_count` で見えます。手元で試してください（20.4 のコード）。

```
[strong]
  + Table(gyro)
  same? 1  use_count=3
  利用者は全員いなくなった
  - ~Table(gyro)
  プールが死んだ
```

`use_count=3` です。利用者は 2 人しかいないのに 3。**残りの 1 はプールです。**
そして `~Table` が呼ばれているのは「プールが死んだ」の**直前**、
つまりプールのデストラクタの中です。利用者が全員いなくなった時点では解放されていません。

これを直すには、**プールが所有権を持たないようにします**。`std::weak_ptr` です。

```cpp
std::map<std::string, std::weak_ptr<const CalibrationTable>> pool_;

Handle get(const std::string & model_id)
{
  auto found = pool_.find(model_id);
  if (found != pool_.end()) {
    if (auto alive = found->second.lock()) {   // まだ生きていれば shared_ptr が返る
      return alive;
    }
    // lock() が nullptr = 誰も使っていない残骸。作り直す
  }
  auto created = std::make_shared<CalibrationTable>(/* ... */);
  pool_[model_id] = created;                   // weak_ptr として登録される
  return created;
}
```

同じプログラムの後半の出力です。

```
[weak]
  + Table(gyro)
  same? 1  use_count=2
  - ~Table(gyro)
  利用者は全員いなくなった
  pool.size()=1
```

3 つ読み取ってください。

1. `use_count=2` — プールは数に入っていません。所有していないからです
2. `~Table` が「利用者は全員いなくなった」より**前**に出ています。手放した瞬間に解放されました
3. **`pool.size()=1` のまま**です

3 番目が `weak_ptr` 版の落とし穴です。`Table` は消えましたが、
**`map` のエントリ（キーと空になった `weak_ptr`）は残っています**。
`weak_ptr` は自分が expired になったことを `map` に伝えられません。

つまり `weak_ptr` にしても、**「引いた型番の種類の数だけ」エントリが溜まります**。
中身（数百バイト）は消えますが、キー（`std::string`）は残ります。
掃除する関数が要ります。

```cpp
std::size_t sweep_expired()
{
  std::size_t removed = 0;
  for (auto it = pool_.begin(); it != pool_.end();) {
    if (it->second.expired()) {
      it = pool_.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }
  return removed;
}
```

`erase` が次の要素を指すイテレータを返すので、それを受けます。
`++it` してから `erase` すると無効なイテレータを触ります。

**いつ呼ぶか**は設計判断です。選択肢は 3 つ。

| 方法 | 向き |
| --- | --- |
| 呼ばない（残骸を許容する） | 種類が有限で少ないとき。この課題の題材ならこれで十分 |
| `get()` の中で、当たった残骸だけ作り直す | ほぼコストゼロ。ただし全体は掃除されない |
| 定期的に `sweep_expired()` を呼ぶ | 種類が実行時に無限に増えうるとき（ユーザ入力がキーなど） |

**種類が有限なら `shared_ptr` プールのままでも実害はありません。**
「起動時に 4 個作って、終了まで持つ」だけです。`weak_ptr` が要るのは
**キーが実行時に増え続けるとき**です。ここを取り違えて、要らない複雑さを入れないでください。

## 20.3 本質的状態と付帯的状態 — 分離を間違えると共有できない

結城本の言葉で言うと、`BigChar`（文字の形）が**本質的（intrinsic）**、
`BigString` の中の「何文字目か」が**付帯的（extrinsic）**です。

課題の題材で言い直します。

| | 中身 | 置き場所 |
| --- | --- | --- |
| 本質的（intrinsic） | 型番ごとの `gain` / `offset` | `CalibrationTable`（共有・`const`） |
| 付帯的（extrinsic） | センサ個体のゼロ点補正 `zero_offset` | `Sensor`（各自が持つ） |

分離の判断はひとつです。

> **その値を書き換えたとき、他の利用者にも波及してよいか。**
> よいなら本質的、だめなら付帯的。

`zero_offset` を `CalibrationTable` に入れた瞬間、
同じ型番のセンサ 2 個が**別々のテーブルを持たざるを得なくなります**。共有が壊れます。

C++ では、この判断ミスを `const` が捕まえてくれます。

```cpp
std::shared_ptr<const CalibrationTable> table_;   // 共有しているもの
double zero_offset_;                              // 自分のもの

double convert(int raw) const
{
  return raw * table_->gain() + table_->offset() + zero_offset_;
  //     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^   共有  ^^^^^^^^^^^^ 個体
}
```

`table_` 経由では非 const メンバ関数を呼べません。**書き換えようとした瞬間にコンパイルエラー**です。
Java 版はこれを規約とコメントで守っています。C++ は型で守れます。

## 20.4 手元で試す

`use_count` とデストラクタで、20.2 の話を自分の目で確認してください。

```cpp
#include <iostream>
#include <map>
#include <memory>
#include <string>

struct Table
{
  explicit Table(std::string id) : id_(std::move(id))
  {
    std::cout << "  + Table(" << id_ << ")\n";
  }
  ~Table() { std::cout << "  - ~Table(" << id_ << ")\n"; }
  std::string id_;
};

// 結城本の BigCharFactory をそのまま移した版
class StrongPool
{
public:
  std::shared_ptr<const Table> get(const std::string & id)
  {
    auto found = pool_.find(id);
    if (found != pool_.end()) {
      return found->second;
    }
    auto created = std::make_shared<Table>(id);
    pool_[id] = created;
    return created;
  }

private:
  std::map<std::string, std::shared_ptr<const Table>> pool_;
};

// weak_ptr で持つ版
class WeakPool
{
public:
  std::shared_ptr<const Table> get(const std::string & id)
  {
    auto found = pool_.find(id);
    if (found != pool_.end()) {
      if (auto alive = found->second.lock()) {
        return alive;
      }
    }
    auto created = std::make_shared<Table>(id);
    pool_[id] = created;
    return created;
  }

  std::size_t size() const { return pool_.size(); }

private:
  std::map<std::string, std::weak_ptr<const Table>> pool_;
};

int main()
{
  std::cout << "[strong]\n";
  {
    StrongPool pool;
    {
      auto a = pool.get("gyro");
      auto b = pool.get("gyro");
      std::cout << "  same? " << (a.get() == b.get()) << "  use_count=" << a.use_count() << "\n";
    }
    std::cout << "  利用者は全員いなくなった\n";
  }
  std::cout << "  プールが死んだ\n";

  std::cout << "[weak]\n";
  {
    WeakPool pool;
    {
      auto a = pool.get("gyro");
      auto b = pool.get("gyro");
      std::cout << "  same? " << (a.get() == b.get()) << "  use_count=" << a.use_count() << "\n";
    }
    std::cout << "  利用者は全員いなくなった\n";
    std::cout << "  pool.size()=" << pool.size() << "\n";
  }
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 2 つの <code>use_count</code> はいくつか。<code>~Table</code> はどのタイミングで出るか</summary>

実際の出力です。

```
[strong]
  + Table(gyro)
  same? 1  use_count=3
  利用者は全員いなくなった
  - ~Table(gyro)
  プールが死んだ
[weak]
  + Table(gyro)
  same? 1  use_count=2
  - ~Table(gyro)
  利用者は全員いなくなった
  pool.size()=1
```

- `strong` 版は `use_count=3`。利用者 2 人 + **プール 1**。
  `~Table` が出るのはプールが死ぬ直前で、利用者が手放した時点ではありません
- `weak` 版は `use_count=2`。`~Table` は利用者が手放した**瞬間**に出ています
- ただし `weak` 版でも `pool.size()` は 1 のまま。
  **`weak_ptr` は expired になっても `map` から自分を消せません**

`same? 1` はどちらも同じです。共有できているかどうかは `strong` / `weak` の差ではありません。
差は**いつ解放されるか**だけです。
</details>

## 20.5 標準ライブラリ／言語機能に同じものが無いか

**あります。というより、C++ には Flyweight クラスを書くより軽い道具が 3 つあります。**
自作する前に、この 3 つで足りないかを必ず確認してください。

### (1) `std::shared_ptr<const T>` — これ自体が共有の道具

`std::shared_ptr` は、Flyweight パターンの半分（共有と寿命）を既に実装しています。
自分で書くのは**プールの部分だけ**です。
「Flyweight クラス」という基底クラスも、`virtual` も、まったく要りません。

結城本の `BigChar` には仮想関数がありません。**そこは真似してください。**
Flyweight に継承は不要です。

### (2) `std::string_view` — 文字列を共有する（コピーしない）

共有したいものが文字列なら、Flyweight は要りません。

```cpp
// 悪い例: 診断メッセージを std::string で持つと、同じ文言のコピーが個数分できる
struct Diagnostic
{
  std::string message;   // "over current" が 200 個ぶんコピーされる
};

// std::string_view なら、指すだけ。確保ゼロ
struct Diagnostic
{
  std::string_view message;
};

inline constexpr std::string_view kOverCurrent = "over current";
```

`std::string_view` は**ポインタと長さのペア**です。コピーしても中身は増えません。
これが一番軽い共有です。

**ただし危険が 1 つあります。元の文字列が死ぬと、`string_view` は宙に浮きます。**

```cpp
std::string_view get_name()
{
  std::string name = "MPU6050";
  return name;              // name はここで死ぬ
}                           // 返ってきた string_view はダングリング

std::string_view bad = std::string("MPU") + "6050";   // 一時オブジェクトが即死
```

これは `Iterator` の章（1.3）で見た「イテレータが本棚より長生きする」問題と**まったく同じ形**です。
`string_view` はイテレータの一種だと思ってください。

安全なのは、指す先が次のどれかのときです。

| 指す先 | 安全か |
| --- | --- |
| 文字列リテラル（`"abc"`）／ `constexpr` テーブル | **安全**。プログラムと同じ寿命 |
| 呼び出し側が持っている `std::string`（引数として渡された） | 呼び出しの間だけ安全 |
| ローカルの `std::string` | **危険**。関数を抜けたら死ぬ |
| 一時オブジェクト（`a + b` の結果） | **危険**。式が終わったら死ぬ |

`std::string_view` はメンバに保存するときが一番危ないです。
**保存するなら、指す先の寿命を型かコメントで約束してください。**

### (3) 文字列インターン

同じ文字列を大量に扱うなら、`std::set<std::string>` に 1 個だけ持ち、
`std::string_view` を配る、というやり方があります。これが「文字列インターン」で、
**Flyweight の一番よくある実例**です。

```cpp
class StringPool
{
public:
  // 返す string_view は、この StringPool より長生きさせないこと。
  std::string_view intern(std::string value)
  {
    // std::set は要素のアドレスが安定している（再確保で動かない）。
    // std::vector<std::string> だと再確保で全部のアドレスが変わり、
    // 配った string_view が全滅します。ここが選定の理由です。
    return *pool_.insert(std::move(value)).first;
  }

private:
  std::set<std::string> pool_;
};
```

コメントに書いたとおり、**コンテナの選定が寿命に直結します**。
`std::vector` を使ってはいけません。

### 標準に「無い」もの

プールそのものは標準にありません。`std::flyweight` はありません
（Boost には `boost::flyweight` があります）。
**共有と寿命は標準が持っていて、プールだけ自分で書く**、というのが C++ での姿です。

## 20.6 スレッド安全性 — `shared_ptr` は半分しか守ってくれない

Java 版の `synchronized` に対応するものを自分で書きます。ここを飛ばすと壊れます。

**`std::shared_ptr` の参照カウントはアトミックです。** 複数スレッドが同じ
`shared_ptr` をコピー／破棄するのは安全です。

**しかしプール（`std::map`）は何も保護されていません。**
2 つのスレッドが同時に `get()` を呼ぶと、

- 片方が `insert` している最中にもう片方が `find` する → 未定義動作
- 両方が「無いから作ろう」と判断する → **同じ型番のインスタンスが 2 個**できる（共有が壊れる）

対策は、`get()` と `sweep_expired()` を丸ごとロックで囲むことです。

```cpp
class CalibrationRegistry
{
public:
  Handle get(const std::string & model_id)
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    // ... find / lock / make_shared / insert
  }

  std::size_t sweep_expired()
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    // ...
  }

private:
  mutable std::mutex mutex_;
  std::map<std::string, std::weak_ptr<const CalibrationTable>> pool_;
};
```

`find` と `insert` の**間で手を離してはいけません**。「探す→無ければ作る」は
1 つの不可分な操作です。ここを 2 回に分けてロックすると、上の 2 番目の事故が起きます。

なお課題のレジストリは**シングルスレッド前提**にしてあります。ロックは入れていません。
ヘッダのコメントにそう書いてあります。**書いてあることが仕事です。**

そして、ここが 20.0 に戻る理由でもあります。
Flyweight を入れると、**ロックという新しい遅さ**が付いてきます。
メモリを節約するために時間を払っているので、本当に得をしているかは測らないと分かりません。

## 20.7 マイコンでの結論

**マイコンでは、この章のプールは書きません。** 理由は 3 つ、すべて致命的です。

1. `std::map` も `std::string` も `std::make_shared` も**ヒープを確保します**
2. `std::shared_ptr` の制御ブロックが 1 個ごとに確保されます
3. そもそもオブジェクトを大量に作りません。作らなければ共有する相手もいません

代わりに使うのが `constexpr` です。**共有したい不変なものは ROM に置きます。**

```cpp
#include <cstdio>
#include <string_view>

struct Spec
{
  std::string_view id;
  double gain;
  double offset;
};

inline constexpr Spec kRom[] = {
  {"MPU6050-GYRO", 0.0076294, 0.0},
  {"AS5600-ENC", 0.0878906, 0.0},
  {"ACS712-30A", 0.0666000, -2.5},
  {"NTC-10K", 0.0244140, -40.0},
};

constexpr const Spec * find_spec(std::string_view id)
{
  for (const Spec & spec : kRom) {
    if (spec.id == id) {
      return &spec;
    }
  }
  return nullptr;
}

int main()
{
  constexpr const Spec * ntc = find_spec("NTC-10K");
  static_assert(ntc != nullptr, "");
  static_assert(ntc->offset == -40.0, "");
  static_assert(find_spec("NO-SUCH") == nullptr, "");

  std::printf("sizeof(kRom) = %zu\n", sizeof(kRom));
  std::printf("offset = %f\n", ntc->offset);
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic rom.cpp -o rom && ./rom
```

出力です。

```
sizeof(kRom) = 128
offset = -40.000000
```

**これが最強の Flyweight です。**

- RAM を **1 バイトも使いません**。`kRom` は `.rodata`（ROM）に置かれます
- プールも `map` も要りません。`find_spec` が**プールの引き当てそのもの**です
- 型番がコンパイル時に分かっていれば、`static_assert` が通る = **実行時に何も起きていません**。
  `find_spec("NTC-10K")` の呼び出しごと消えます
- 全員が同じ `&kRom[3]` を指します。共有は自動的に達成されます
- 寿命の問題がありません。`weak_ptr` も `sweep_expired()` も要りません

実際、上のコードを `-O2` でコンパイルして中身を見ると、
`find_spec` の呼び出しは残っていません。`static_assert` の 3 行はコンパイル時に消え、
実行時に残るのは `printf` だけです。

`std::string_view` を使っているのも意図的です。`std::string` にすると確保が走ります。
リテラルを指す `string_view` は、指す先がプログラムと同じ寿命なので**安全な使い方**です（20.5 の表の 1 行目）。

### 型番が実行時にしか分からないとき

EEPROM から型番を読む、という場合でも `constexpr` テーブルは使えます。
消えるのは「`find_spec` の呼び出しが消える」ことだけで、
**テーブルが ROM にあることと、確保がゼロであることは変わりません**。

```cpp
const Spec * spec = find_spec(read_model_id_from_eeprom());
if (spec == nullptr) {
  // 未知の型番。例外は使えないので戻り値で返す
  return Error::UnknownModel;
}
```

`-fno-exceptions` が普通なので、**見つからないときは `nullptr` を返します**。
`throw` も `std::optional` の例外送出版（`.value()`）も使いません。
課題の `CalibrationRegistry::get()` を「未知なら `nullptr`」にしてあるのは同じ理由です。

### それでも実行時に作る必要があるとき

較正値をフィールドで書き換えたい、というような場合です。そのときは
**起動時に固定長配列へ全部作り、インデックスで配ります**。

```cpp
// 起動時に 1 回だけ。ループ中の確保はゼロ
class CalibrationStore
{
public:
  /// 見つからなければ nullptr。所有権は Store が持ち続ける（利用者は指すだけ）
  const Spec * find(std::string_view id) const
  {
    for (std::size_t i = 0; i < count_; ++i) {
      if (entries_[i].id == id) {
        return &entries_[i];
      }
    }
    return nullptr;
  }

private:
  static constexpr std::size_t kCapacity = 8;
  Spec entries_[kCapacity] = {};
  std::size_t count_ = 0;
};
```

`shared_ptr` を使っていない点に注目してください。
**所有者が 1 人（`CalibrationStore`）に決まっているなら、共有所有は要りません。**
利用者は生ポインタで指すだけです。寿命の約束は「`CalibrationStore` はグローバルに 1 個、
プログラムと同じ寿命」とコメントに書きます。

`shared_ptr` が要るのは「**誰が最後に手放すか分からない**」ときだけです。
マイコンでそんな状況はまず作りません。

## 20.8 ROS 2 での結論（補足）

ROS 2 では Flyweight を自分で書く場面はほとんどありません。

- メッセージ型の定義や QoS プロファイルは、そもそも数が少ない
- `rclcpp` の内部では `shared_ptr` が大量に使われていますが、それは
  Flyweight（同じ内容の共有）ではなく、**寿命管理**のための `shared_ptr` です。混同しないでください
- 大きいデータを配るときに考えるのは Flyweight ではなく
  **ゼロコピー**（`loaned message` / intra-process 通信）です。目的は「同じ内容を共有する」ことではなく
  「コピーとシリアライズを避ける」ことなので、別の話です

近いのは「パラメータ名やフレーム ID を `std::string` で持ち回っている」場合です。
コールバックのたびに `std::string` をコピーしているなら、
`std::string_view` に変えるか、`static const std::string` を 1 個持ちます。
これは 20.5 の (2)(3) の話で、Flyweight クラスは要りません。

## 20.9 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| 利用者が全員いなくなっても解放されない | プールが `shared_ptr` で持っている。`weak_ptr` にする |
| `use_count` が想定より 1 大きい | 同上。プールが 1 数えている |
| `weak_ptr` にしたのに `pool.size()` が減らない | expired なエントリは自動で消えない。`sweep_expired()` が要る |
| 同じ型番なのに別のインスタンスが返る | `lock()` の戻りを確認していない、またはプールに登録し忘れ |
| 誰かが値を書き換えたら全員おかしくなった | `shared_ptr<T>` を返している。`shared_ptr<const T>` にする |
| Flyweight をコピーしたら共有が壊れた | コピーコンストラクタを `= delete` していない |
| センサごとに別のテーブルができる | 付帯的状態（`zero_offset`）を Flyweight 側に入れている |
| `sweep_expired()` でクラッシュする | `erase` したイテレータを `++` している。`it = pool_.erase(it)` にする |
| 保存した `string_view` が化ける | 指していた `std::string` が死んでいる。20.5 の表を見る |
| マルチスレッドで同じ型番が 2 個できた | `find` と `insert` の間でロックを手放している |
| マイコンでヒープが足りなくなった | `std::map` + `make_shared` を使っている。`constexpr` テーブルにする |

## 20.10 対応する課題

```bash
./drill run dp20
```

センサの較正テーブルを共有します。型番は 4 種類しかないのに、機体にはセンサが何十個も載る、
という状況が題材です。`exercises/dp20_flyweight/src/calibration.cpp` に 3 つ実装します。

1. **`CalibrationRegistry::get()`** — `std::weak_ptr` のプールから共有インスタンスを引く。
   生きていれば共有、残骸なら作り直し、ROM に無ければ `nullptr`
2. **`CalibrationRegistry::sweep_expired()`** — expired な残骸を掃除する
3. **`Sensor::convert()`** — 共有された `gain` / `offset` と、個体ごとの `zero_offset` を合成する

`constexpr` 版（`kCalibrationRom` / `find_spec`）は**実装済み**です。読んでください。
実務ではこれで終わることが多い、というのがこの章の結論だからです。

テストは 7 つ。

- 同じ型番を 2 回引くと**同一のアドレス**が返ること
- 生成回数が、引いた回数（5 回）ではなく**種類の数**（3 つ）と一致すること
- 全員が手放したら破棄され、`sweep_expired()` でプールからも消えること（`use_count` とデストラクタで実測）
- 付帯的状態が共有されないこと
- `constexpr` テーブルが `static_assert` を通り、**実行時にヒープ確保が 0 回**であること
  （テストがグローバルの `operator new` を差し替えて数えています）

## 20.11 この章のまとめ

- Flyweight は**最適化**。設計を良くするパターンではない。**測ってから入れる**
- 入れると、削減できるメモリと引き換えに**寿命の複雑さとロック**が増える
- Java の `HashMap` プールを `std::map<K, std::shared_ptr<T>>` に移すと、
  **プロセスが終わるまで解放されない**。`use_count` が 1 大きくなるので気付ける
- `std::weak_ptr` プールにすると解放される。ただし**空になったエントリは自分で掃除する**
- 共有するものは **`const`**。`shared_ptr<const T>` を返し、コピーは `= delete`
- 本質的状態と付帯的状態の分け方は「書き換えたとき他人に波及してよいか」
- C++ には軽い道具がある。**`std::shared_ptr<const T>` / `std::string_view` / 文字列インターン**。
  `string_view` は指す先の寿命が命
- `shared_ptr` の参照カウントはアトミックだが、**プールは保護されない**。`find` と `insert` の間で手を離さない
- **マイコンでは `static constexpr` の ROM テーブル**。RAM 0 バイト、確保 0 回、寿命の問題なし。
  これが最強の Flyweight

---

前: [19. State](19_State.md) ／ 次: 21. Proxy（準備中）
