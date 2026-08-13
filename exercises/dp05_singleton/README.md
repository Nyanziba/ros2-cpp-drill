# dp05 Singleton 〔デザインパターン編〕

結城本 第5章。**ロボットに 1 本しかない UART** をシングルトンにします。

「設定を持ち回るのが面倒だから」ではありません。
**物理的に 1 個しか無いから**です。この理由が言えないものはシングルトンにしないでください
（[0. 使う前に](../../docs/patterns/00_使う前に.md) の 0.2）。

## やること

`src/uart_port.cpp` に 5 か所実装してください。すべて **関数ローカル static**（Meyers Singleton）が鍵です。

1. **`UartPort::instance()`**
   - `static UartPort the_port;` を関数の中に置いて、その参照を返す
   - 今の実装は呼ばれるたびに `new` していて、毎回別のオブジェクトになっています

2. **`UartPort::construction_count()`**
   - `g_uart_construction_count` を返す

3. **`UartPort::reset()`**
   - `baud_rate_` を `kDefaultBaudRate` に、`sent_lines_` を空にする
   - **オブジェクトを作り直してはいけません。状態だけ戻します**

4. **`LazyProbe::instance()`**
   - 同じく関数ローカル static

5. **`LazyProbe::was_constructed()`**
   - `g_lazy_probe_constructed` を返す

`include/drill/uart_port.hpp` と `test/test_exercise.cpp` は編集しません。

## 動かしてみる

```bash
./drill run dp05
```

## つまずきポイント

- **関数ローカル static は C++11 以降スレッドセーフです。** 自分で `std::mutex` を
  書く必要はありません（マジックスタティック）。書くとむしろ遅くなります
- `instance()` は **参照** を返します。ポインタを返すと、呼んだ側が
  `delete` してよいのか分からなくなります
- `reset()` で `*this = UartPort{};` と書きたくなりますが、**通りません**。
  コピー代入もムーブ代入も `= delete` されているからです。メンバを 1 つずつ戻します
- コピー・ムーブの `= delete` が 4 行あるのは飾りではありません。
  消すと `UartPort port = UartPort::instance();` が通り、シングルトンが崩れます。
  テストの先頭の `static_assert` がそれを見張っています
- コンストラクタの中でハードウェアを触らないでください。
  **いつ走るか分からない**からです（詳しくは記事の「マイコンでの結論」）

## テスト

```bash
./drill run dp05
```

8 つのテストがあります。

| テスト | 見ているもの |
| --- | --- |
| `instanceは何度呼んでも同じオブジェクトを返す` | アドレスが一致するか |
| `状態が唯一のインスタンスで共有される` | 別経路から同じ状態が見えるか |
| `初期化は何度instanceを呼んでも一度しか走らない` | コンストラクタが 1 回だけか |
| `resetでボーレートが既定値に戻る` / `resetで送信履歴が空になる` | `reset()` の中身 |
| `resetはオブジェクトを作り直さない` | アドレスと構築回数が変わらないか |
| `前のテストの状態が残っていない` | テスト間の状態の漏れ |
| `初期化は最初のinstance呼び出しまで走らない` | 遅延初期化になっているか |

`static_assert` でコピー・ムーブ・外部からの構築が禁止されていることも見ています。

## 参考

- [5. Singleton](../../docs/patterns/05_Singleton.md)
- [C++入門編 7. static](../../docs/cpp-basics/07_static.md)
- [cppreference: Storage class specifiers（静的局所変数）](https://en.cppreference.com/w/cpp/language/storage_duration)
