# dp10 Strategy 〔デザインパターン編〕

結城本 第10章。**同じアルゴリズムを C++ の 3 通りの手段で実装**して、
結果が一致することと、コストが違うことを確かめます。

題材は速度指令のフィルタです。上位から降ってくる生の指令 `raw` を、
前回出した値 `previous` と突き合わせて丸めてからモータに渡します。

| 名前 | 計算 |
| --- | --- |
| clamp | `raw` を `[-max_abs, +max_abs]` に収める（`previous` は使わない） |
| slew rate | `raw` を `[previous - max_delta, previous + max_delta]` に収める |

**前回値は Strategy ではなく Context（Commander）が持ちます。**
Strategy を状態なしにしておくと `const` にでき、`static` な実体を共有できます。
マイコンで効く判断です。

## やること

`src/velocity_filter.cpp` を実装してください。3 つのブロックに分かれています。

### 1) 仮想関数版

- `ClampFilter::apply` / `SlewRateFilter::apply`
  - 使わない引数は**名前を書かない**でください（`-Wunused-parameter` が出ます）
  - `std::clamp` が `<algorithm>` にあります
- `VirtualCommander` のコンストラクタ / `set_filter` / `filter` / `update`
  - Strategy は**コピーせず、アドレスで指す**こと。所有はしません
  - コピーするとスライシングして、派生の `apply()` が消えます

### 2) `std::function` 版

- `FunctionCommander` のコンストラクタ / `set_filter` / `update`
  - `std::move` で受け取ってください
- `make_clamp_fn` — `ClampFilter::apply` と同じ計算をするラムダを返す
  - `max_abs` をキャプチャします。キャプチャがあるので関数ポインタには変換できません

### 3) テンプレート版

- `ClampPolicy::apply` / `SlewRatePolicy::apply`
  - 中身は仮想関数版と 1 文字も変わりません。**`virtual` を書かないこと**が仕事です
  - `StaticCommander` はヘッダに書いてあります（テンプレートなので定義がヘッダに要る）

## 動かしてみる

```bash
./drill run dp10
```

## つまずきポイント

- **3 つとも同じ出力**でなければいけません。手段が違うだけで、アルゴリズムは同じものです
- `VirtualCommander` が Strategy をコピーして持つと、テスト
  「仮想関数版はStrategyを所有せず参照で指している」がアドレス比較で落とします
- `filter_` を `const VelocityFilter &`（参照）にしたくなりますが、
  **参照は再束縛できない**ので差し替えができません。だからポインタです
- `VirtualCommander` は Strategy を**所有しません**。
  `ClampFilter` の実体を Commander より先に殺すと未定義動作です
- `ClampPolicy` に `virtual` を付けると `static_assert` で落ちます。
  vtable が付いて `sizeof` も増えます

## テスト

```bash
./drill run dp10
```

9 つのテストがあります。3 方式の一致、実行時の差し替え、
Strategy を所有していないこと（アドレス比較）、
テンプレート版が多態でないこと（`std::is_polymorphic_v` と `sizeof`）まで見ます。

## 参考

- [10. Strategy](../../docs/patterns/10_Strategy.md)
- [0. 使う前に](../../docs/patterns/00_使う前に.md) — 実装が 1 つなら Strategy は入れません
- [cppreference: std::function](https://en.cppreference.com/w/cpp/utility/functional/function)
- [cppreference: std::clamp](https://en.cppreference.com/w/cpp/algorithm/clamp)
- [cppreference: std::is_polymorphic](https://en.cppreference.com/w/cpp/types/is_polymorphic)
