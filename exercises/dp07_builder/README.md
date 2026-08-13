# dp07 Builder 〔デザインパターン編〕

結城本 第7章。**「Builder」と呼ばれるものは 2 つある**ので、両方を実装します。
名前が同じだけで、解いている問題が違います。混ぜないでください。

| | 結城本の Builder | 実務の Builder |
| --- | --- | --- |
| 解く問題 | 同じ手順から違う表現を作る | C++ に名前付き引数が無い |
| 登場人物 | Director / Builder / ConcreteBuilder | Builder 1 つだけ |
| ファイル | `src/telemetry_builder.cpp` | `src/motor_config.cpp` |

## やること

### A. 結城本の形 — `src/telemetry_builder.cpp`

1. **`TelemetryDirector::construct()`**
   - 決められた順で `builder_` を 5 回呼ぶ
   - **カンマも波かっこも一切書かない**のが Director の仕事です。
     書式を知っているのは ConcreteBuilder だけ

2. **`CsvTelemetryBuilder`**
   ```
   # robot telemetry
   battery_voltage,12.5
   motor_current,3.25
   cpu_temperature,41
   # end
   ```

3. **`JsonTelemetryBuilder`**
   ```
   {
     "title": "robot telemetry",
     "battery_voltage": 12.5,
     "motor_current": 3.25,
     "cpu_temperature": 41
   }
   ```
   - カンマを**項目の前**に置いてください。後ろに置くと最後の項目に余分なカンマが残ります

### B. 実務の形 — `src/motor_config.cpp`

4. **`MotorConfigBuilder` のセッタ 7 つ**
   - 戻り型は `MotorConfigBuilder &`。**値で返すとチェーンのたびに Builder ごとコピー**されます
   - `name()` は引数を値で受けているので `std::move` で移してください

5. **`MotorConfigBuilder::build() const &`**
   - `motor_id` が未設定なら `std::nullopt`
   - そうでなければ `config_` を**コピー**して返す（Builder はこのあとも使われうる）

6. **`MotorConfigBuilder::build() &&`**
   - 同じチェックのあと、`config_` を **`std::move`** して返す
   - こちらは一時オブジェクトからしか呼ばれないので、中身を持っていって構いません

`ControlLimitsBuilder`（ヘッダの下のほう）は **実装済みの見本**です。課題ではありません。
`constexpr` だけで組み立てると、実行時に 1 命令も走らずに ROM へ置ける、という例です。読んでください。

## 動かしてみる

```bash
./drill run dp07
```

## つまずきポイント

- セッタが `MotorConfigBuilder`（値）を返していると、テスト
  「チェーンは同じBuilderの参照を返す」がアドレス比較で落とします
- `build() &&` と `build() const &` は**両方**実装します。片方だけだと、
  もう片方の呼び方がコンパイルエラーになります
  （`error: 'this' argument to member function 'build' is an lvalue, but function has rvalue ref-qualifier`）
- `build() const &` で `std::move` すると、Builder が壊れて 2 回目の `build()` が空になります。
  **左辺値版はコピー**です
- Director に書式を書き始めたら手が滑っています。Director は `make_header` / `make_field` /
  `make_footer` を順に呼ぶだけです
- `TelemetryDirector` は `TelemetryBuilder &` を参照で持っています。
  **Builder より長生きさせられません**。Java には無い制約です

## テスト

```bash
./drill run dp07
```

9 つのテストがあります。出来上がりの文字列だけでなく、
**Director の呼び出し順序**、**チェーンがコピーを起こしていないこと**、
**`&&` 版がムーブしていること**まで見ます。
`constexpr` Builder は `static_assert` でコンパイル時に検査しています。

## 参考

- [7. Builder](../../docs/patterns/07_Builder.md)
- [cppreference: std::optional](https://en.cppreference.com/w/cpp/utility/optional)
- [cppreference: ref-qualifier（メンバ関数の参照修飾）](https://en.cppreference.com/w/cpp/language/member_functions)
- [cppreference: constexpr](https://en.cppreference.com/w/cpp/language/constexpr)
