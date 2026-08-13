# dp06 Prototype 〔デザインパターン編〕

結城本 第6章。**モータに流す波形パターンのひな型**を複製します。

先に言っておきます。**C++ にはコピーコンストラクタがあります。**
`PulseTrain b = a;` で複製できるなら `clone()` は要りません。
`clone()` が本当に要るのは、**`std::unique_ptr<Waveform>` しか持っていない状態で、
実体が何の型か知らないまま複製したいとき**だけです
（[6. Prototype](../../docs/patterns/06_Prototype.md) の 6.1）。

## やること

`src/waveform.cpp` に 5 か所実装してください。

1. **`Waveform::clone()`**
   - `do_clone()` の戻り値を `std::unique_ptr<Waveform>` に包んで返す
   - `std::make_unique` は使えません。実体の型が分からないからです

2. **`PulseTrain` のコピーコンストラクタ**
   - `other.pattern_` の中身を 1 つずつ写す（深いコピー）
   - 配列の確保は書いてあります。**中身を写す**のが仕事です

3. **`PulseTrain::do_clone()`**
   - 自分の複製を `new` して返す。中身はコピーコンストラクタに任せる（1 行）
   - 戻り値の型が `PulseTrain *` なのが**共変戻り値型**です

4. **`SineSweep::do_clone()`**
   - 同じ。`SineSweep` は値メンバしか持たないので、コピーコンストラクタは書きません（Rule of Zero）

5. **`WaveformLibrary::duplicate()`**
   - 全要素を `clone()` して新しいライブラリに詰める
   - **要素の実体の型を一切知らないまま**複製できることを確かめてください

`include/drill/waveform.hpp` と `test/test_exercise.cpp` は編集しません。

## 動かしてみる

```bash
./drill run dp06
```

## つまずきポイント

- `std::unique_ptr<PulseTrain> do_clone()` と書きたくなりますが、**通りません**。
  `std::unique_ptr<Base>` → `std::unique_ptr<Derived>` は共変戻り値型になれません。
  だから `do_clone()` は生ポインタを返し、`clone()` が包みます
- `clone()` は **`override` しないでください**。非仮想です。
  override すべきは private の `do_clone()` の方です
- `PulseTrain` のコピーコンストラクタで `Waveform(other)` を忘れないでください。
  忘れると基底部分がデフォルト構築されます（この課題では見えませんが、
  基底が状態を持つ設計だとバグになります）
- `pattern_` は `std::unique_ptr<double[]>` なので、**浅いコピーを書こうとしても
  コンパイルが通りません**。生ポインタメンバだと通ってしまい、二重解放で落ちます
- `WaveformLibrary` のコピーが `= delete` してあるのは、
  「`std::vector<std::unique_ptr<...>>` を持てば自動的にコピー禁止になる」が
  **嘘だから**です。ヘッダのコメントを読んでください

## テスト

```bash
./drill run dp06
```

8 つのテストがあります。

| テスト | 見ているもの |
| --- | --- |
| `cloneは元とは別のオブジェクトを返す` | アドレスが違うか |
| `unique_ptr経由でも派生の型が保たれる` | `dynamic_cast` で実体の型 |
| `SineSweepもcloneで複製できる` | もう一方の派生でも同じか |
| `cloneした波形は深いコピーになっている` | 元を書き換えても複製が変わらないか |
| `PulseTrainのコピーコンストラクタが深いコピーを作る` | `clone()` を通さない素のコピー |
| `複製はバッファを共有しない` | `data()` のアドレス比較 |
| `duplicateは要素数と型を保つ` / `duplicateした要素は元と共有されない` | ライブラリ丸ごとの複製 |

`static_assert` で、コピーが禁止されているべきクラス（`Waveform` への代入、
`WaveformLibrary` のコピー）が実際に禁止されていることも見ています。

## 参考

- [6. Prototype](../../docs/patterns/06_Prototype.md)
- [C++編 5. ムーブと所有権](../../docs/cpp/05_ムーブと所有権.md)
- [C++編 6. スマートポインタ](../../docs/cpp/06_スマートポインタ.md)
- [cppreference: Copy constructors](https://en.cppreference.com/w/cpp/language/copy_constructor)
