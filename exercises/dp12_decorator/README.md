# dp12 Decorator 〔デザインパターン編〕

結城本 第12章。ログの整形を「素のメッセージ」に**包んで**足していきます。
Border/Display にあたるのが `SinkDecorator` / `LogSink` です。

C++ の本題は 1 つ、**所有権**です。デコレータは中身を `std::unique_ptr<LogSink>` で
所有します。コンストラクタは値で受け取り、`std::move` でメンバに入れます。

## やること

`src/log_sink.cpp` に実装してください。

1. **`join_tag()`**
   - タグと本文を半角スペース 1 つでつなぐ
   - `unique_ptr` 版とテンプレート版の**両方**から呼ばれます。ここが一致の根拠です

2. **`PlainMessage`**
   - `format()` は何も足さずにそのまま返す
   - デストラクタで `DestructionLog::record("PlainMessage")`

3. **`SinkDecorator`**
   - コンストラクタ: 受け取った `inner` を `std::move` でメンバへ
   - `inner()`: 所有している中身への**参照**を返す

4. **`LevelTag` / `TimestampTag` / `SourceTag`**
   - `format()` は「先に中身へ整形させ、その結果にタグを前置」
   - デストラクタでそれぞれ自分の名前を記録

5. **`plain()` / `with_level()` / `with_timestamp()` / `with_source()`**
   - 組み立てヘルパ。`std::make_unique` を三重に書く代わりになります

## 動かしてみる

```bash
./drill run dp12
```

## つまずきポイント

- `SinkDecorator(std::unique_ptr<LogSink> inner) : inner_(inner)` はコンパイルエラーです。
  `unique_ptr` はコピーできません。**`std::move(inner)`** と書きます
- `inner()` は `const LogSink &` を返します。`LogSink` は抽象クラスなので値では返せません
- `format()` の中で**先に中身を呼ぶ**こと。順番を間違えるとタグの位置が入れ替わります
- 包む順番で出力が変わるのが Decorator です。テストがそれを見ています
- デストラクタの記録を忘れると「一番外側を破棄すると内側まで全部破棄される」が落ちます

## テスト

10 個あります。包む順番で出力が変わること、何重にも包めること、
外側 1 個を捨てれば内側まで解放されること、
テンプレート版が同じ出力かつ `std::is_polymorphic_v` が `false` であることまで見ます。

## 参考

- [12. Decorator](../../docs/patterns/12_Decorator.md)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
- [cppreference: std::move](https://en.cppreference.com/w/cpp/utility/move)
