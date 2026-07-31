# c03 継承して仮想関数を実装する 〔C++編〕

継承と仮想関数（ポリモーフィズム）を学びます。

## やること

`src/sensor.cpp` に 2 つのセンサクラスを実装してください：

1. **TemperatureSensor** — 温度センサ
   - `read()` は override で **25.0** を返す
   - `label()` は override しない（基底クラスのデフォルト "Sensor" を使用）

2. **HumiditySensor** — 湿度センサ
   - `read()` は override で **60.0** を返す
   - `label()` は override して **"HumiditySensor"** を返す

## 動かしてみる

```bash
./drill run c03
```

## つまずきポイント

- **純粋仮想関数** `= 0` は、必ず派生クラスで override しなければいけません。
- **仮想関数** は `virtual` キーワードで、派生クラスでは `override` キーワードを使います。
- C++11 以降の `override` キーワードは、うっかり綴りを間違えたときに検出してくれるので便利です。
- ポリモーフィズムで値が正しく取得できるか、テストで確認しましょう。

## テスト

```bash
./drill run c03
```

| テスト | 見ているところ |
| --- | --- |
| `TemperatureSensorが正しい値を返す` | TemperatureSensor の read() 実装 |
| `HumiditySensorが正しい値を返す` | HumiditySensor の read() 実装 |
| `TemperatureSensorはデフォルトのlabelを使う` | デフォルト実装の継承 |
| `HumiditySensorはlabelをoverrideしている` | override の実装 |
| `ポリモーフィズムで正しくディスパッチされる` | 基底クラスポインタでの動的ディスパッチ |

## 参考

- [cppreference: Virtual function](https://en.cppreference.com/w/cpp/language/virtual)
- [cppreference: override specifier](https://en.cppreference.com/w/cpp/language/override)
- [3. 継承して仮想関数を実装する](../../docs/cpp/03_継承.md)
