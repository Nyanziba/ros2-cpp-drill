# 11. Composite

> **結城本 第11章 対応。** `Entry` / `File` / `Directory` を手元に開いてください。
>
> **この章のねらい**: 構造そのものは Java 版とほぼ同じです。**変わるのは所有権だけ**です。
> Java の `ArrayList<Entry> directory` を C++ に持ってくると、
> `std::vector<Entry>` にした瞬間に**スライシングして派生部分が消え**、
> `std::vector<Entry *>` にすると**誰が解放するのか誰も知らない**状態になります。
> 正解は `std::vector<std::unique_ptr<Entry>>` ですが、
> これを選ぶと今度は**そのクラスがコピーできなくなります**。
> さらに、結城本のように親へのリンクを持たせると `shared_ptr` では**循環して解放されません**。
> この章はほぼ全部、寿命の話です。

## 11.1 Java 版をそのまま C++ にすると

結城本の `Entry` はこうです（抜粋）。

```java
public abstract class Entry {
    public abstract String getName();
    public abstract int getSize();
    public Entry add(Entry entry) throws FileTreatmentException {
        throw new FileTreatmentException();
    }
    public void printList() { printList(""); }
    protected abstract void printList(String prefix);
}
```

C++ に移すとこうなります。

```cpp
class Entry
{
public:
  virtual ~Entry() = default;
  const std::string & name() const { return name_; }
  virtual std::size_t size() const = 0;
  virtual void print_list(const std::string & prefix, std::vector<std::string> & out) const = 0;

protected:
  explicit Entry(std::string name) : name_(std::move(name)) {}

private:
  std::string name_;
};
```

Java 版から変えた点が 3 つあります。

### 変更点1: `virtual ~Entry() = default;` を足した

第 1 章と同じ話ですが、**この章では他の章より危険度が高い**です。
Composite では子を `std::unique_ptr<Entry>` で持ちます。つまり
**基底クラスのポインタ経由で `delete` が走ることが構造上確定しています**。
仮想デストラクタが無ければ、木を捨てるたびに派生クラスのデストラクタが呼ばれず、
`std::string` も `std::vector` も解放されません。

### 変更点2: `add()` を基底から**消した**

結城本の `Entry` は `add()` を持っていて、`File` に対して呼ぶと
`FileTreatmentException` が飛びます。「葉にも `add()` があるが、呼ぶと実行時に失敗する」設計です。

C++ では `add()` を `Directory` にだけ置きます。

```cpp
File file{"imu_whoami", 100};
file.add(...);       // error: no member named 'add' in 'File'
```

**実行時例外がコンパイルエラーになりました。** マイコンでは `-fno-exceptions` が普通なので、
そもそも `throw` という選択肢がありません。値を返して呼び側に確認させるくらいしかできず、
それは誰も確認しません。

GoF 本にもこの議論（「透過性 vs 安全性」）があります。Java 版は透過性を取り、
基底に `add()` を置いて葉で失敗させます。**C++ では安全性を取ります。**
理由は、C++ に `dynamic_cast` を使わずに「これは葉か」を安全に聞く手段が乏しく、
かつマイコンでは `-fno-rtti` で `dynamic_cast` 自体が使えないことが多いからです。

透過性が本当に要る場面（葉と節を区別せずに `add` を呼びたい）は、実際にはまず来ません。
`add` を呼ぶコードは「今どこにグループを作っているか」を必ず知っています。

### 変更点3: `print_list` は文字列を返さず `out` に積む

Java 版は `System.out.println` を直接呼びます。
ライブラリのクラスが標準出力に書くのは、部活のコードでも避けてください。
テストできなくなります。`std::vector<std::string> & out` を渡して積むか、
コールバックを受け取ります。**マイコンには標準出力がそもそも無い**という事情もあります。

## 11.2 誰が子を所有するのか

Java 版はこうです。

```java
private List<Entry> directory = new ArrayList<>();
public Entry add(Entry entry) { directory.add(entry); return this; }
```

C++ での選択肢は 4 つあり、**正解は 1 つです**。

| 書き方 | 何が起きるか | 判定 |
| --- | --- | --- |
| `std::vector<Entry>` | **スライシング**。`Entry` の部分だけコピーされ、派生部分が消える。抽象クラスならそもそもコンパイルできない | ✗ |
| `std::vector<Entry *>` | 誰が `delete` するか型に書かれない。木を捨てるコードを自分で書く羽目になる | ✗ |
| `std::vector<std::shared_ptr<Entry>>` | 動く。ただし**共有したい理由が無い**。参照カウントの分だけ重く、親へのリンクを足すと循環する | △ |
| `std::vector<std::unique_ptr<Entry>>` | 親が単独で所有する。親が死ねば木が丸ごと死ぬ | **○** |

```cpp
class Directory : public Entry
{
public:
  void add(std::unique_ptr<Entry> child);

private:
  std::vector<std::unique_ptr<Entry>> children_;
};
```

**「木の所有者は根である」が型に書かれました。** `delete` は 1 行も書きません。
根の `unique_ptr` を捨てれば、`vector` のデストラクタ →
各 `unique_ptr` のデストラクタ → 子の `Directory` のデストラクタ → …と連鎖します。

`shared_ptr` を選ぶ理由があるとすれば「同じ部分木を 2 つの親にぶら下げたい」ときですが、
それは木ではなく DAG です。**診断項目の木にそんな要件はありません。**
「なんとなく `shared_ptr`」は 0 章で言った過剰適用そのものです。

## 11.3 スライシング — 値で持つと派生が黙って消える

C++ 固有の危険の 1 つ目です。`Entry` が純粋仮想関数を持っていれば
`std::vector<Entry>` はコンパイルエラーになるので気づけます。
**こわいのは、基底に実装があってコンパイルが通ってしまう場合**です。

```cpp
class Entry
{
public:
  virtual ~Entry() = default;
  virtual int size() const { return 0; }   // 純粋仮想ではない
};

class Check : public Entry
{
public:
  explicit Check(int size) : size_(size) {}
  int size() const override { return size_; }

private:
  int size_;
};

std::vector<Entry> by_value;
by_value.push_back(Check{100});            // コンパイルは通る
by_value[0].size();                        // ?
```

実測（11.9 で全体を動かします）。

```
vector<Entry>            : 0
vector<unique_ptr<Entry>>: 100
```

`push_back` が `Entry` のコピーコンストラクタを呼び、
**`Check` の部分（`size_` も vtable も）を切り落として**います。
エラーも警告も出ません。`Composite` を値の `vector` で持ってはいけない理由がこれです。

Java にこの現象はありません。`ArrayList<Entry>` は常に参照を持つからです。

## 11.4 `unique_ptr` の `vector` を持つクラスはコピーできない

`std::vector<std::unique_ptr<Entry>>` をメンバに持った瞬間、
`Directory` の**コピーコンストラクタが暗黙に delete されます**。

```cpp
Directory a{"motors"};
Directory b = a;      // error: call to implicitly-deleted copy constructor
```

これは事故ではなく**正しい**挙動です。`Directory` をコピーできたら、
同じ子を 2 つの親が所有することになり、二重解放します。
コピーしたいなら、木を再帰的に複製する `clone()` を自分で書くしかありません
（第 6 章 Prototype の話です）。

`add()` の引数も、この制約から形が決まります。

```cpp
void add(std::unique_ptr<Entry> child);          // 値で受ける
```

`unique_ptr` はコピーできないので、**呼び側は必ずムーブして渡すことになります**。
「所有権をここで手放しました」が呼び出し側のコードに `std::move` として現れます。

```cpp
auto motors = std::make_unique<Directory>("motors");
motors->add(std::make_unique<Check>("motor_l", true));   // 一時オブジェクトは自動でムーブ
root->add(std::move(motors));                            // 名前付き変数は std::move が要る
// ここから先の motors は nullptr。使うと落ちる
```

受け取った側でも `std::move` が要ります。

```cpp
void Directory::add(std::unique_ptr<Entry> child)
{
  children_.push_back(std::move(child));   // move を忘れるとコンパイルエラー
}
```

`child` は**名前が付いた変数**なので、そのままでは左辺値です。`std::move` を忘れると
「`unique_ptr` のコピーコンストラクタは削除されています」と言われます。
**このエラーは正常です。** 直し方は `std::move` を足すことです。

なお、デストラクタを自分で書くと（この課題ではログのために書きます）
**ムーブコンストラクタが暗黙に生成されなくなります**。5 つ全部を明示してください。

```cpp
~Directory() override;
Directory(const Directory &) = delete;
Directory & operator=(const Directory &) = delete;
Directory(Directory &&) noexcept = default;
Directory & operator=(Directory &&) noexcept = default;
```

## 11.5 親へのポインタを持つと循環する

結城本の演習には「親をたどれるようにする」という話が出てきます。
C++ でこれを `shared_ptr` でやると、**親子が互いを所有して永久に解放されません**。

```cpp
struct Bad
{
  std::vector<std::shared_ptr<Bad>> children;
  std::shared_ptr<Bad> parent;      // 親も shared_ptr = 循環する
};

struct Good
{
  std::vector<std::shared_ptr<Good>> children;
  std::weak_ptr<Good> parent;       // 親は weak_ptr。所有しない
};
```

デストラクタにログを入れて実測した結果です（コードは 11.9 の後半）。

```
Bad  root.use_count = 2
--- Bad のスコープを抜けた ---
Good root.use_count = 1
~Good root
~Good leaf
--- Good のスコープを抜けた ---
```

**`Bad` は `~Bad` が 1 行も出ていません。** 2 個ともリークしています。
`root` を捨てても、`leaf` が `parent` として `root` を持っているのでカウントが 0 になりません。
`leaf` は `root` の `children` に入っているので、こちらも 0 になりません。
Java なら GC が「外から到達できない循環」を回収するので、この問題は起きません。
**参照カウントは循環を回収できない**、というのが `shared_ptr` の原理的な限界です。

`use_count` が `Bad` で 2、`Good` で 1 になっているのが原因そのものです。

### この課題ではどうするか

**親へのリンクは持ちません。** 要らないからです。
必要になったときの選択肢は 2 つです。

| 方法 | 条件 |
| --- | --- |
| 生ポインタ `Entry * parent_` | 子は親に所有されているので、**親が子より先に死ぬことはありえない**。だから安全 |
| `std::weak_ptr<Entry> parent_` | 子を `shared_ptr` で持っている場合のみ。`lock()` が要る |

**子を `unique_ptr` で持っているなら、親は生ポインタで正しい**です。
「生ポインタ = 危険」ではありません。**所有しないポインタである**ことが型で表現できていれば十分です。
`unique_ptr`（所有する）と生ポインタ（所有しない）を使い分けているコードは、読めば所有権が分かります。

## 11.6 再帰的なデストラクタとスタック

`unique_ptr` の連鎖は自動的に解放されますが、**その解放は再帰呼び出し**です。
深い木を捨てると、デストラクタのネストがそのままスタックを食います。

20 万段の連結リストを `unique_ptr` で作って解放した実測です。

```cpp
struct N { std::unique_ptr<N> next; };
// 20 万段つないでから head.reset();
```

```
built
（ここで SIGSEGV。exit code 139）
```

**構築は成功し、解放でスタックオーバーフローしました。** `freed` は出ていません。
macOS のメインスレッドのスタックは 8 MB あってこれです。
マイコンのスタックは **数 KB 〜 数十 KB** なので、数百段でも危ういと考えてください。

診断項目の木のように**深さが 3〜4 段**なら何の問題もありません。
問題になるのは、木ではなく連結リストを木のクラスで表してしまったときや、
パーサの構文木のように深さが入力依存になるときです。
その場合は、デストラクタで明示的なループ（自分のスタックを持って解体する）を書きます。

**深さが構造的に決まっているか、入力で決まるか**を最初に確認してください。

## 11.7 標準ライブラリ／言語機能に同じものが無いか

**ありません。** Composite に相当する汎用のクラステンプレートは標準ライブラリにありません。

近いものとして `std::filesystem::directory_entry` と
`std::filesystem::recursive_directory_iterator` がありますが、これは
「ファイルシステムという既存の木を走査する道具」であって、
**自分の木を組み立てる道具ではありません**。

木を型で表す方法として、C++ には Java に無い選択肢が 1 つあります。**入れ子の型**です。

```cpp
// 構造がコンパイル時に決まっているなら、これも Composite の一種
template <typename... Children>
class Group;
```

`std::tuple` で子を持てば、仮想関数も動的確保もゼロで再帰的な集計ができます。
コンパイル時に木の形が決まっている場合に限りますが、マイコンでは強力です（11.8）。

## 11.8 手元で試す

課題を解く前に、この 2 本を**出力を予想してから**実行してください。

### その1: スライシング

```cpp
#include <iostream>
#include <memory>
#include <vector>

// あえて抽象クラスにしていない。抽象クラスだと vector<Entry> がそもそも作れず、
// 「スライシングして黙って動く」という一番こわい状態を再現できないため。
class Entry
{
public:
  virtual ~Entry() = default;
  virtual int size() const { return 0; }
};

class Check : public Entry
{
public:
  explicit Check(int size) : size_(size) {}
  int size() const override { return size_; }

private:
  int size_;
};

int main()
{
  std::vector<Entry> by_value;
  by_value.push_back(Check{100});
  std::cout << "vector<Entry>            : " << by_value[0].size() << "\n";

  std::vector<std::unique_ptr<Entry>> by_pointer;
  by_pointer.push_back(std::make_unique<Check>(100));
  std::cout << "vector<unique_ptr<Entry>>: " << by_pointer[0]->size() << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: 上の行は何が出るか。警告は出るか</summary>

```
vector<Entry>            : 0
vector<unique_ptr<Entry>>: 100
```

**警告は 1 つも出ません。** `-Wall -Wextra -Wpedantic` でも黙って通ります。

`push_back(Check{100})` は `Entry` のコピーコンストラクタを呼び、
`Check` の `size_` と vtable を捨てています。残った `Entry` の `size()` は 0 を返します。

`Entry::size()` を `= 0`（純粋仮想）にすると、`push_back` した時点で
`error: allocating an object of abstract class type 'Entry'` になります。
**純粋仮想関数を 1 つ入れておくと、この事故がコンパイル時に止まります。**
Composite の基底クラスを抽象に保つ実用上の理由です。
</details>

### その2: 親を `shared_ptr` で持つと解放されない

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct Bad
{
  explicit Bad(std::string name) : name_(std::move(name)) {}
  ~Bad() { std::cout << "~Bad " << name_ << "\n"; }

  std::vector<std::shared_ptr<Bad>> children;
  std::shared_ptr<Bad> parent;        // 親も shared_ptr = 循環する
  std::string name_;
};

struct Good
{
  explicit Good(std::string name) : name_(std::move(name)) {}
  ~Good() { std::cout << "~Good " << name_ << "\n"; }

  std::vector<std::shared_ptr<Good>> children;
  std::weak_ptr<Good> parent;        // 親は weak_ptr。所有しない
  std::string name_;
};

int main()
{
  {
    auto root = std::make_shared<Bad>("root");
    auto leaf = std::make_shared<Bad>("leaf");
    root->children.push_back(leaf);
    leaf->parent = root;
    std::cout << "Bad  root.use_count = " << root.use_count() << "\n";
  }
  std::cout << "--- Bad のスコープを抜けた ---\n";

  {
    auto root = std::make_shared<Good>("root");
    auto leaf = std::make_shared<Good>("leaf");
    root->children.push_back(leaf);
    leaf->parent = root;
    std::cout << "Good root.use_count = " << root.use_count() << "\n";
  }
  std::cout << "--- Good のスコープを抜けた ---\n";
  return 0;
}
```

<details>
<summary>予想: <code>~Bad</code> は何回出るか</summary>

**0 回です。**

```
Bad  root.use_count = 2
--- Bad のスコープを抜けた ---
Good root.use_count = 1
~Good root
~Good leaf
--- Good のスコープを抜けた ---
```

`use_count` の差がすべてです。`Bad` は `leaf->parent` が `root` を握っているので 2。
スコープを抜けて `root` 変数が消えても 1 残り、解放されません。
`leaf` も `root->children` から握られたままなので、2 個ともリークします。

`Good` は `parent` が `weak_ptr` なのでカウントを増やしません。1 のまま → 0 になり、
`root` → `children` → `leaf` の順に解放されます。

**このプログラムはクラッシュもエラーも出しません。** 静かにメモリが残るだけです。
長時間動くノードやマイコンでこれをやると、いずれ確保に失敗します。
</details>

## 11.9 マイコンでの結論

**動的な木構造そのものが使いにくい**、というのが結論です。
`std::vector<std::unique_ptr<Entry>>` は、木を 1 個組み立てるだけで
ノードの数だけヒープ確保が走ります。起動時に 1 回だけならまだしも、
実行中に木を組み替えるコードは書けません。

### 方法1: 構造がコンパイル時に決まるなら、`const` な配列に置く

診断項目の木は**電源投入時にはもう形が決まっています**。だったら ROM に置きます。

```cpp
#include <cstddef>
#include <cstdint>

struct DiagNode
{
  const char * name;
  bool (*check)();            // 葉なら関数ポインタ。節なら nullptr
  std::uint8_t first_child;   // 節のときだけ意味を持つ
  std::uint8_t child_count;
};

bool check_imu_whoami();
bool check_imu_bias();
bool check_motor_l();

// 木は「配列 + インデックス」で表す。ポインタも動的確保も vtable も無い。
// constexpr なので ROM に置かれ、RAM を 1 バイトも使わない。
constexpr DiagNode kTree[] = {
  {"robot",      nullptr,           1, 2},  // 0: 子は 1,2
  {"imu",        nullptr,           3, 2},  // 1: 子は 3,4
  {"motors",     nullptr,           5, 1},  // 2: 子は 5
  {"imu_whoami", check_imu_whoami,  0, 0},  // 3: 葉
  {"imu_bias",   check_imu_bias,    0, 0},  // 4: 葉
  {"motor_l",    check_motor_l,     0, 0},  // 5: 葉
};

/// index 以下をすべて実行し、合格数を返す。葉と節を同じ関数で扱える。
std::uint8_t run_diagnostics(std::uint8_t index)
{
  const DiagNode & node = kTree[index];
  if (node.check != nullptr) {
    return node.check() ? 1 : 0;
  }
  std::uint8_t passed = 0;
  for (std::uint8_t i = 0; i < node.child_count; ++i) {
    passed = static_cast<std::uint8_t>(passed + run_diagnostics(static_cast<std::uint8_t>(node.first_child + i)));
  }
  return passed;
}
```

**葉と節を同一視する**という Composite の目的は、これで果たせています。
`run_diagnostics(0)` を呼ぶ側は、0 番が葉なのかグループなのかを知りません。

得たもの: 確保ゼロ、vtable ゼロ、`std::string` ゼロ。
`DiagNode` は 1 個あたりポインタ 2 個 + 2 バイトで、ROM に置かれます。

失ったもの: 実行時に木を組み替えられません。**それでいいかを先に確認してください。**
部活のロボットで、診断項目を実行中に足す場面はまず来ません。

再帰は残っていますが、**深さが `kTree` の形で決まっている**ので上限が読めます。
気になるならインデックスのスタックを自分で持ってループにします。

### 方法2: 実行時に組み替えたいなら、ノードプールを固定長で持つ

```cpp
template <std::size_t Capacity>
class DiagTree
{
public:
  /// 追加できたら index、満杯なら Capacity を返す（例外は投げない）
  std::size_t add_check(const char * name, bool (*check)());

private:
  DiagNode nodes_[Capacity] = {};
  std::size_t used_ = 0;
};
```

確保は**起動時にこのオブジェクトを 1 個作るときだけ**です。
満杯のときに `throw` できない（`-fno-exceptions`）ので、
**失敗を戻り値で返します**。呼び側は起動時に 1 回だけ確認すれば済みます。

### `std::unique_ptr` 版を使ってよい場合

起動時に木を 1 回組んで、あとは実行するだけ、かつヒープが十分にある
（RTOS 上で数十 KB 使える）なら、課題の実装をそのまま持ち込んで構いません。
**ループの中で `add()` を呼ばないこと**だけが条件です。

## 11.10 ROS 2 での結論（補足）

ROS 2 では、木を**クラスの入れ子ではなく `/` 区切りの名前で平坦に**表すことが多いです。

- パラメータの `motors.left.max_duty` は、階層に見えて中身は文字列キーです
- 診断メッセージも、名前に階層を埋め込んでフラットな配列で送ります

この課題の `full_names()` が `"/robot/imu/imu_whoami"` を作っているのは、
**木を組み立てるのはノード内部、外に出すときは平坦な名前**という
ROS 2 での実際の使い分けに合わせているからです。
木のまま話すのはノードの中だけです。

なお、ROS 2 の「コンポジション」（`ComposableNode`、複数ノードを 1 プロセスに載せる仕組み）は
**Composite パターンとは無関係**です。名前が似ているだけなので混同しないでください。

## 11.11 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `error: call to implicitly-deleted copy constructor of 'std::unique_ptr<...>'` | `children_.push_back(child)` に `std::move` が無い |
| `add()` に変数を渡したらコンパイルエラー | 名前付き変数は左辺値。`add(std::move(group))` と書く |
| `add()` の後に元の変数を使ったら落ちた | ムーブ済みで `nullptr`。ムーブしたら二度と使わない |
| 木を捨てても子のデストラクタが呼ばれない | 基底の仮想デストラクタが無い |
| 集計がいつも 0 | 子を `std::vector<Entry>` で持ってスライシングしている |
| メモリが減り続ける。クラッシュはしない | 親を `shared_ptr` で持って循環している。`weak_ptr` か生ポインタへ |
| `Group` を返す関数を書いたらムーブできないと言われた | デストラクタを自分で書いたのでムーブが暗黙生成されていない。5 つ明示する |
| 深い木を捨てたら SIGSEGV | デストラクタの再帰でスタックオーバーフロー（11.6） |
| 葉に `add()` して実行時に落ちる設計にしてしまった | `add()` は節にだけ置く（11.1 変更点2） |

## 11.12 対応する課題

```bash
./drill run dp11
```

題材は**ロボットの起動時自己診断**です。
「IMU の WHO_AM_I を読む」のような個別の診断（葉）と、
「IMU 関連の診断ひとまとめ」のようなグループ（節）を同一視して、
**木全体を 1 回の `run()` で実行し、合否を集計します**。

`exercises/dp11_composite/src/diagnostic_tree.cpp` に実装します。

1. **`DiagnosticCheck`（葉）** — `check_count()` / `run()` / `collect_names()`
2. **`DiagnosticGroup::add()`** — `std::unique_ptr<DiagnosticEntry>` を値で受けてムーブで入れる
3. **`DiagnosticGroup`（節）** — 子に再帰して集計する `check_count()` / `run()` / `collect_names()`

テストが見るのは次の 4 点です。

- 葉と節が `const DiagnosticEntry *` の同じ配列に入り、同じ呼び方で集計できること
- 再帰的な集計（葉の総数、合格数・不合格数）が正しいこと
- **親を破棄すると子も破棄されること**（デストラクタのログを観測します）
- **`DiagnosticGroup` がコピー不可でムーブ可能であること**（`static_assert`）

`delete` は 1 行も書きません。書きたくなったら設計が間違っています。

## 11.13 この章のまとめ

- 構造は Java 版とほぼ同じ。**違うのは所有権だけ**
- 子は `std::vector<std::unique_ptr<Entry>>`。
  値だと**スライシング**、生ポインタだと**解放者が不明**、`shared_ptr` は理由が無ければ過剰
- `unique_ptr` を持つクラスは**コピーできない**。`add()` は値で受けて `std::move` で入れる
- デストラクタを自分で書いたら、**コピー/ムーブの 5 つを明示**する
- **親を `shared_ptr` で持つと循環して永久に解放されない**。親は生ポインタか `weak_ptr`
- 子を `unique_ptr` で持つなら、**親は生ポインタで正しい**（親が先に死なないため）
- 解放は再帰。**深い木はスタックオーバーフローしうる**。深さが入力依存かを確認する
- `add()` は基底に置かず**節にだけ**置く。結城本の実行時例外がコンパイルエラーになる
- マイコンでは動的な木を作らない。**`constexpr` な配列 + インデックス**で木を表す

---

前: [10. Strategy](10_Strategy.md) ／ 次: 12. Decorator（準備中）
