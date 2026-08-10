# dp21 Proxy 〔デザインパターン編〕

結城本 第21章。`PrinterProxy` の遅延生成を C++ で書きます。
C++ 版の Proxy は継承ではなく **`operator->` を書く**のが本体です。

## やること

`src/calibration_proxy.cpp` に 5 つ実装してください。
本体側（`CalibrationTable` / `RegisterFile`）は実装済みです。

1. **`CalibrationProxy::operator->()`**
   - `real_` が空なら `std::make_unique<CalibrationTable>(source_)` で作り、`real_.get()` を返す
   - `const` メンバ関数です。`real_` が `mutable` なので中で書き換えられます

2. **`RegisterAccess::RegisterAccess()`** — `owner_.record("enter");`

3. **`RegisterAccess::~RegisterAccess()`** — `owner_.record("leave");`

4. **`RegisterAccess::operator->()`** — `return &owner_.file_;`
   - ここでポインタが返るので `operator->` の連鎖が止まります

5. **`SafeRegisterProxy::read()` / `write()`** — 範囲検査と記録

### 記録（ログ）の形式

テストが文字列を完全一致で見ます。

| 場面 | 記録する文字列 |
| --- | --- |
| `RegisterAccess` の生成 | `enter` |
| `RegisterAccess` の破棄 | `leave` |
| 範囲内の読み出し | `read:1` |
| 範囲内の書き込み | `write:1=255` |
| 範囲外の読み出し | `reject:read:99` |
| 範囲外の書き込み | `reject:write:4` |

## 動かしてみる

```bash
./drill run dp21
```

## つまずきポイント

- `error: no viable overloaded '='` / `but method is not marked const`
  → `operator->` は `const` です。ヘッダの `mutable std::unique_ptr<...> real_;` を見てください
- 「最初のアクセスまで本体は作られない」が落ちる
  → `CalibrationProxy` のコンストラクタで本体を作ってしまっています
- 「二度目以降のアクセスで本体は作り直されない」が落ちる
  → `if (!real_)` の判定を忘れて毎回 `make_unique` しています
- `proxy->write_raw(...)` が動かない
  → `RegisterAccess::operator->()` が `nullptr` を返しています。
    `SafeRegisterProxy` の `friend` なので `owner_.file_` に直接届きます
- 「弾いたのに本体に触っています」で落ちる
  → 範囲外の分岐で `file_.read_raw()` / `file_.write_raw()` を呼んでいます。
    実機ではここが別のペリフェラルの破壊になります
- `enter` / `leave` の数が合わない
  → `read()` / `write()` の中で `operator->` を使っています。
    検査つきの経路は一時オブジェクトを通しません

## テスト

```bash
./drill run dp21
```

9 つのテストがあります。遅延生成が起きる回数、`const` な Proxy からの遅延生成、
範囲外が本体に届かないこと、`operator->` の連鎖と一時オブジェクトの寿命まで見ます。

## 参考

- [21. Proxy](../../docs/patterns/21_Proxy.md)
- [cppreference: operator->](https://en.cppreference.com/w/cpp/language/operator_member_access)
- [cppreference: mutable](https://en.cppreference.com/w/cpp/language/cv)
- [cppreference: copy elision](https://en.cppreference.com/w/cpp/language/copy_elision)
