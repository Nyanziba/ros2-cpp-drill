# dp13 Visitor 〔デザインパターン編〕

結城本 第13章。起動時セルフチェックの結果ツリーに対して、
**GoF 版の二重ディスパッチ**と **`std::variant` + `std::visit`** の 2 通りを実装し、
同じ結果が出ることを確かめます。

扱う木はこれです。

```
[selftest]
  bat 11800mV OK
  [drive]
    motor_l fault=0 OK
    motor_r fault=3 NG
  temp 65000mV NG
```

## やること

`src/diagnostics.cpp` に実装します。ヘッダは編集しません。

### その1: GoF 版（二重ディスパッチ）

1. **`SensorCheck::accept` / `MotorCheck::accept` / `CheckGroup::accept`**
   - どれも中身は `visitor.visit(*this);` の 1 行
   - **3 つとも字面は同じですが、基底クラスに 1 個書いてまとめることはできません。**
     基底の中では `*this` の静的型が `DiagNode` になり、それを取る `visit` が無いからです
2. **`CheckGroup::add`** — `children_` に `std::move` で入れる（所有権はグループが持つ）
3. **`FailureCountVisitor::visit`（3 つ）** — 葉で数を数え、グループでは子を `accept` する
4. **`TextReportVisitor::visit`（3 つ）** — グループで `depth_` を増減しながら行を足す

**木をたどるのは訪問者側の仕事**です（結城本の `ListVisitor` と同じ）。
要素側に走査を書くと、訪問順を変えたい訪問者が書けなくなります。

### その2: `std::variant` 版（継承なし・仮想関数なし・`accept` なし）

5. **`DiagArena::add`** — 末尾に入れて添字を返す
6. **`overloaded` イディオム** — 無名名前空間に自分で書きます

   ```cpp
   template <class ... Ts>
   struct overloaded : Ts ...
   {
     using Ts::operator() ...;
   };

   template <class ... Ts>
   overloaded(Ts ...) -> overloaded<Ts ...>;
   ```

   C++17 では**推論ガイド（下 2 行）を自分で書く必要があります**。C++20 では不要です
7. **`count_failures` / `make_report`** — `std::visit` で種類ごとに分岐。
   グループでは子の添字をたどって再帰します。
   **GoF 版と 1 文字も違わない文字列**を返してください

出力の形（インデントと行の書式）は `include/drill/diagnostics.hpp` の
`diag_format` に用意してあります。両方の版でこれを使えば文字列はずれません。

## 動かしてみる

```bash
./drill run dp13
```

## つまずきポイント

- `accept` を実装していないうちは、`variant` 版のテストが
  `C++ exception with description "vector"` で落ちます。
  `DiagArena::add` が何も保存していないので `at()` が範囲外になっているだけです
- `accept` が仮想でないと、基底ポインタ経由の呼び出しで種類が消えます。
  テスト「基底ポインタ経由でも派生ごとのvisitが選ばれる」がそこを見ます
- `visit` の引数を `const SensorCheck &` ではなく `SensorCheck` にすると**コピー**が走ります。
  基底型で受けると**スライシング**します
- `overloaded` の推論ガイドを忘れると
  `no viable constructor or deduction guide for deduction of template arguments` になります
- ラムダの中から自分を再帰呼び出しはできません。
  無名名前空間に名前付きの関数を用意して、その中で `std::visit` してください
- `std::visit` に渡すラムダが 1 つでも足りないと**コンパイルエラー**になります。
  これは事故ではなく、**種類を増やしたときに直す場所を教えてくれている**ということです

## テスト

```bash
./drill run dp13
```

11 個のテストがあります。二重ディスパッチが効いているか、
2 種類の Visitor が同じ木に当てられるか、`variant` 版が GoF 版と一致するか、
`variant` 版が多態でないか（`static_assert`）まで見ます。

## 参考

- [13. Visitor](../../docs/patterns/13_Visitor.md)
- [cppreference: std::visit](https://en.cppreference.com/w/cpp/utility/variant/visit)
- [cppreference: std::variant](https://en.cppreference.com/w/cpp/utility/variant)
