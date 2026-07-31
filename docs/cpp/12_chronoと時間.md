# 12. `std::chrono` と時間

> **この章のねらい**: ドリルに **`chrono_literals` が 28 回**出てきます。
> `500ms` や `2s` といった書き方は、`using namespace std::chrono_literals;` という
> 1 行があることで初めて成立します。
> この「数字のあとに `ms` や `s` が付く」という書き方（**ユーザー定義リテラル**）は、
> C++ の高度な機能ですが、ROS 2 では「当たり前に使う」ものです。
> さらに時間の型（`std::chrono::duration`）は、異なる単位を別の型として扱うため、
> 単純に足し算しても「型の違いによる落とし穴」が起きます。
> また ROS 2 は `std::chrono` とは別に、独自の時間型（`rclcpp::Duration` / `rclcpp::Time`）を持ち、
> これらは**シミュレーション時間に対応**します。この違いを知らないと、
> 「実ロボットでは動くが、シミュレータでタイムアウトする」という謎のバグが起きます。

## 12.1 ユーザー定義リテラル — `500ms` の仕組み

### `using namespace std::chrono_literals`

```cpp
using namespace std::chrono_literals;

auto delay = 500ms;  // std::chrono::milliseconds(500)
auto second = 1s;    // std::chrono::seconds(1)
auto microsec = 100us;  // std::chrono::microseconds(100)
```

`500ms` という書き方は、**C++ コンパイラが予約語として持っている**わけではなく、
`std::chrono_literals` というネームスペースで **ユーザー定義リテラル演算子** `operator""ms` が定義されているからです。

```cpp
// C++ 標準ライブラリの内部（を単純化したもの）
namespace std::chrono_literals {
  constexpr std::chrono::milliseconds operator""ms(unsigned long long count) {
    return std::chrono::milliseconds(count);
  }
}
```

`using namespace` でそのネームスペースの中身をグローバルに持ってくるので、
`500ms` が通じるようになります。

実装には、ユーザー定義リテラルなしで書くこともできます。

```cpp
// 同じ意味だが、冗長
auto delay = std::chrono::milliseconds(500);
```

ドリルの `solutions/01_publisher/src/minimal_publisher.cpp` はこの形です。

```cpp
using namespace std::chrono_literals;
timer_ = this->create_wall_timer(500ms, std::bind(&MinimalPublisher::timer_callback, this));
```

### 利用可能なリテラル

```cpp
using namespace std::chrono_literals;

std::chrono::nanoseconds ns(100ns);         // ナノ秒
std::chrono::microseconds us(100us);        // マイクロ秒
std::chrono::milliseconds ms(100ms);        // ミリ秒
std::chrono::seconds s(100s);               // 秒
std::chrono::minutes min(100min);           // 分
std::chrono::hours h(100h);                 // 時間
```

## 12.2 `std::chrono::duration` — 時間の型

`duration` は**テンプレート**で、「数値の型」と「時間の単位」の両方で定まります。

```cpp
template<typename Rep, typename Period>
class duration {
  // ...
};
```

typedef で簡略化されているので、通常は以下の短い型名を使います。

```cpp
std::chrono::milliseconds ms(500);    // 500 ms
std::chrono::seconds sec(5);          // 5 秒
std::chrono::nanoseconds ns(1000000); // 100万 ナノ秒
```

### 重要な事実：異なる単位は異なる型

```cpp
std::chrono::milliseconds a(500);
std::chrono::seconds b(1);

// a と b を足すと...
auto sum = a + b;
// sum の型は？milliseconds（小さい単位に統一される）
std::cout << sum.count() << "\n";  // 1500
```

対して、直接足し算では型がぶつかります。

```cpp
// 古い規約や厳格な型チェックでは
std::chrono::milliseconds ms(500);
std::chrono::seconds s(1);
if (ms < s) { }  // 型が違うので比較できない場合もある
```

正式には、共通の単位に変換してから使うべきです。

## 12.3 単位の変換 — 暗黙的と明示的

### 暗黙的な変換：大きい単位から小さい単位へ（ロスレス）

秒からミリ秒は、ロスレスに変換できます（1秒 = 1000ミリ秒）。

```cpp
std::chrono::seconds sec(1);
std::chrono::milliseconds ms = sec;
std::cout << ms.count() << "\n";  // 1000
```

このとき、暗黙的な変換が起きます。
`seconds` の 1 を `milliseconds` の 1000 に変換するのは、
情報の損失がないため自動で行われます。

### 明示的な変換：小さい単位から大きい単位へ（ロッシー）

ミリ秒から秒は、**丸めが必要**です。

```cpp
std::chrono::milliseconds ms(500);
// 以下は自動変換されない（500ms → 0s の切り詰め）
// std::chrono::seconds sec = ms;  // エラー

// 明示的なキャストが必須
std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(ms);
std::cout << sec.count() << "\n";  // 0（切り詰め）
```

`duration_cast` は **切り詰め** です。`500ms` を秒に変換すると `0秒` になります。

必要なら、四捨五入や端数を取得します。

```cpp
// 端数を計算
std::chrono::milliseconds ms(1500);
std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(ms);
std::chrono::milliseconds rem = ms - sec;
std::cout << sec.count() << "s " << rem.count() << "ms\n";
// 出力: 1s 500ms
```

## 12.4 期間の算術演算

```cpp
using namespace std::chrono_literals;

auto a = 500ms;
auto b = 1s;
auto sum = a + b;        // 1500ms（型は milliseconds）
auto diff = b - a;       // 500ms（b を秒に変換してから diff を取る？）
auto doubled = 2 * a;    // 1000ms
```

**足し算・引き算で、型が「小さい単位」に統一されます。**

## 12.5 `steady_clock` と `system_clock`

### `system_clock` — 壁時計

```cpp
auto now = std::chrono::system_clock::now();
```

実世界の時刻です。NTP（Network Time Protocol）で同期されると、
**時刻が遡ることもあります。**
ユーザーが手動で時刻を変更することもあります。

シミュレーションでは、`system_clock::now()` は**シミュレーション開始からの経過時間ではなく、
実機動かしたときの実時刻**になります。

### `steady_clock` — 計測用時計

```cpp
auto start = std::chrono::steady_clock::now();
// ... 何か処理 ...
auto end = std::chrono::steady_clock::now();
auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "経過時間: " << elapsed.count() << " ms\n";
```

`steady_clock` は**単調増加を保証**します。
時刻が遡ることはなく、NTP で調整されることもありません。

**時間の経過を計測するには `steady_clock` を使う。**

## 12.6 ドリルで出てくる時間の使い方

### 課題 01、15：タイマー

```cpp
// solutions/01_publisher/src/minimal_publisher.cpp
using namespace std::chrono_literals;
timer_ = this->create_wall_timer(500ms, std::bind(&MinimalPublisher::timer_callback, this));
```

`create_wall_timer` の第 1 引数に `std::chrono::duration` を渡します。
ここは **500ms という `duration` を直接書きます。**

### 課題 13：タイムアウト待機

```cpp
// tools/drill_harness.hpp（ドリル実行環境）
std::future<T> future = ...;
if (future.wait_for(2s) == std::future_status::ready) {
  // 応答が返った
} else {
  // タイムアウト
}
```

`wait_for()` も `std::chrono::duration` を引数に取ります。

### テスト：スピン間隔

```cpp
// tools/drill_harness.hpp
exec.spin_all(20ms);  // 20ms 間、ノードを回す
```

`spin_all(duration)` で、その期間ノードをスピンさせます。

## 12.7 ROS 2 の時間型 — `std::chrono` とは別

### `std::chrono::duration` と `rclcpp::Duration` は別物

ROS 2 には独自の時間型があります。

```cpp
#include <rclcpp/time.hpp>

rclcpp::Duration ros_duration(5, 0);  // 5 秒 0 ナノ秒
std::chrono::seconds std_duration(5);  // 5 秒

// ros_duration と std_duration は別の型
// 直接足したりできない
```

### シミュレーション時間への対応

ROS 2 が **`use_sim_time` パラメータを有効にすると、**
ノードの時刻が「`/clock` トピックで配信されるシミュレーション時間」に従います。

- `rclcpp::Time::now()` — シミュレーション時間の現在時刻（`use_sim_time` で変わる）
- `rclcpp::Clock::steady_clock::now()` — システム時刻（変わらない）

対して `std::chrono` は**常にシステム時刻**です。

```cpp
// これはシミュレーション時間に対応しない
auto now = std::chrono::system_clock::now();

// シミュレーション時間に対応する
auto now = this->now();  // rclcpp::Node の now()
```

### `create_wall_timer` は WALL time

```cpp
timer_ = this->create_wall_timer(500ms, callback);
```

`create_wall_timer` の「wall」は「壁の時計」、つまり**実時刻**という意味です。

ロボットが物理的に動いているなら、タイマーは実時刻で回ります。
シミュレータで高速に動かしたい場合は、`create_wall_timer` ではなく
`create_timer(500ms, callback)` を使い、`/clock` トピックに対応させます。

ドリル 01〜15 は全て実時刻の想定なので、`create_wall_timer` で大丈夫です。

### `std::chrono` が必要な理由

`std::chrono` は「物理的な時間経過の計測」に使います。
シミュレーション時間に左右されない「実測値」が必要なとき。

```cpp
// ハードウェアの反応時間を計測する（シミュレーション非対応で良い）
auto start = std::chrono::steady_clock::now();
sensor.read();
auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
  std::chrono::steady_clock::now() - start);
```

ROS 2 の時間型（`rclcpp::Time` / `rclcpp::Duration`）は、
「ロボットの制御スケジュール」に使います。

```cpp
// 制御ループの同期（シミュレーション時間に対応すべき）
if (this->now() - last_update > rclcpp::Duration(0, 500000000)) {  // 500ms
  update();
  last_update = this->now();
}
```

## 手元で試す

`std::chrono::duration` の型と変換、計測を確認します。

```cpp
// chrono_time_demo.cpp
#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

int main()
{
  std::cout << "=== Duration types ===\n";
  std::chrono::milliseconds ms(500);
  std::chrono::seconds sec(1);
  std::chrono::nanoseconds ns(1000000);
  std::cout << "500ms: " << ms.count() << "\n";
  std::cout << "1s: " << sec.count() << "\n";
  std::cout << "1000000ns: " << ns.count() << "\n";

  std::cout << "\n=== Implicit conversion (seconds -> milliseconds) ===\n";
  sec = 2s;
  std::chrono::milliseconds from_sec = sec;
  std::cout << "2s -> milliseconds: " << from_sec.count() << "\n";

  std::cout << "\n=== Explicit conversion (milliseconds -> seconds) ===\n";
  ms = 500ms;
  std::chrono::seconds from_ms =
    std::chrono::duration_cast<std::chrono::seconds>(ms);
  std::cout << "500ms -> seconds: " << from_ms.count() << " (truncated to 0)\n";

  std::cout << "\n=== Arithmetic ===\n";
  auto sum = 500ms + 1s;
  std::cout << "500ms + 1s = " << sum.count() << " ms\n";
  auto doubled = 2 * 500ms;
  std::cout << "2 * 500ms = " << doubled.count() << " ms\n";

  std::cout << "\n=== Measurement with steady_clock ===\n";
  auto start = std::chrono::steady_clock::now();
  // Simulate work
  for (int i = 0; i < 50000000; ++i) {
    volatile int x = i;
    (void)x;
  }
  auto end = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Loop took " << elapsed.count() << " ms\n";

  std::cout << "\n=== User-defined literals ===\n";
  auto delay1 = 500ms;
  auto delay2 = std::chrono::milliseconds(500);
  std::cout << "500ms: " << delay1.count() << "\n";
  std::cout << "milliseconds(500): " << delay2.count() << "\n";
  std::cout << "Both are equivalent.\n";

  return 0;
}
```

**予想: 出力の型と値は何か。特に `500ms` と `duration_cast<seconds>(500ms)` の結果。**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic chrono_time_demo.cpp -o chrono_time_demo && ./chrono_time_demo
```

実測結果：

```
=== Duration types ===
500ms: 500
1s: 1
1000000ns: 1000000

=== Implicit conversion (seconds -> milliseconds) ===
2s -> milliseconds: 2000

=== Explicit conversion (milliseconds -> seconds) ===
500ms -> seconds: 0 (truncated to 0)

=== Arithmetic ===
500ms + 1s = 1500 ms

=== Measurement with steady_clock ===
Loop took 343 ms

=== User-defined literals ===
500ms: 500
milliseconds(500): 500
Both are equivalent.

```

**確認ポイント：**
1. `2s` は `2` だが、ミリ秒に変換すると `2000` になります（10 倍の）
2. `500ms` を秒に変換すると `0` に切り詰められます
3. `500ms + 1s` の結果は `1500ms`（小さい単位に統一）
4. `steady_clock` での計測は実際の時間経過を反映します

## つまずきポイント

**`500ms` が見つからない**

```cpp
auto delay = 500ms;  // エラー
```

```
error: unable to deduce 'auto' from '500' [with -funsafe-math-optimizations]
// または
error: unable to deduce template arguments for 'operator""ms'
```

`using namespace std::chrono_literals;` を追加してください。

```cpp
using namespace std::chrono_literals;
auto delay = 500ms;  // OK
```

**異なる単位を比較・演算するとエラー**

```cpp
std::chrono::milliseconds ms(500);
std::chrono::seconds sec(1);
if (ms < sec) { }   // 昔のコンパイラではエラー
```

どちらかを統一してください。

```cpp
auto ms_as_sec = std::chrono::duration_cast<std::chrono::seconds>(ms);
if (ms_as_sec < sec) { }
```

**`wait_for()` にミリ秒を整数で渡している**

```cpp
future.wait_for(500);  // エラー、または型が不明確
```

`std::chrono::duration` を渡してください。

```cpp
using namespace std::chrono_literals;
future.wait_for(500ms);

// または
future.wait_for(std::chrono::milliseconds(500));
```

**シミュレータで高速実行するとタイムアウトする**

```cpp
timer_ = this->create_wall_timer(500ms, callback);  // 常に実時刻
```

`create_wall_timer` は実時刻で動きます。
シミュレーション時間に対応させるには `create_timer` を使ってください。

```cpp
timer_ = this->create_timer(500ms, callback);  // シミュレーション時間に対応
```

ドリル 01〜15 は実時刻の想定なので、この問題は起きません。

**`system_clock::now()` が使用禁止と言われた**

```cpp
auto now = std::chrono::system_clock::now();
// ROS の設計では非推奨
```

ROS 2 のノード内では、`this->now()` を使ってください。
これは `rclcpp::Time` で、シミュレーション時間に対応します。

```cpp
rclcpp::Time now = this->now();
```

`std::chrono::steady_clock` は「実測計測」のときだけ使います。

## ドリルのどこで出るか

| 課題 | この章の何が出るか |
| --- | --- |
| 01, 15 | `using namespace std::chrono_literals;` と `500ms` のリテラル |
| 01, 15 | `create_wall_timer(500ms, ...)` |
| 13 | `future.wait_for(2s)` での時間指定 |
| 14, 15 | `auto request = std::make_shared<...>();` の戻り値を `auto` で受ける（8章） |
| テスト | `drill::spin_until(..., 5s)` での時間指定 |
| テスト | `exec.spin_all(20ms)` でのスピン間隔 |

課題 01 の publisher と 13 の future 待機がこの章の核です。

## 対応する課題

この章を読んだら、対応するドリルで手を動かしてください。

- `cpp12_chrono` — 時間を型で扱う

```bash
./drill run cpp12
```

課題側からは `./drill read` でこの章に戻ってこられます。

## 参考

- [cppreference の std::chrono](https://en.cppreference.com/w/cpp/chrono)
- [cppreference の duration_cast](https://en.cppreference.com/w/cpp/chrono/duration/duration_cast)
- [cppreference の steady_clock](https://en.cppreference.com/w/cpp/chrono/steady_clock)
- [rclcpp の Time / Duration ドキュメント](https://docs.ros.org/en/humble/Concepts/Intermediate/About-Time.html)
- [ROS 2 のシミュレーション時間](https://docs.ros.org/en/humble/Tutorials/Intermediate/Understanding-ROS2-Node-Executors.html)

---

前章 → [11. 標準ライブラリの道具箱](11_標準ライブラリの道具箱.md)
次章 → [13. 並行性の最小限](13_並行性の最小限.md)
