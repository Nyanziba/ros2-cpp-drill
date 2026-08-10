# 15. Facade

> **結城本 第15章 対応。** `PageMaker` / `Database` / `HtmlWriter` を手元に開いてください。
>
> **この章のねらい**: このパターンは、**C++ では半分が言語機能に吸収されます**。
> 結城本の `PageMaker` は「private コンストラクタ + static メソッド 1 個」のクラスですが、
> C++ にはトップレベルの自由関数と名前空間があるので、**クラスにする理由がありません**。
> 残る半分 —「初期化して、使って、後始末する」窓口 — が C++ 版 Facade の本命で、
> ここは **RAII** で書きます。Java の Facade には無い仕事です。

## 15.1 まず、たいていの Facade はクラスではなく関数でよい

結城本の `PageMaker` はこうです。

```java
public class PageMaker {
    private PageMaker() {                       // インスタンスを作らせない
    }
    public static void makeWelcomePage(String mailaddr, String filename) {
        // Database と HtmlWriter を組み合わせる
    }
}
```

**`private PageMaker() {}` という行が、この設計のすべてを語っています。**
「これはクラスではない。ただ Java にはクラスの外に関数を置く方法が無いだけだ」という
注釈です。C++ にはその制約がありません。

```cpp
// robot_startup.hpp
namespace robot
{
StartupResult start_once(const StartupConfig & config);
}
```

これで終わりです。`class` は 1 文字も要りません。

### `static` メンバ関数だけのクラスを書かない

C++ に移すときに、こう書きたくなります。**書かないでください。**

```cpp
// 悪い例：Java の癖がそのまま出ている
class RobotStarter
{
public:
  RobotStarter() = delete;
  static StartupResult start_once(const StartupConfig & config);
};
```

得るものがありません。失うものは 3 つあります。

| 論点 | `static` メンバだけのクラス | 名前空間 + 自由関数 |
| --- | --- | --- |
| 呼び出し | `RobotStarter::start_once(c)` | `robot::start_once(c)`（`using` で短縮も可） |
| 関数を 1 つ足す | ヘッダのクラス定義を書き換える → **全利用者が再コンパイル** | 別のヘッダに足せる。分割できる |
| ADL（実引数依存探索） | 効かない | 効く |
| 名前空間をまたいで分割 | できない（クラスは 1 箇所で閉じる） | できる |

2 番目が実務では効きます。クラスは「開いていない」ので、
`start_once` の隣に `start_for_test` を足したいだけで、
そのヘッダを include している全 `.cpp` が再コンパイルされます。
名前空間は開いているので、別ヘッダに足せば足した側だけで済みます。

**判断基準はひとつです。「状態を持つか」。**

- 持たない（引数を受けて、やって、返すだけ）→ **名前空間 + 自由関数**
- 持つ（初期化して、使って、後始末する）→ **クラス。それも RAII クラス**

第0章の「実装が 1 つしかないのに抽象化する」と同じ話です。
`class` と書いた瞬間に、読む人は「状態があるのか」「複数個作るのか」「継承するのか」を
探し始めます。**答えが全部「いいえ」なら、それは関数です。**

## 15.2 Java 版をそのまま C++ にすると

課題の題材で見ます。「ロボットを起動する」の裏に 4 手順があります。

```
電源投入 → センサ初期化 → キャリブレーション → 通信確立
```

### 変更点1: `PageMaker` クラス → `namespace robot` + 自由関数

15.1 のとおりです。結城本の `makeWelcomePage` に相当するのが `robot::start_once()` です。

```cpp
namespace robot
{
StartupResult start_once(const StartupConfig & config, std::vector<std::string> * log = nullptr);
}
```

### 変更点2: 戻り値を `void` から `StartupResult` にした

Java 版の `makeWelcomePage` は `void` で、失敗したら例外です。
C++ では**どの段で失敗したか**を値で返します。

```cpp
struct StartupResult
{
  bool ok = false;
  StartupStage failed_stage = StartupStage::kPower;
};
```

理由は第14章と同じです。マイコンでは `-fno-exceptions` が普通なので、
`throw` を前提にした API はそのまま持ち込めません。
「起動に失敗した」は**想定内**であって、例外の出番ではありません。

### 変更点3: 内部クラスをヘッダから消した

結城本の `Database` / `HtmlWriter` は `package` スコープ（アクセス修飾子なし）で、
パッケージの外からは見えません。
C++ に `package` はありませんが、**もっと強い手段があります**。
サブシステムを `.cpp` の無名名前空間に置けば、**他の翻訳単位からは名前すら存在しません**。
15.6 でやります。

### 変わらない点: Facade は薄い

結城本が強調しているとおりです。Facade に判断を書き始めたら、それは Facade ではありません。
C++ でも同じで、**Facade の中身は「呼ぶ順番」だけ**であるべきです。

## 15.3 状態を持つ Facade は RAII で書く — こちらが本命

`start_once()` は「起動して、その場で止める」窓口です。
実際に欲しいのは「起動して、**使って**、止める」の方でしょう。
ここで初めてクラスが正当化されます。

```cpp
class RobotSession
{
public:
  explicit RobotSession(StartupConfig config, std::vector<std::string> * log = nullptr);
  ~RobotSession();

  RobotSession(const RobotSession &) = delete;
  RobotSession & operator=(const RobotSession &) = delete;
  RobotSession(RobotSession && other) noexcept;
  RobotSession & operator=(RobotSession &&) = delete;

  bool is_ready() const;
  StartupStage failed_stage() const;
  bool drive(int duty);

private:
  StartupConfig config_;
  std::vector<std::string> * log_;
  int completed_stages_;
  bool ready_;
  StartupStage failed_stage_;
};
```

使う側はこうです。

```cpp
{
  RobotSession session{StartupConfig{}};
  if (!session.is_ready()) {
    return 1;
  }
  session.drive(50);
}   // ここで通信切断 → キャリブレーション破棄 → センサ停止 → 電源遮断
```

**`stop()` を呼ぶ行がどこにもありません。** これが Java 版との最大の差です。

Java で同じことを書くと、こうなります。

```java
RobotSession session = new RobotSession(config);
try {
    session.drive(50);
} finally {
    session.close();          // 書き忘れたら電源が入りっぱなし
}
```

`try-with-resources` で多少ましになりますが、**`close()` を呼ぶ責任は利用者側にあります**。
C++ ではスコープを抜けるという事実そのものが後始末です。
**「Facade が窓口を 1 つにする」のと「RAII が後始末を 1 箇所にする」は、
別々の話に見えて、この形では同じ 1 つの型に同居します。**

### `init()` を呼ばせるか、コンストラクタでやるか

RAII なのだからコンストラクタでやるのが原則です。ただし例外が 2 つあります。

1. **オブジェクトが静的記憶域にある**とき（マイコン）。
   起動直後はクロックもスタックも整っていないので、
   静的初期化のタイミングでペリフェラルを触ってはいけません。15.9 で扱います
2. **失敗を戻り値で返したい**が、例外が使えないとき。
   コンストラクタには戻り値がありません

2 番については、この課題では **「コンストラクタは失敗しても投げず、`is_ready()` で聞く」**
という形を採りました。「構築はできたが動作可能ではない」状態を許す設計です。
賛否あります。厳しくやるなら、

```cpp
static std::optional<RobotSession> create(StartupConfig config);   // 失敗なら nullopt
```

というファクトリにして、コンストラクタを `private` にすれば
「`RobotSession` が存在する = 起動済み」という不変条件が型で保証できます。
**部活のライブラリではこちらの方が事故が減ります。**
課題で `is_ready()` 方式にしたのは、失敗時の巻き戻しをテストから観測させるためです。

## 15.4 誰が所有するのか

Facade は「サブシステムをまとめる」パターンなので、
**Facade がサブシステムを所有するのか、借りているだけなのか**を必ず決めます。
Java では GC のおかげでこの問いが発生しません。

3 通りあります。

| 形 | 書き方 | 使う場面 |
| --- | --- | --- |
| **所有する（値メンバ）** | `PowerRail power_;` | サブシステムが Facade 専用。**まずこれ** |
| **所有する（`unique_ptr`）** | `std::unique_ptr<PowerRail> power_;` | 型をヘッダに出したくない（Pimpl）、多態が要る |
| **借りる（参照 / 生ポインタ）** | `PowerRail & power_;` | サブシステムが Facade より長生きすると**保証できる**とき |

3 番目は第14章と同じ罠です。借りている相手が先に死ぬと未定義動作になります。
「Facade は寄せ集めるだけだから所有はしない」という設計は自然に聞こえますが、
**寿命を保証するのが誰かを書けないなら選んではいけません**。

課題の `RobotSession` はサブシステムを 1 個も持っていません。
状態は「どこまで初期化したか」だけです。

```cpp
int completed_stages_;   ///< 0〜4
```

**これで足りるなら、これが最善です。** サブシステムがハードウェアそのもの
（電源 IC、IMU、CAN コントローラ）で、C++ 側にオブジェクトとして持つ実体が無いとき、
Facade が持つべき状態は「どこまで進んだか」だけになります。

## 15.5 C++ 固有の危険 — 途中で失敗したときの巻き戻し

**この章でいちばん事故が起きるのがここです。**

4 手順のうち 3 番目で失敗したら、**すでに済んだ 2 つを逆順で戻す**必要があります。
Java でも同じですが、C++ には道具が 2 つあり、選択を間違えると壊れます。

### (a) 段数を数えて、後始末を 1 箇所に集める

課題で書くのがこれです。

```cpp
void teardown(int completed_stages, std::vector<std::string> * log)
{
  if (completed_stages >= 4) { link_down(log); }
  if (completed_stages >= 3) { calibration_clear(log); }
  if (completed_stages >= 2) { sensor_deinit(log); }
  if (completed_stages >= 1) { power_off(log); }
}
```

`if` を `>=` の降順に並べるのが要点で、**フォールスルーではなく独立した `if`** です。
段が増えたときに直すのはここ 1 箇所だけになります。

自由関数版（`start_once`）では、これを**自分で呼ばなければなりません**。

```cpp
if (!calibrate(config, log)) {
  teardown(2, log);                                       // ← 忘れたらリーク
  return StartupResult{false, StartupStage::kCalibration};
}
```

`return` が 5 通りあり、**そのすべてに `teardown()` が要ります**。
1 つ忘れても誰も教えてくれません。これが RAII 版の動機です。

```cpp
RobotSession::~RobotSession()
{
  teardown(completed_stages_, log_);   // 呼び忘れようがない
}
```

**コンストラクタの途中で `return` しても、デストラクタは必ず走ります。**
オブジェクトは構築済みだからです（構築の途中で*例外を投げた*場合は話が別で、
そのときはデストラクタは走りません — 代わりに構築済みのメンバだけが破棄されます）。

### (b) サブシステムを RAII メンバとして並べる

サブシステムが C++ オブジェクトとして実体を持つなら、こちらの方が強い。

```cpp
class Robot
{
public:
  explicit Robot(bool calibration_succeeds)
  : power_("power", true),
    sensor_("sensor", power_.ok()),
    calib_("calib", sensor_.ok() && calibration_succeeds),
    link_("link", calib_.ok())
  {
  }

private:
  Stage power_;    // 宣言順に構築され、
  Stage sensor_;
  Stage calib_;
  Stage link_;     // 逆順に破棄される
};
```

**巻き戻しのコードが 1 行もありません。** メンバは宣言順に構築され、逆順に破棄される、
という言語規則がそのまま「初期化順序」と「後始末順序」になります。

代償が 2 つあります。

1. **メンバは全部構築されます。** 3 段目で失敗しても 4 段目のコンストラクタは走ります。
   だから各 `Stage` が「握っていない（`ok_ == false`）なら何もしない」と自分で判断する必要があります。
   15.8 の出力に `calib FAILED` の直後 `link FAILED` が出るのがこれです
2. **メンバの宣言順が仕様になります。** 誰かが「アルファベット順に並べ替えました」と
   リファクタしただけで、初期化順序が変わって壊れます。
   **`// 宣言順 = 初期化順。並べ替え禁止` とコメントを書いてください**

なお、初期化リストの順序はメンバの宣言順に従い、**書いた順ではありません**。
書いた順と宣言順が違うと `-Wreorder`（`-Wall` に含まれる）が出ます。無視しないでください。

### `explicit` を付ける

```cpp
explicit RobotSession(StartupConfig config, std::vector<std::string> * log = nullptr);
```

デフォルト引数があるので、実質 1 引数で呼べます。`explicit` が無いと、

```cpp
void arm(const RobotSession & session);
arm(StartupConfig{});     // ← 通ってしまう。ここでロボットが起動する
```

**関数を呼んだつもりでハードウェアが起動します。**
副作用が重い型ほど `explicit` は必須です。課題のテストは
`std::is_convertible<StartupConfig, RobotSession>` で見ています。

### コピー禁止・ムーブ可

`RobotSession` は「起動済みのハードウェア 1 台」を表します。コピーに意味がありません。
コピーできてしまうと、コピーが 2 つとも破棄されるときに**後始末が 2 回**走ります。

```cpp
RobotSession(const RobotSession &) = delete;
RobotSession & operator=(const RobotSession &) = delete;
```

ムーブは許します。「起動済みのセッションを関数から返す」ができないと不便だからです。
ただし**ムーブ元を必ず空にします**。

```cpp
RobotSession::RobotSession(RobotSession && other) noexcept
: config_(std::move(other.config_)),
  log_(other.log_),
  completed_stages_(other.completed_stages_),
  ready_(other.ready_),
  failed_stage_(other.failed_stage_)
{
  other.log_ = nullptr;
  other.completed_stages_ = 0;    // ← これを忘れると後始末が 2 回走る
  other.ready_ = false;
}
```

**`std::move` した後のオブジェクトも、デストラクタは走ります。**
「ムーブしたから消えた」ではありません。「中身が空になっただけで、そこにまだ居る」です。
ここを勘違いすると、電源が 2 回落ちます。

ムーブ代入は `= delete` にしました。ムーブ代入には
「代入先がすでに握っている起動状態を、いつ・どの順で落とすか」という判断が要り、
その判断が要らないなら**書かない方が安全**です。
なお、ムーブコンストラクタを自分で書いた時点でコピー系とムーブ代入は暗黙 delete されますが、
第14章と同じ理由で**明示的に書いてください**。

## 15.6 見せる面を減らす — ヘッダに何を書かないか

**Facade の本質は「窓口を 1 つにすること」ではなく「見せる面を減らすこと」です。**
窓口を 1 つにしても、サブシステムがヘッダに全部並んでいたら、何も減っていません。

課題のヘッダ `drill/robot_startup.hpp` に出てくるのは、

- `StartupConfig` / `StartupStage` / `StartupResult`
- `robot::start_once()`
- `robot::RobotSession`

これだけです。`power_on` / `sensor_init` / `calibrate` / `link_up` と、
その後始末の 4 つは `.cpp` の無名名前空間にいます。

```cpp
namespace robot
{
namespace
{
bool power_on(const StartupConfig & config, std::vector<std::string> * log);
void power_off(std::vector<std::string> * log);
// ...
}  // namespace
}  // namespace robot
```

無名名前空間に入れると内部リンケージになり、**他の翻訳単位からは名前が存在しません**。
Java の `package` スコープより強い隠蔽です。得られるものは 3 つ。

1. **利用者が誤って直接呼べない。** 「電源だけ入れてセンサを初期化しない」が起こらない
2. **ヘッダを include するファイルが減る。** `.cpp` 側だけが SPI やレジスタ定義のヘッダを見る
3. **順序を変えても利用者に影響しない。** シグネチャが公開 API でないから

### Pimpl（第9章）との関係

やっていることは Pimpl と同じ方向です。**「実装をヘッダから追い出す」**。
違いは目的です。

| | Facade | Pimpl |
| --- | --- | --- |
| 目的 | **使い方**を単純にする | **コンパイル依存**を切る |
| 何を隠す | サブシステムの存在そのもの | クラスのメンバの型 |
| 公開型の数 | 減る（多 → 1） | 変わらない（1 → 1） |
| 実行時コスト | ゼロ | ポインタ 1 段 + ヒープ 1 回 |

課題のように **Facade が状態をほとんど持たない**なら、
無名名前空間だけで足り、Pimpl は要りません。
Facade がサブシステムを値メンバとして持ちたい、かつその型をヘッダに出したくない、
となって初めて Pimpl を足します。**順番はこの通りです。まず無名名前空間。**

## 15.7 標準ライブラリ／言語機能に同じものが無いか

**あります。しかも大量にあります。** 標準ライブラリは Facade だらけです。

| 標準の Facade | 隠している相手 |
| --- | --- |
| `std::fstream` | `open` / `read` / `write` / `close` + バッファ管理。**RAII 付き Facade の教科書** |
| `std::filesystem::copy_file` | `stat` / `open` × 2 / `read` / `write` / `close` × 2 / パーミッション複製 |
| `std::stoi` | `strtol` + `errno` の確認 + 範囲チェック |
| `std::async` | スレッド生成 + 結果の受け渡し + 例外の転送 |
| `std::lock_guard` | `lock` / `unlock`（Facade というより純粋な RAII） |

`std::fstream` を見ておく価値があります。

```cpp
{
  std::ofstream file{"log.csv"};    // open
  file << "t,v\n";                  // write（バッファ経由）
}                                   // flush して close
```

**「窓口が 1 つ」「後始末が自動」「内部の FILE* は見えない」** の 3 つが揃っています。
15.3 で書いた `RobotSession` は、これと同じ形をロボットの起動シーケンスに当てたものです。
迷ったら `fstream` の形を真似てください。

`std::stoi` も分かりやすい例です。素で書くとこうなります。

```cpp
errno = 0;
char * end = nullptr;
const long value = std::strtol(text, &end, 10);
if (end == text || errno == ERANGE || value > INT_MAX || value < INT_MIN) {
  // ...
}
```

これが `std::stoi(text)` の 1 行になっています。**Facade は行数を減らす道具ではなく、
「間違えられる箇所」を減らす道具です。** 上のコードは 4 通りの間違え方ができます。

### 「あるなら書かない」

第0章のチェック項目そのままです。**自作の前に標準を探してください。**
「ファイルを 1 個コピーする Facade」を自作したら、それは `std::filesystem::copy_file` です。

## 15.8 手元で試す

15.5 (b) の「サブシステムを RAII メンバとして並べる」形です。
**出力を予想してから**実行してください。特に、
`calib` が失敗した回に `link` の行が出るか出ないか、
出るとしたら何が出るかを当ててください。

```cpp
#include <iostream>
#include <string>

namespace
{

int indent = 0;

void trace(const std::string & text)
{
  for (int i = 0; i < indent; ++i) {
    std::cout << "  ";
  }
  std::cout << text << "\n";
}

/// サブシステム 1 個ぶんの RAII。ok_ が false なら「握っていない」。
class Stage
{
public:
  Stage(const char * name, bool succeeds)
  : name_(name), ok_(succeeds)
  {
    trace(ok_ ? std::string{name_} + " up" : std::string{name_} + " FAILED");
  }

  ~Stage()
  {
    if (ok_) {
      trace(std::string{name_} + " down");
    }
  }

  Stage(const Stage &) = delete;
  Stage & operator=(const Stage &) = delete;

  bool ok() const { return ok_; }

private:
  const char * name_;
  bool ok_;
};

/// メンバを並べただけの Facade。順序も後始末も言語がやる。
class Robot
{
public:
  explicit Robot(bool calibration_succeeds)
  : power_("power", true),
    sensor_("sensor", power_.ok()),
    calib_("calib", sensor_.ok() && calibration_succeeds),
    link_("link", calib_.ok())
  {
  }

  bool ready() const { return link_.ok(); }

private:
  Stage power_;
  Stage sensor_;
  Stage calib_;
  Stage link_;
};

}  // namespace

int main()
{
  trace("--- 全部成功 ---");
  indent = 1;
  {
    const Robot robot{true};
    trace(std::string{"ready="} + (robot.ready() ? "true" : "false"));
  }
  indent = 0;

  trace("--- calib で失敗 ---");
  indent = 1;
  {
    const Robot robot{false};
    trace(std::string{"ready="} + (robot.ready() ? "true" : "false"));
  }
  indent = 0;

  trace("--- 抜けた ---");
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>予想: <code>calib</code> が失敗したとき、<code>link</code> の行は出るか</summary>

```
--- 全部成功 ---
  power up
  sensor up
  calib up
  link up
  ready=true
  link down
  calib down
  sensor down
  power down
--- calib で失敗 ---
  power up
  sensor up
  calib FAILED
  link FAILED
  ready=false
  sensor down
  power down
--- 抜けた ---
```

**`link FAILED` は出ます。** ここが Java と発想がずれるところです。
「3 段目でこけたから 4 段目は作られない」ではありません。
**メンバは全部構築されます。** `link_` のコンストラクタは必ず走り、
渡された `calib_.ok()` が `false` だったので「握らなかった」だけです。

そして `down` は `sensor` と `power` の 2 つしか出ません。
**握っていないメンバのデストラクタは走っていますが、中で何もしていない**からです。
「デストラクタが走るかどうか」と「後始末が起きるかどうか」は別の話で、
後者は各 `Stage` が自分の `ok_` で判断しています。

`down` の順番が `link → calib → sensor → power` なのは、
メンバが**宣言順の逆**に破棄されるからです。
`Stage power_;` を宣言の一番下に移すと、電源が最初に落ちてから
センサを止めることになります。**メンバの並び順が仕様です。**
</details>

## 15.9 マイコンでの結論

方針は 3 つです。

1. **状態を持たない Facade は、名前空間 + 自由関数。** `static` メンバだけのクラスは書かない
2. **状態を持つ Facade は 1 個だけ静的記憶域に置く。** ヒープは使わない
3. **コンストラクタでペリフェラルを触らない。`init()` を明示的に呼ばせる**

3 番が macOS / ROS 2 との最大の差です。理由を書きます。

グローバルオブジェクトのコンストラクタは `main()` の**前**に走ります
（スタートアップコードが `__libc_init_array` 相当で回します）。
その時点でクロック設定が終わっている保証はありません。
**「起動直後で、まだ PLL が立っていない状態で SPI を叩く」**という事故になります。
さらに、他のグローバルとの初期化順序は翻訳単位をまたぐと**不定**です（第5章 Singleton の話）。

なので、**コンストラクタは何もしない `constexpr` にして、
`main()` からクロック設定のあとに `init()` を呼びます**。

```cpp
#include <cstdio>

namespace
{

// --- Facade が隠す相手。ヘッダには出さない ---------------------------------
// クラスにしない。状態はペリフェラルのレジスタ側にあり、C++ 側に持つ必要が無い。

bool clock_enable()
{
  std::printf("clock on\n");
  return true;
}
void clock_disable() { std::printf("clock off\n"); }

bool power_rail_on(int battery_mv)
{
  if (battery_mv < 11000) {
    std::printf("power FAILED (%d mV)\n", battery_mv);
    return false;
  }
  std::printf("power on\n");
  return true;
}
void power_rail_off() { std::printf("power off\n"); }

bool imu_init()
{
  std::printf("imu init\n");
  return true;
}
void imu_deinit() { std::printf("imu deinit\n"); }

bool can_open()
{
  std::printf("can open\n");
  return true;
}
void can_close() { std::printf("can close\n"); }

constexpr int kStageCount = 4;

/// 状態（どこまで初期化したか）を持つので、ここだけクラスにする。
/// ヒープは使わない。std::string も std::optional も使わない。
class RobotFacade
{
public:
  /// 何もしない。静的記憶域に置いても他の静的オブジェクトに依存しない。
  constexpr RobotFacade() = default;

  ~RobotFacade() { deinit(); }

  RobotFacade(const RobotFacade &) = delete;
  RobotFacade & operator=(const RobotFacade &) = delete;

  /// クロックが立ち、スタックが用意できた**あと**に main から明示的に呼ぶ。
  bool init(int battery_mv)
  {
    if (!clock_enable()) {
      return false;
    }
    ++stages_;
    if (!power_rail_on(battery_mv)) {
      return false;
    }
    ++stages_;
    if (!imu_init()) {
      return false;
    }
    ++stages_;
    if (!can_open()) {
      return false;
    }
    ++stages_;
    return true;
  }

  void deinit()
  {
    if (stages_ >= 4) { can_close(); }
    if (stages_ >= 3) { imu_deinit(); }
    if (stages_ >= 2) { power_rail_off(); }
    if (stages_ >= 1) { clock_disable(); }
    stages_ = 0;
  }

  bool is_ready() const { return stages_ == kStageCount; }

private:
  int stages_ = 0;
};

}  // namespace

// 静的記憶域に 1 個。確保はゼロ。
RobotFacade g_robot;

int main()
{
  std::printf("--- 12.0 V ---\n");
  if (g_robot.init(12000)) {
    std::printf("ready\n");
  }
  g_robot.deinit();

  std::printf("--- 10.5 V ---\n");
  if (!g_robot.init(10500)) {
    std::printf("not ready\n");
  }
  g_robot.deinit();
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti mcu.cpp -o mcu && ./mcu
```

```
--- 12.0 V ---
clock on
power on
imu init
can open
ready
can close
imu deinit
power off
clock off
--- 10.5 V ---
clock on
power FAILED (10500 mV)
not ready
clock off
```

電圧が足りない回で、**`clock off` は出るが `power off` は出ない**ことを確認してください。
`stages_` が 1 で止まっているので、握っていないものを解放しに行きません。

ポイントを 5 つ。

- **`constexpr RobotFacade() = default;`。** コンストラクタが何もしないので、
  `g_robot` は静的初期化（`.bss` をゼロクリアするだけ）で済み、
  `main()` 前に走るコードがありません。**静的初期化順序問題が消えます**
- **`init()` は `main()` から明示的に呼ぶ。** RAII の原則からは外れますが、
  マイコンでは「コンストラクタが走るタイミング」を制御できないので、こちらが正解です
- **デストラクタは書いておくが、当てにしない。** 組込みの `main()` は普通
  `while (true)` で抜けないので `~RobotFacade()` は走りません。
  `deinit()` を公開して、スリープ遷移や再起動シーケンスから呼べるようにしておきます
- **`std::string` / `std::optional` / 例外を使わない。** 名前は `const char *`、
  失敗は `bool`、詳細が要るなら `enum` を返します
- **仮想関数がゼロ。** Facade に多態は要りません。**サブシステムを差し替えたいなら
  それは Strategy（第10章）か Bridge（第9章）であって、Facade ではありません**

### `init()` を 2 回呼べるようにするか

上のコードは `deinit()` してから `init()` を呼び直せます。
**そうしたくないなら `init()` の先頭で弾いてください。**

```cpp
bool init(int battery_mv)
{
  if (stages_ != 0) {
    return is_ready();     // すでに初期化済み。二重初期化はしない
  }
  // ...
}
```

部活のコードで実際に起きるのは「エラーからの復帰処理でもう一度 `init()` を呼ぶ」です。
**どちらの仕様なのかをヘッダのコメントに 1 行書いておいてください。**

## 15.10 ROS 2 での結論（補足）

rclcpp 自体が Facade の塊です。

- `rclcpp::init()` — rcl / rmw / DDS の初期化をまとめた窓口。**まさに `PageMaker`**
- `Node::create_publisher<T>()` — QoS 変換、型サポート取得、rmw の publisher 生成をまとめる
- `rclcpp::spin(node)` — executor の生成・追加・実行ループをまとめる

自作するなら、**ノードを Facade にしない**でください。よくある失敗が、

```cpp
// 悪い例：ノードが全部の窓口になっている
class RobotNode : public rclcpp::Node
{
  // publisher 10 個、subscriber 8 個、サービス 4 個、状態機械、制御則、ログ整形…
};
```

これは Facade ではなく**ただの巨大クラス**です。Facade は薄いはずでした。
制御則やハードウェア抽象は ROS 2 に依存しない素の C++ クラスに切り出し、
ノードは「トピックとその素のクラスをつなぐだけ」にしてください。
**そうしておくと、同じクラスがマイコン側でもそのまま使えます。**
このトラックの課題がすべて素の C++17 で書かれているのはそのためです。

`rclcpp::init` / `shutdown` を RAII で包むのは有効です。

```cpp
class RclcppContext
{
public:
  RclcppContext(int argc, char ** argv) { rclcpp::init(argc, argv); }
  ~RclcppContext() { rclcpp::shutdown(); }
  RclcppContext(const RclcppContext &) = delete;
  RclcppContext & operator=(const RclcppContext &) = delete;
};
```

テストで `shutdown()` を呼び忘れて次のテストが落ちる、という事故が消えます。

## 15.11 つまずきポイント

| 症状 | 原因 |
| --- | --- |
| `RobotStarter::start()` と書きたくなる | Java の癖。状態が無いなら名前空間 + 自由関数 |
| ヘッダに関数を 1 つ足しただけで全体が再コンパイルされる | `static` メンバだけのクラスにしている。名前空間なら分割できる |
| 起動に失敗したのに電源が入りっぱなし | 巻き戻しを書いていない。`return` の全経路で `teardown()` が要る |
| 後始末が 2 回走る（電源が 2 回落ちる） | ムーブ元を空にしていない。`std::move` してもデストラクタは走る |
| 関数を呼んだだけでロボットが起動した | コンストラクタに `explicit` が無く、暗黙変換された |
| メンバを並べ替えたら初期化順序が変わって壊れた | メンバの宣言順 = 初期化順。`-Wreorder` を無視しない |
| 3 段目で失敗したのに 4 段目のログが出る | メンバは全部構築される。各段が `ok_` を見て「何もしない」を選ぶ |
| `main()` の前にハングする（マイコン） | グローバルのコンストラクタでペリフェラルを触っている。`init()` を分ける |
| デストラクタが呼ばれない（マイコン） | `main()` が `while (true)` で抜けない。`deinit()` を公開しておく |
| Facade が 2000 行になった | 判断や制御則を Facade に書いている。Facade は「呼ぶ順番」だけ |
| サブシステムを差し替えたくなった | それは Facade の仕事ではない。Strategy（第10章）か Bridge（第9章） |

## 15.12 対応する課題

```bash
./drill run dp15
```

`exercises/dp15_facade/src/robot_startup.cpp` に、ロボットの起動シーケンス
（電源投入 → センサ初期化 → キャリブレーション → 通信確立）の窓口を実装します。

1. **サブシステム 8 個**（`power_on` / `power_off` / `sensor_init` / `sensor_deinit` /
   `calibrate` / `calibration_clear` / `link_up` / `link_down`）—
   無名名前空間の中の自由関数。ヘッダには出しません
2. **`teardown()`** — 完了した段数を受け取り、**逆順**で後始末する
3. **`robot::start_once()`** — 名前空間 + 自由関数版の Facade。
   全 `return` 経路で `teardown()` を呼ぶ
4. **`RobotSession` のコンストラクタ** — 起動シーケンスを走らせる。**後始末は書かない**
5. **`RobotSession` のデストラクタ** — `teardown()` を呼ぶだけ
6. **`RobotSession` のムーブコンストラクタ** — ムーブ元を空にする
7. **`RobotSession::drive()`** — 起動できていなければ何もしない

テストが見るのは、

- Facade を 1 回作るだけで、内部の 4 手順が**正しい順序で**全部走ること
- スコープを抜けると後始末が**逆順で**走ること
- 途中で失敗したらそれ以降が走らず、**すでに初期化したものだけ**が巻き戻ること
  （電源で失敗 → 後始末ゼロ / センサで失敗 → 電源だけ / キャリブで失敗 → 2 段）
- **名前空間 + 自由関数版と RAII クラス版のログが完全に一致する**こと
- ムーブしても後始末が**一度だけ**走ること
- コピー禁止・ムーブ構築可・ムーブ代入禁止・`explicit` であること（`static_assert`）

## 15.13 この章のまとめ

- 結城本の `PageMaker` は「Java にはクラス外の関数が無い」ことの産物。
  C++ では **名前空間 + 自由関数**にする。**`static` メンバだけのクラスを書かない**
- 分かれ目は **「状態を持つか」** の 1 点。持たないなら関数、持つならクラス
- 状態を持つ Facade は **RAII**。コンストラクタで初期化、デストラクタで後始末。
  **これが C++ 版 Facade の本命**で、Java 側には `close()` を呼ぶ責任が残る
- 途中で失敗したときの**巻き戻し**が C++ 側だけの仕事。
  段数を数えて `teardown()` に集めるか、サブシステムを RAII メンバとして並べる
- メンバは**宣言順に構築、逆順に破棄**。並べ替えは仕様変更。`-Wreorder` を見る
- **コピー禁止・ムーブ可**。ムーブ元を空にしないと**後始末が 2 回**走る
- 副作用が重い型のコンストラクタには必ず **`explicit`**
- Facade の本質は「見せる面を減らすこと」。サブシステムは **`.cpp` の無名名前空間**へ。
  足りなければ Pimpl（第9章）を足す
- 標準ライブラリは Facade だらけ（`fstream` / `filesystem::copy_file` / `stoi` / `async`）。
  **自作の前に探す**
- マイコンでは **`constexpr` コンストラクタ + 明示的な `init()`**。
  `main()` 前にペリフェラルを触らない。ヒープも仮想関数も要らない
- Facade は薄い。**判断や差し替えを書き始めたら、それは Strategy か Bridge**

---

前: [14. Chain of Responsibility](14_ChainOfResponsibility.md) ／ 次: 16. Mediator（準備中）
