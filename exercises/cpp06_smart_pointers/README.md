# c06 スマートポインタで所有権を表す 〔C++編〕

weak_ptr と Registry パターンを学びます。

## やること

`src/registry.cpp` に Registry クラスを実装してください。

Registry は Item への参照を管理するクラスです。
weak_ptr を使い、外部で保持されている Item だけを fire() で処理します。

実装する関数：
- `add()`: Item を weak_ptr に変換して登録
- `fire()`: 登録された Item をイテレート、生きているものだけ出力

## 動かしてみる

```bash
./drill run c06
```

## つまずきポイント

- `std::weak_ptr` は参照カウント（shared_ptr）を増やしません。
- `expired()` でオブジェクトが生きているかをチェックできます。
- `lock()` で weak_ptr を shared_ptr に変換します。

## テスト

```bash
./drill run c06
```

| テスト | 見ているところ |
| --- | --- |
| `生きているItemだけが出力される` | weak_ptr の expired() チェック |

## 参考

- [cppreference: weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)
- [cppreference: shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr)
- [6. スマートポインタで所有権を表す](../../docs/cpp/06_スマートポインタ.md)
