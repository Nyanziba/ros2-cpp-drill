# dp17 Observer 〔デザインパターン編〕

結城本 第17章。**この講習の山場**です。

Java 版の `addObserver()` は `void` を返して終わりです。C++ でそれをやると、
**購読者が先に死んだ瞬間に Subject が宙に浮いたポインタを叩きます。**
ここでは「購読を表す RAII トークン（`Subscription`）を返す」方式で解きます。

## やること

`src/sensor_hub.cpp` の TODO(1)〜(8) を埋めてください。

1. **`detail::Registry::remove()`**
   - id に一致する `Entry` の `observer` に `nullptr` を入れる（**印を付けるだけ**）
   - `notifying` が false のときだけ `compact()` を呼ぶ

2. **`detail::Registry::compact()`**
   - `observer == nullptr` の要素を実際に取り除く

3. **`Subscription::~Subscription()`**
   - `reset()` を呼ぶだけ。トークン方式の価値はこの 1 行に全部あります

4. **`Subscription`（ムーブコンストラクタ）**
   - ムーブ元を「何も購読していない」状態に戻す

5. **`Subscription::operator=`（ムーブ代入）**
   - 自己代入を弾く → 今の購読を解除 → 奪う → 相手を空にする

6. **`Subscription::reset()`**
   - `weak_ptr::lock()` が空なら **何もしない**（Subject がもう死んでいる）
   - 2 回呼んでも安全であること

7. **`Subscription::active()`**
   - Subject が先に死んだ場合も false になること

8. **`SensorHub::subscribe()` / `publish()` / `observer_count()`**
   - `publish()` は登録順。再入防止・遅延削除・添字ループの 3 点に注意

## 動かしてみる

```bash
./drill run dp17
```

## つまずきポイント

- **通知ループの中で `entries.erase()` を呼ばない。**
  走っているループの添字がずれて、飛ばされる購読者と 2 回呼ばれる購読者が出ます。
  印を付けるだけにして、削除はループが終わってから
- **参照やイテレータを取り置きしない。** 通知の最中に `subscribe()` されると
  `std::vector` が再確保され、取り置きしたものが無効になります。添字で回してください
- **再入防止フラグを戻し忘れない。** `on_sample` が例外を投げても戻るように、
  デストラクタで false にする小さな RAII をその場で書くのが安全です
- **`Subscription` のムーブ元を空にする。** 忘れると、ムーブ元が死んだ瞬間に
  購読が切れます（`unique_ptr` と同じ話です）
- **`registry_` は `weak_ptr`。** 生ポインタで持つと、`SensorHub` が先に死んだあとの
  `~Subscription()` で死んだオブジェクトを触ります

## テスト

```bash
./drill run dp17
```

10 個のテストがあります。通知が届くことだけでなく、

- 観測者が先に死んでも Subject が壊れないこと
- 通知の最中に解除しても落ちないこと
- 通知が循環しても無限ループしないこと
- `Subscription` がコピー不可・ムーブ可であること
- **Subject が先に死んでもトークンの破棄が安全であること**

まで見ます。

## 参考

- [17. Observer](../../docs/patterns/17_Observer.md)
- [cppreference: std::weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)
- [cppreference: std::remove_if](https://en.cppreference.com/w/cpp/algorithm/remove)
