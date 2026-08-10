# dp22 Command 〔デザインパターン編〕

結城本 第22章。ロボットアームのテレオペ指令を題材に、Command を 3 通り実装します。
**クラス版（undo / redo あり）**、**`std::function` 版（undo なし）**、
**マイコン向けの POD + 固定長リングバッファ版**です。

3 つ書くのが主題です。「どれを選ぶか」の判断基準が身につきます。

## やること

`src/robot_console.cpp` に実装してください。ヘッダの `RobotArm` は実装済みです。

1. **`RotateCommand::execute()` / `undo()` / `name()`**
   - 相対回転。**逆操作が自明**（+30 の逆は -30）なので状態を保存しません
   - `name()` は `"rotate"`

2. **`GripperCommand::execute()` / `undo()` / `name()`**
   - **逆操作が自明でない**例。`execute()` で実行前の状態を `previous_` に控えます
   - 「閉じる」の逆は「開く」ではありません。もともと閉じていたなら閉じたままが正解
   - `name()` は `closed_` なら `"grip"`、そうでなければ `"release"`

3. **`MacroCommand::add()` / `execute()` / `undo()` / `name()`**
   - 子は `std::vector<std::unique_ptr<Command>>` で**所有**します
   - `execute()` は積んだ順、**`undo()` は逆順**
   - `MacroCommand` 自身も `Command` なので、マクロの中にマクロを入れられます（Composite）

4. **`CommandHistory::run()` / `undo()` / `redo()`**
   - `run()` は実行して `done_` に積み、**`undone_` を捨てる**
   - `undo()` は `done_` の末尾を取り消して `undone_` へ移す
   - `redo()` は `undone_` の末尾を実行して `done_` へ戻す

5. **`ActionQueue::push()` / `run_all()`**
   - `std::function<void()>` 版。クラスを 1 つも作らないコマンドキュー
   - 空の `std::function` は積まないこと

6. **`MotorCommandRing::push()` / `pop()` と `apply()`**
   - 動的確保なし・仮想関数なしのマイコン版
   - 満杯なら**最も古いコマンドを落として** `false` を返す
   - `apply()` は `virtual` の代わりに `switch`

## 動かしてみる

```bash
./drill run dp22
```

## つまずきポイント

- **`undo()` の順序が本題です。** マクロを正順に undo すると、
  依存のある操作（掴んでから持ち上げる、など）で状態が壊れます。末尾から戻してください
- **`CommandHistory::run()` で `undone_.clear()` を忘れない。**
  undo したあと別のコマンドを実行してから redo できてしまうと、
  復元できない状態になります
- `done_.back()` を `std::move` してから `pop_back()` してください。
  順番を逆にすると解放済みのものを触ります
- `add()` / `run()` / `push()` は `std::unique_ptr` を**値で**受け取ります。
  `const std::unique_ptr<Command> &` で受けるとコピーできず、そもそも積めません
- **コマンドは `RobotArm` を所有しません。** `RotateCommand` が持っているのは
  生ポインタです。アームより長生きさせるとぶら下がります（第17章 Observer と同じ問題）
- `MotorCommandRing` の `head_` は「**次に書く位置**」です。
  最古は `(head_ + kCapacity - size_) % kCapacity`。
  `std::size_t` が負にならないよう `kCapacity` を足してから `%` を取ります
- `MotorCommand` に `std::string` や `std::function` を足したくなったら止まってください。
  テストの `static_assert`（`is_trivially_copyable` / `sizeof <= 4`）がコンパイルを止めます

## テスト

```bash
./drill run dp22
```

10 個のテストがあります。実行できることだけでなく、
**積んだ順に実行されるか**、**undo が逆順か**、
**マクロが 1 個のコマンドとして履歴に積まれるか**、
**`std::function` 版とクラス版で結果が一致するか**、
**リングバッファが容量を超えたら古いものから落とすか**まで見ます。

## 参考

- [22. Command](../../docs/patterns/22_Command.md)
- [18. Memento](../../docs/patterns/18_Memento.md) — 状態を丸ごと保存する側との使い分け
- [11. Composite](../../docs/patterns/11_Composite.md) — マクロコマンドの構造
- [cppreference: std::function](https://en.cppreference.com/w/cpp/utility/functional/function)
- [cppreference: std::move_only_function](https://en.cppreference.com/w/cpp/utility/functional/move_only_function)
