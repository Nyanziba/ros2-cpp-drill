# dp20 Flyweight 〔デザインパターン編〕

結城本 第20章。型番ごとの較正テーブルを、センサ何十個で共有します。

Flyweight は**唯一、最適化のためのパターン**です。この課題の題材も、実務なら
`constexpr` テーブル（`include/drill/calibration.hpp` の `kCalibrationRom`）で終わります。
それでも一度手で書くのは、**「constexpr で終わり」だと判断できるようになるため**です。

## やること

`src/calibration.cpp` に 3 つ実装してください。

1. **`CalibrationRegistry::get()`**
   - プールに生きているものがあれば、それを返す（`std::weak_ptr::lock()`）
   - `lock()` が `nullptr` なら「誰も使わなくなった残骸」。作り直す
   - `find_spec(model_id)` が `nullptr`（ROM に無い型番）なら `nullptr` を返す。例外は投げない
   - `std::make_shared` で作り、**`weak_ptr` として**プールに登録する

2. **`CalibrationRegistry::sweep_expired()`**
   - `expired()` なエントリを消し、消した個数を返す

3. **`Sensor::convert()`**
   - `raw * gain + offset + zero_offset`
   - `gain` / `offset` は共有（本質的状態）、`zero_offset` は個体ごと（付帯的状態）
   - テーブルが `nullptr` なら `0.0`

`constexpr` 版（`kCalibrationRom` / `find_spec`）は**実装済み**です。先に読んでください。

## 動かしてみる

```bash
./drill run dp20
```

## つまずきポイント

- **プールに `shared_ptr` を入れてはいけません。** 入れるとプールが参照を握り続け、
  プロセスが終わるまで解放されません。テストは `use_count` を見ています。
  利用者が 2 人なら `use_count` は **2** です。3 になったらプールが数えています
- **`weak_ptr` にしても、`map` のエントリは自動では消えません。**
  `weak_ptr` は自分が expired になったことを `map` に伝えられないからです。
  だから `sweep_expired()` が要ります
- `sweep_expired()` では `it = pool_.erase(it)` と書いてください。
  `++it` してから `erase` すると無効なイテレータを触ります
- `zero_offset` を `CalibrationTable` 側に持たせないでください。
  その瞬間、同じ型番のセンサ同士で共有できなくなります
- `Handle` は `std::shared_ptr<const CalibrationTable>` です。`const` が付いています。
  共有しているものを 1 人が書き換えたら、全員に波及するからです

## テスト

```bash
./drill run dp20
```

7 つのテストがあります。

- 同じ型番を 2 回引くと**同一のアドレス**が返ること
- 生成回数が、引いた回数（5 回）ではなく**種類の数**（3 つ）と一致すること
- 全員が手放したら破棄され、`sweep_expired()` でプールからも消えること
- 付帯的状態が共有されないこと
- `constexpr` テーブルが `static_assert` を通り、**実行時にヒープ確保が 0 回**であること
  （テストがグローバルの `operator new` を差し替えて数えています）

## 参考

- [20. Flyweight](../../docs/patterns/20_Flyweight.md)
- [cppreference: std::weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)
- [cppreference: std::string_view](https://en.cppreference.com/w/cpp/string/basic_string_view)
