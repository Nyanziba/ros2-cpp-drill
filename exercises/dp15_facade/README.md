# dp15 Facade 〔デザインパターン編〕

結城本 第15章。ロボットの起動シーケンス

```
電源投入 → センサ初期化 → キャリブレーション → 通信確立
```

を **1 つの窓口**にまとめます。ただし C++ では窓口の作り方が 2 通りあるので、
**両方を実装して、同じ結果になること**を確かめます。

1. **名前空間 + 自由関数版** `robot::start_once()` — 結城本の `PageMaker` に相当。
   `static` メソッドだけのクラスは C++ では書きません
2. **RAII クラス版** `robot::RobotSession` — 状態を持つのでクラスにする。
   コンストラクタで初期化、デストラクタで後始末。**こちらが C++ 版 Facade の本命です**

## やること

`src/robot_startup.cpp` に実装します。ヘッダは編集しません。

### その1: サブシステム（無名名前空間の中の自由関数 8 個）

1. **`power_on` / `sensor_init` / `calibrate` / `link_up`** —
   成功なら `"power_on"` などをログに足して `true`、
   失敗なら `"power_on_failed"` などを足して `false`
2. **`power_off` / `sensor_deinit` / `calibration_clear` / `link_down`** — 後始末側

**この 8 個はヘッダに出てきません。** 無名名前空間に入れると内部リンケージになり、
他の翻訳単位からは名前すら存在しません。
「見せる面を減らす」のが Facade の本質です。

### その2: 後始末を 1 箇所に集める

3. **`teardown(completed_stages, log)`** — 完了した段数を受け取り、**逆順**で戻す

```cpp
if (completed_stages >= 4) { link_down(log); }
if (completed_stages >= 3) { calibration_clear(log); }
if (completed_stages >= 2) { sensor_deinit(log); }
if (completed_stages >= 1) { power_off(log); }
```

フォールスルーではなく**独立した `if` を降順に並べる**のが要点です。

### その3: 名前空間 + 自由関数版

4. **`robot::start_once()`** — 4 段を順に呼び、
   失敗したらそこで打ち切って `teardown(成功した段数, log)`。
   成功しても最後に `teardown(4, log)`（「1 回起動して、その場で止める」窓口だから）

`return` が 5 通りあり、**そのすべてで `teardown()` を呼ぶ**必要があります。
1 つ忘れても誰も教えてくれません。これが次の RAII 版の動機です。

### その4: RAII クラス版

5. **コンストラクタ** — 起動シーケンスを走らせる。
   成功するたび `completed_stages_` を増やし、失敗したら `failed_stage_` を入れて打ち切る。
   **後始末をここに書いてはいけません**
6. **デストラクタ** — `teardown(completed_stages_, log_)` を呼ぶだけ
7. **ムーブコンストラクタ** — ムーブ元を空にする
   （`completed_stages_ = 0` / `ready_ = false` / `log_ = nullptr`）。
   **忘れると後始末が 2 回走ります**
8. **`drive(duty)`** — `is_ready()` でなければ何もせず `false`。
   そうでなければ `"drive:50"` の形でログに足して `true`

## 動かしてみる

```bash
./drill run dp15
```

## つまずきポイント

- コンストラクタの途中で `return` しても**デストラクタは必ず走ります**。
  オブジェクトは構築済みだからです。だから巻き戻しはデストラクタに書けます
- `std::move` した後のオブジェクトも**デストラクタは走ります**。
  「ムーブしたから消えた」ではありません。ムーブ元を空にしないと電源が 2 回落ちます
- 電源投入で失敗した場合、成功した段は 0 個です。**`power_off` を呼んではいけません**。
  テスト「電源投入で失敗すると後始末は何も走らない」がそこを見ます
- `append()` は `const char *` を取ります。`drive()` は `std::string` を作るので、
  `log_->push_back()` を直接使ってください（`log_` が `nullptr` のことがあります）
- 自由関数版を「`RobotSession` を作って捨てるだけ」で実装したくなりますが、
  **手で巻き戻す形で書いてください。** RAII 版との差を体で知るのがこの課題の主眼です

## テスト

```bash
./drill run dp15
```

11 個のテストがあります。手順が正しい順序で走るか、
失敗した段より先が走らないか、成功済みの段だけが逆順で巻き戻るか、
2 つの版のログが一致するか、ムーブ後に後始末が二重に走らないか、
コピー禁止・`explicit` まで `static_assert` で見ます。

## 参考

- [15. Facade](../../docs/patterns/15_Facade.md)
- [cppreference: std::fstream](https://en.cppreference.com/w/cpp/io/basic_fstream)
- [cppreference: unnamed namespaces](https://en.cppreference.com/w/cpp/language/namespace)
