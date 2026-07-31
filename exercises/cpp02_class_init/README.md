# c02 クラスを初期化する 〔C++編〕

クラスのコンストラクタと const メンバを学びます。

## やること

`src/stopwatch.cpp` のコンストラクタを完成させてください。

メンバ初期化リストを使って `max_time_` と `elapsed_` を初期化します：

```cpp
Stopwatch::Stopwatch(int max_time_ms)
  : max_time_(??), elapsed_(??)
{
}
```

また、`advance()`、`elapsed()`、`max_time()` メンバ関数も実装してください。

## 動かしてみる

```bash
./drill run c02
```

## つまずきポイント

- **const メンバ変数** は、コンストラクタ内の通常の代入では初期化できません。
  必ず **メンバ初期化リスト** を使って初期化します。
- メンバ初期化リストは `: 変数(値), ...` という形式です。セミコロンではなくカンマで区切ります。
- const メンバ関数 `const` キーワードはこう書きます: `int elapsed() const { ... }`
  中で メンバ変数を変更すると、コンパイルエラーになります。

## テスト

```bash
./drill run c02
```

| テスト | 見ているところ |
| --- | --- |
| `コンストラクタでmaxTimeが設定される` | メンバ初期化リストの正確性 |
| `初期状態では経過時間は0` | elapsed_ の初期化 |
| `advanceで経過時間が増える` | advance() メソッドの実装 |
| `constメンバ関数で値を取得できる` | const メンバ関数の構文 |

## 参考

- [cppreference: Initializer list](https://en.cppreference.com/w/cpp/language/initializer_list)
- [cppreference: const (qualifier)](https://en.cppreference.com/w/cpp/language/const)
- [2. クラスを初期化する](../../docs/cpp/02_クラスと初期化.md)
