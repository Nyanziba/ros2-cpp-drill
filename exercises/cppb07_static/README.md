# cppb07 static の3つの意味 〔C++入門編〕

static の3つの文脈を学びます。

## やること

`src/counter.cpp` に次の 3 つを実装してください。

1. 関数内 static：初回のみ初期化される変数
2. クラス static メンバ：全インスタンス間で共有される変数（ファイルスコープで定義が必須）
3. クラス static メンバ関数：this がない関数

## 動かしてみる

```bash
./drill run cppb07
```

## つまずきポイント

- 関数内 static は初回のみ初期化、以後も存続します。
- クラス static メンバは `.cpp` ファイルで必ず定義が必要です。
- 関数内 static とクラス static メンバは別物です。

## テスト

```bash
./drill run cppb07
```

| テスト | 見ているところ |
| --- | --- |
| `関数内Staticは値を保持` | 関数内 static |
| `クラスStaticメンバは共有される` | クラス static メンバと定義 |

## 参考

- [7. static](../../docs/cpp-basics/07_static.md)
