# dp03 Template Method 〔デザインパターン編〕

結城本 第3章 Template Method を、C++ の **NVI (Non-Virtual Interface)** で書きます。

センサ読み取りの手順は決まっています。

```
初期化（初回だけ） → 生値の取得 → 物理量への変換 → 検証
```

この**順番だけ**を基底クラス `SensorReader` が持ち、各段の中身を派生クラスが埋めます。

## やること

`src/sensor_reader.cpp` を実装してください。`include/drill/sensor_reader.hpp` は編集しません。

1. **`SensorReader::read_once()`** — テンプレートメソッド（手順の骨格）
   - 上の 4 段をこの順で呼ぶ
   - 各段を呼ぶ直前に `record("段の名前")` を呼ぶ
   - 初期化は初回だけ
   - 検証に落ちたら `std::nullopt`

2. **`SensorReader::validate()`** — 既定の検証（有限値かどうか）

3. **`EncoderReader`** — カウント値 → 角度[deg]

4. **`ThermistorReader`** — AD 値 → 温度[degC]、範囲外を弾く
   - `validate()` は `SensorReader::validate()` を呼んでから範囲を見る

## 動かしてみる

```bash
./drill run dp03
```

## つまずきポイント

- **`read_once()` に `virtual` を付けない。** 骨格は差し替えさせない。Java の `final` メソッドに当たる
- 各段の仮想関数は `private`。**private でもオーバーライドはできる**（呼び出しだけができない）
- 派生から基底版を呼びたい `validate()` だけ `protected` にしてある
- `override` は必ず書く。`const` を 1 つ落とすと黙って別の関数になる
- **コンストラクタから `read_once()` を呼んではいけない**（派生の実装がまだ動かない）

## テスト

`call_log()` を見て、**手順の順番が守られているか**を検査します。
値が合っていても順番が違えば落ちます。

## 参考

- [3. Template Method](../../docs/patterns/03_TemplateMethod.md)
- [C++編 3. 継承](../../docs/cpp/03_継承.md)
