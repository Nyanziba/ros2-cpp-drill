# dp09 Bridge 〔デザインパターン編〕

結城本 第9章。「機能のクラス階層」と「実装のクラス階層」を分けます。
そのうえで、**同じ構造で目的だけが違う Pimpl** を書きます。

## やること

### 1. `src/telemetry_view.cpp` — Bridge

実装のクラス階層（どこへ出すか）。

1. **`RecordingSink::open()` / `put_line()` / `close()`**
   - `open()` は `"<open>"`、`close()` は `"<close>"` を `log_` に積む
   - `put_line()` は受け取った行をそのまま積む

2. **`NumberedSink::open()` / `put_line()` / `close()`**
   - `put_line()` は `"0: v=12.4"` のように 0 始まりの通し番号を付ける
   - `open()` で番号を 0 に戻す

機能のクラス階層（何を出すか）。

3. **`TelemetryView::show()`**
   - `open()` → `put_line(text)` → `close()` の順で `sink()` を呼ぶ

4. **`RepeatView::show_repeat()`**
   - `open()` は 1 回、`put_line(text)` を `times_` 回、`close()` は 1 回
   - `times_` が 0 以下でも `open()` と `close()` は呼ぶ

### 2. `src/link_stats.cpp` — Pimpl

5. **`struct LinkStats::Impl`** — サンプルを保持するメンバを持たせる
6. **`LinkStats::LinkStats()`** — `impl_` を `std::make_unique<Impl>()` で作る
7. **`add_sample()` / `count()` / `mean()` / `max()`** — サンプルが 0 個なら `mean()` も `max()` も `0.0`

`~LinkStats()` とムーブの 3 つは**すでに書いてあります。消さないでください。**
消してヘッダ側に `= default` を書くと、不完全型のせいでコンパイルが通りません。
理由は記事の 9.4 にあります。

## 動かしてみる

```bash
./drill run dp09
```

## つまずきポイント

- `TelemetryView` は `TelemetrySink` を**継承していません**。メンバとして 1 本持っているだけです。
  継承にすると、機能 M 個 × 実装 N 個で M×N クラス要るようになります
- `show()` の中に「どこへ出るか」を書いてしまったら Bridge が壊れています。
  `sink()` を呼ぶ以外のことをしないでください
- `LinkStats` のデストラクタをヘッダで `= default` にすると
  `error: invalid application of 'sizeof' to an incomplete type 'LinkStats::Impl'` になります
- `count()` を `impl_->...` で書く前に、コンストラクタで `impl_` を作ってください。
  作り忘れると nullptr 参照です

## テスト

```bash
./drill run dp09
```

9 つのテストがあります。機能 2 種 × 実装 2 種の 4 通りが全部動くこと、
実装を差し替えても機能側の関数が同じであること、
Pimpl 版のサイズがポインタ 1 個分であること（`static_assert`）まで見ます。

## 参考

- [9. Bridge](../../docs/patterns/09_Bridge.md)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
- [cppreference: PImpl](https://en.cppreference.com/w/cpp/language/pimpl)
