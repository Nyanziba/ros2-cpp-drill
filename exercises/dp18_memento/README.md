# dp18 Memento 〔デザインパターン編〕

結城本 第18章。PID ゲインの調整履歴を題材に、Memento を 2 通り実装します。
**値セマンティクスを使った普通の版**と、**マイコン向けの固定長リングバッファ版**です。

## やること

`src/gain_tuner.cpp` に 7 つ実装してください。

1. **`GainTuner::create_snapshot()`**
   - 現在の `kp_` / `ki_` / `kd_` / `label_` を持つ `GainSnapshot` を**値で**返す
   - `GainSnapshot` のコンストラクタは private ですが、`GainTuner` は `friend` なので呼べます

2. **`GainTuner::restore(const GainSnapshot &)`**
   - Memento の中身を自分に書き戻す。`label_` は**コピー**
   - snapshot 側は変化しません。あとでもう一度戻せます

3. **`GainTuner::restore(GainSnapshot &&)`**
   - ムーブ版。`label_` は `std::move` で奪う
   - 奪ったあと `snapshot.label_.clear()` を呼ぶこと。
     ムーブ後の `std::string` の中身は規格上「未規定」なので、
     「空になる」と約束するなら自分で空にします

4. **`GainTuner::capture_state()` / `restore_state()`**
   - マイコン向けの POD 経路。`GainState` は `kp` / `ki` / `kd` だけ
   - `restore_state()` は `label_` を触りません

5. **`GainHistory::push()` / `size()` / `recent()`**
   - 固定長リングバッファ。`std::vector` は使いません
   - 満杯なら最も古いものを捨てる
   - `recent(0)` が最新、`recent(1)` が 1 つ前

## 動かしてみる

```bash
./drill run dp18
```

## つまずきポイント

- **Memento のメンバは値で持ちます。** `const State &` や `State *` や
  `std::shared_ptr<State>` で持つとスナップショットになりません。
  元を変えると Memento も一緒に変わります。
  `shared_ptr` が防ぐのは**寿命**であって共有ではありません
- `GainSnapshot` のコンストラクタが private なのは意図どおりです。
  Java の package private に相当する仕組みが C++ に無いので `friend` で表現しています。
  外から `saved.kp_` に触ると
  `error: 'kp_' is a private member of 'GainSnapshot'` になります
- `GainSnapshot` のコピーは**禁止しないでください**。
  Undo 履歴は `std::vector<GainSnapshot>` に積むので、コピーもムーブも消すと
  `push_back` すら通りません
- `head_` は「**次に書く位置**」です。最新は `head_` の 1 つ手前。
  引き算で `std::size_t` が負にならないよう、`kCapacity` を足してから `%` を取ってください
- `GainState` に `std::string` を足したくなったら止まってください。
  `memcpy` で運べなくなり、ヘッダの `static_assert` がコンパイルを止めます

## テスト

```bash
./drill run dp18
```

9 つのテストがあります。復元できることだけでなく、
**保存後に元を変えても Memento が変わらないこと**（スナップショットになっているか）、
`GainSnapshot` が外から構築できないこと、
リングバッファが容量を超えたら古いものから捨てることまで見ます。

## 参考

- [18. Memento](../../docs/patterns/18_Memento.md)
- [cppreference: std::is_trivially_copyable](https://en.cppreference.com/w/cpp/types/is_trivially_copyable)
- [cppreference: friend declaration](https://en.cppreference.com/w/cpp/language/friend)
