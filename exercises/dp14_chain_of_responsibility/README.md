# dp14 Chain of Responsibility 〔デザインパターン編〕

結城本 第14章。ロボットの異常検知（電圧低下・過電流・通信断）を、
段階的に別のハンドラへたらい回しする連鎖として書きます。

この章の主題は構造ではなく **連鎖の寿命** です。
`next` を `std::unique_ptr` で所有する版（GoF 版）と、
連鎖を作らず配列を順に回す版の**両方**を実装して比べます。

## やること

`src/fault_chain.cpp` に 6 つ実装してください。

1. **`FaultHandler::~FaultHandler()`**
   - `destruction_log_` が非 `nullptr` なら `name_` を `push_back`
   - 「先頭を捨てたら連鎖全部が消えた」ことをテストから観測するための仕掛けです

2. **`FaultHandler::set_next()`**
   - `next` を `next_` に move して所有する
   - **戻り値は `*this` ではなく `*next_`**。`a.set_next(b).set_next(c)` を成立させるため

3. **`FaultHandler::support()`**
   - 自分の `resolve()` → ダメなら `next_->support()` → 次もいなければ `std::nullopt`
   - **例外は投げません**

4. **`FaultHandler::support_alone()`**
   - 次には回さず、自分の `resolve()` の結果だけを返す

5. **`dispatch()`**
   - 連鎖を作らない方式。配列を先頭から `support_alone()` で聞いていく

6. **`LowVoltageHandler` / `OverCurrentHandler` / `CommTimeoutHandler` の `resolve()`**
   - 自分が処理できるなら `FaultAction`、できないなら `std::nullopt`
   - **ここに「次に回す」を書いてはいけません**（NVI）

| ハンドラ | 処理する条件 | action |
| --- | --- | --- |
| `LowVoltageHandler` | `kLowVoltage` かつ `magnitude <  threshold_mv` | `"reduce_duty"` |
| `OverCurrentHandler` | `kOverCurrent` かつ `magnitude >= limit_ma` | `"cut_output"` |
| `CommTimeoutHandler` | `kCommTimeout` かつ `magnitude >= timeout_ms` | `"safe_stop"` |

`kEncoderSlip` は**誰も処理しません**。`std::nullopt` になることを確かめます。

## 動かしてみる

```bash
./drill run dp14
```

## つまずきポイント

- `set_next()` で `std::move(next)` した**あと**の `next` は空です。
  `*next` を返すと落ちます。`next_` に入れてから `*next_` を返してください
- `set_next()` が `*this` を返すと、`a.set_next(b).set_next(c)` で
  **b が黙って解放されます**。テスト「setNextは次のハンドラ自身への参照を返す」が落とします
- デストラクタ本体が走ったあとにメンバ `next_` が破棄されます。
  つまり破棄ログは**先頭から末尾の順**に並びます
- `resolve()` の中で `next_` を触りたくなったら設計が壊れています。
  たらい回しは `support()` だけの仕事です
- 「誰も処理しなかった」は `std::nullopt` です。`throw` しないでください
  （マイコンでは `-fno-exceptions` が普通のため）

## テスト

```bash
./drill run dp14
```

10 個のテストがあります。順番を入れ替えると処理するハンドラが変わること、
先頭を破棄すると連鎖全体が破棄されること（デストラクタのログの並び）まで見ます。

## 参考

- [14. Chain of Responsibility](../../docs/patterns/14_ChainOfResponsibility.md)
- [cppreference: std::optional](https://en.cppreference.com/w/cpp/utility/optional)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
