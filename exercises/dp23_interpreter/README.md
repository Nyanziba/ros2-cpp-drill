# dp23 Interpreter 〔デザインパターン編〕

結城本 第23章。動作記述ミニ言語 **MotionScript** を構文木に落として、動作列に展開します。

```
forward 100; turn 90; repeat 3 { forward 50; }
```

↓

```
forward 100, turn 90, forward 50, forward 50, forward 50
```

## 文法（これで全部）

```
program   := statement*
statement := "forward" NUMBER ";"
           | "turn"    NUMBER ";"
           | "repeat"  NUMBER "{" program "}"
```

## やること

`src/motion_script.cpp` に 5 つ実装してください。字句解析（`tokenize`）は実装済みです。

1. **`CommandNode::evaluate()`**
   - 自分が持つ `Motion` を 1 つ `out` に積む

2. **`SequenceNode::evaluate()`**
   - 子を先頭から順に `evaluate()`

3. **`RepeatNode::evaluate()`**
   - 本体を `count_` 回 `evaluate()`

4. **`parse()`** — 再帰下降パーサ
   - `parse_sequence(depth)` / `parse_statement(depth)` に分けると素直です
   - **例外を投げないこと。** `ParseResult::failure(ParseError{...})` を返します
   - `parse_sequence` の入口で `depth > kMaxNestingDepth` を弾くこと

5. **`run_variant()`** — `std::variant` + `std::visit` 版の評価
   - クラス版の `run()` と同じ結果になること

## 動かしてみる

```bash
./drill run dp23
```

## つまずきポイント

- **`evaluate()` は結果を返さず `out` に積みます。** ノードごとに `std::vector` を作って
  結合すると、木の深さぶん確保が走ります
- 失敗を上に伝える方法。`throw` を使わないなら、
  **「エラーはメンバに置き、関数は `nullptr` を返す」**のが定番です
- `parse_sequence` は `}` と入力終端の**両方**で止まります。
  止め忘れると `repeat` の本体が外まで食い込みます
- 深さ制限は**再帰の入口**で見ます。出口や後始末で見ても、
  そこに着く前にスタックが尽きます
- `variant_ast::VNode` は `std::variant<Command, std::unique_ptr<Repeat>>` です。
  **再帰的な型なので variant にしても間接参照は消えません**。
  variant の要素は完全型でなければならないため、`Repeat` は `unique_ptr` で挟んであります
- `std::visit` に渡す呼び出し可能物は、**すべての候補型に対する `operator()`** を
  持っている必要があります。1 つ足りないとテンプレートのエラーが数十行出ます

## テスト

```bash
./drill run dp23
```

13 個のテストがあります。入れ子の展開、構文エラーがエラー値で返ること、
`std::variant` 版が同じ結果になること、深すぎる入れ子で落ちないことまで見ます。

## 参考

- [23. Interpreter](../../docs/patterns/23_Interpreter.md)
- [cppreference: std::variant](https://en.cppreference.com/w/cpp/utility/variant)
- [cppreference: std::visit](https://en.cppreference.com/w/cpp/utility/variant/visit)
