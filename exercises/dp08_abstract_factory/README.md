# dp08 Abstract Factory 〔デザインパターン編〕

結城本 第8章。**製品群（family）をまるごと差し替える**構造を、実行時版とテンプレート版の
両方で書きます。

題材は「モータ出力とエンコーダ入力の対」です。
実機用の部品群とシミュレーション用の部品群があり、**この 2 つを混ぜてはいけません**。
実機のモータとシミュのエンコーダを組み合わせたら、制御はそのまま暴走します。

## やること

### `src/actuator_kit.cpp`（実行時版）

1. **具体製品を 4 つ**（匿名 namespace の中に）
   - `SimMotor` / `SimEncoder` / `HwMotor` / `HwEncoder`
   - 中身はヘッダの `*Core` を値で持つだけ。`kit_id()` は自分の製品群を返す

2. **`SimulationKitFactory` / `HardwareKitFactory` の `create_motor()` / `create_encoder()` / `kit_id()`**
   - `std::make_unique` で作って返す
   - **同じファクトリからは同じ bus / registers を渡す**。ここが Abstract Factory の肝です

3. **`run_open_loop()`**
   - 具体ファクトリの名前を**1 つも書かずに**書けること
   - 書けなかったら抽象化ができていません

### `src/static_kit.cpp`（テンプレート版）

4. **`run_open_loop_static<KitTraits>()`** を書き、
   `run_open_loop_static_sim` / `run_open_loop_static_hw` から呼ぶ
   - 生成は `make_unique` ではなく「その場に置く」だけ
   - vtable もヒープ確保もゼロ。マイコンではこちらが本命です

## 動かしてみる

```bash
./drill run dp08
```

## つまずきポイント

- `create_motor()` と `create_encoder()` に**別の bus を渡す**と、製品群が混ざります。
  テスト「同じファクトリから出た部品どうしは繋がっている」が落とします
- `run_open_loop()` の中に `SimulationKitFactory` と書いたら設計が壊れています。
  引数は `const ActuatorKitFactory &` だけです
- 実機側は 4 逓倍エンコーダなので、同じ duty でもカウントは 4 倍進みます。
  **製品群が違えば挙動も違って当然**です。合わせようとしないでください
- テンプレート版で `typename KitTraits::Motor` の `typename` を忘れるとコンパイルエラーです
- `MotorOutput` / `EncoderInput` / `ActuatorKitFactory` の仮想デストラクタはヘッダ側にあります。
  自分で基底クラスを増やすなら、必ず仮想デストラクタを書いてください

## テスト

8 つのテストがあります。ファクトリを差し替えるだけで同じ抽象コードが両方の製品群で動くこと、
生成された部品が同じ製品群に属していること、所有権が呼び出し側にあること、
テンプレート版が実行時版と同じ結果になることを見ます。

## 参考

- [8. Abstract Factory](../../docs/patterns/08_AbstractFactory.md)
- [0. 使う前に](../../docs/patterns/00_使う前に.md) — 0.3「生成が 3 段になる」
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
