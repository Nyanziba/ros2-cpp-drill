# dp11 Composite 〔デザインパターン編〕

結城本 第11章。ロボットの起動時自己診断を、**個別の診断（葉）とグループ（節）を同一視する**木で表します。
`File` / `Directory` / `Entry` が `DiagnosticCheck` / `DiagnosticGroup` / `DiagnosticEntry` に対応します。

この課題の主題は所有権です。子は
`std::vector<std::unique_ptr<DiagnosticEntry>>` で持ちます。
**`delete` は 1 行も書きません。**

## やること

`src/diagnostic_tree.cpp` に 7 つ実装してください。デストラクタは実装済みです。

1. **`DiagnosticCheck::check_count()`**
   - 葉は「実際に走る診断」1 個です

2. **`DiagnosticCheck::run()`**
   - `passes_` が `true` なら `passed = 1`、`false` なら `failed = 1`

3. **`DiagnosticCheck::collect_names()`**
   - `prefix + "/" + name()` を `out` に積む

4. **`DiagnosticGroup::add()`**
   - `children_.push_back(std::move(child))`
   - `child` は**名前付きの変数**なので、`std::move` を忘れると
     「コピーコンストラクタは削除されています」とコンパイルエラーになります
   - `child` が `nullptr` のときは何もしない

5. **`DiagnosticGroup::check_count()`**
   - 子の `check_count()` の合計。グループ自身は数えません

6. **`DiagnosticGroup::run()`**
   - 子の `run()` を登録順に呼び、`passed` / `failed` を合計する

7. **`DiagnosticGroup::collect_names()`**
   - 自分のフルパスを積んでから、それを `prefix` にして子に再帰する

葉かグループかを `dynamic_cast` で判定する必要はありません。**判定しないのが Composite です。**

## 動かしてみる

```bash
./drill run dp11
```

## つまずきポイント

- `error: call to implicitly-deleted copy constructor of 'std::unique_ptr<...>'` が出たら、
  `push_back` に `std::move` が足りていません。`unique_ptr` はコピーできません
- `add(std::move(group))` したあとの `group` は `nullptr` です。二度と使えません
- 子を `std::vector<DiagnosticEntry>`（値）で持ちたくなっても、できません。
  抽象クラスなので `vector` に入れられず、入れられたとしても
  **スライシング**して `DiagnosticCheck` の部分が消えます
- `DiagnosticGroup` は `unique_ptr` のメンバを持つので**コピーできません**。
  テスト先頭の `static_assert` がこれを見ています
- 親へのポインタは持たせていません。`shared_ptr` で親子を相互参照すると
  **循環して永久に解放されません**（記事 11.5 に実測があります）

## テスト

```bash
./drill run dp11
```

9 つのテストがあります。再帰的な集計だけでなく、
**親を破棄すると子も破棄されること**をデストラクタのログで確認し、
**コピー不可・ムーブ可**を `static_assert` で確認します。

## 参考

- [11. Composite](../../docs/patterns/11_Composite.md)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
- [cppreference: std::move](https://en.cppreference.com/w/cpp/utility/move)
- [cppreference: object slicing](https://en.cppreference.com/w/cpp/language/derived_class)
