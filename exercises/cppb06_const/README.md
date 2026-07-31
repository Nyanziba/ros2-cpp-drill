# cppb06 const を宣言と定義の両方で合わせる 〔C++入門編〕

`const` の 4 つの位置を、ヘッダと `.cpp` の対応という形で確かめる課題です。

## ヘッダに 4 つの const が既に入っています

`include/drill/config.hpp` は**編集しません**。まず読んでください。

```cpp
class Config
{
public:
  Config(const int & limit);      // ① 引数を読み取り専用で受け取る
  int get_limit() const;          // ② このメンバ関数はオブジェクトを変更しない
  const int * ptr_to_limit() const;  // ③ 戻り値の指す先が読み取り専用  ②と同じ末尾 const
private:
  const int limit_;               // ④ メンバ自体が変更不可
};
```

**同じ `const` が 4 か所にあって、意味が全部違います。** これがこの章の主題です。

| 位置 | 何が const になるか |
| --- | --- |
| ① 引数 `const int &` | 呼び出し元の値を書き換えない |
| ② 末尾 `const` | `this` が指すオブジェクトを書き換えない |
| ③ 戻り値 `const int *` | 返したポインタの**指す先**を書き換えられない |
| ④ メンバ `const int` | 初期化のあと代入できない（初期化子リストが必須になる） |

## やること

編集するのは `src/config.cpp` だけです。
いま定義側に末尾 `const` が付いていないので、**宣言と定義が別の関数として扱われます。**

```
error: no declaration matches ‘int Config::get_limit()’
```

これを消してください。付け足す `const` は **2 か所**（`get_limit` と `ptr_to_limit`）です。
①と④はヘッダ側の話なので、こちらで書くことはありません。

## 動かしてみる

```bash
./drill run cppb06
```

**未着手のうちはビルドが通りません。** 末尾 `const` は関数の型の一部なので、
付け忘れは「テストが赤くなる」ではなく「宣言と一致しない」という形で出ます。

## つまずきポイント

- 末尾 `const` は**関数の型の一部**です。だから宣言と定義の両方に必要です。
  片方だけに付けると別の関数を定義したことになり、上のエラーになります
- `no declaration matches` が出たら、まずヘッダの宣言と 1 文字ずつ見比べてください
- `const int * f()` と `int * f() const` は別物です。前者は戻り値、後者は `this` の話です

## テスト

| テスト | 見ているところ |
| --- | --- |
| `ConstCorrect` | `const Config` から `get_limit()` が呼べるか（② の末尾 const） |
| `戻り値のポインタはconst` | 戻り値の型が `const int *` か（③） |

## 参考

- [6. const](../../docs/cpp-basics/06_const.md)
