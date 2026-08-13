# dp02 Adapter 〔デザインパターン編〕

結城本 第2章 Adapter。**委譲版と継承版の両方**を書いて、突き合わせます。

## 題材

3 年前の先輩が書いた `LegacyMotorDriver`（生ドライバ）を、
部内共通インタフェース `MotorActuator`（単位は rad/s と rad）に合わせます。

| 役 | クラス |
| --- | --- |
| Target（合わせたい形） | `MotorActuator` |
| Adaptee（既存で変更できないもの） | `LegacyMotorDriver` |
| Adapter | `DelegatingMotorAdapter` / `InheritingMotorAdapter` |

`LegacyMotorDriver` は**書き換えられません**。仮想関数も仮想デストラクタもありません。
他の 5 つのプロジェクトが依存している、という設定です。

## やること

`src/motor_adapter.cpp` の 6 つの関数を実装してください。

1. **`DelegatingMotorAdapter`（委譲版）** — 生ドライバを**メンバとして持つ**
   - `set_velocity()` / `stop()` / `position_rad()`
2. **`InheritingMotorAdapter`（継承版）** — 生ドライバを **private 継承**する
   - 同じ 3 つ。振る舞いは委譲版と完全に一致させる

変換の規則は次のとおりです。

```
パルス指令 = std::lround(rad_per_sec * PULSES_PER_RAD_PER_SEC)   // 100 pulse per rad/s
角度 [rad] = readEncoderRaw() / COUNTS_PER_RAD                    // 200 count per rad
```

`include/drill/*.hpp` と `test/test_exercise.cpp` は編集しません。

## 動かしてみる

```bash
./drill run dp02
```

## つまずきポイント

- 継承版は private 継承なので、`driver_.setPulse(...)` ではなく **`setPulse(...)` と直接**呼びます
- `MotorActuator` の仮想デストラクタを消すと、`unique_ptr<MotorActuator>` で解放したとき未定義動作になります
- パルスは `int`。`static_cast<int>` の切り捨てではなく `std::lround` で丸めます
- 50.0 rad/s は 5000 パルス相当ですが、ドライバ側が 1000 に丸めます。**Adapter 側では丸めません**

## 考えること（テストは見ませんが、記事の本題です）

- 継承版で `private` を `public` に変えると何が壊れるか
- `LegacyMotorDriver` にも仮想関数があって、両方を継承したくなったらどうなるか
- `std::stack` は `std::deque` のどちら版の Adapter か

## 参考

- [2. Adapter](../../docs/patterns/02_Adapter.md)
- [継承](../../docs/cpp/03_継承.md)
- [スマートポインタ](../../docs/cpp/06_スマートポインタ.md)
