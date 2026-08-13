# dp19 State 〔デザインパターン編〕

結城本 第19章。ロボットの動作状態機械を、**同じ規則のまま 3 通り**に実装して比べます。

## 遷移規則

```
Stopped --PowerOn--> Idle --Start--> Running --Stop--> Idle
どの状態からでも EmergencyStop --> Faulted
Faulted --Reset--> Stopped          （手動リセットでしか抜けられない）
表に無い組み合わせは無視。状態は変わらず、入場/退場アクションも走りません。
```

入場・退場アクション（3 実装とも同じ並びを出すこと）:

- 退場: `"exit:<状態名>"`。**Running から出るときだけ**続けて `"motor:stop"`
- 入場: `"enter:<状態名>"`。**Faulted に入るときだけ**続けて `"brake:engage"`
- **退場が先、入場が後**

## やること

`src/state_machine.cpp` に 3 通り実装してください。

### 手段1: `enum` + `switch`

1. **`EnumStateMachine::next_state()`** — 遷移表を引くだけの静的関数。副作用なし
2. **`EnumStateMachine::handle()`** — 遷移するときだけ `log_exit` → 差し替え → `log_enter`

`switch` に `default:` を書かないでください。全 `enum` 値を列挙しておくと、
あとで状態を足したときに**書き漏れをコンパイラが警告してくれます**。

### 手段2: State クラス（GoF 版）

3. **4 つの `State` 派生クラスの `handle()`** — 遷移先の `const State *` を返すだけ。
   遷移しないなら `this`
4. **`RunningStateObject::on_exit()` / `FaultedStateObject::on_enter()`** を override
5. **`state_object()`** — 状態 id に対応する `static` 実体を返す
6. **`ClassStateMachine::handle()`** — 遷移先を**受け取ってから**差し替える

### 手段3: `std::variant` + `std::visit`

7. **`id_of()`** — `std::visit` と `if constexpr` で変換
8. **`VariantStateMachine::next_state()`** — 遷移先を返す。遷移しないなら `std::nullopt`
9. **`VariantStateMachine::handle()`**

## 動かしてみる

```bash
./drill run dp19
```

## つまずきポイント

- **`handle()` の中で状態機械を差し替えないでください。** そもそもできないように、
  `State::handle()` は `Context` を受け取りません。これがこの章で一番大事なところです。
  Java 版の `context.changeState(...)` を C++ で写経すると `delete this` になります
- **遷移しない入力ではアクションを走らせない**こと。`Running` で `Start` をもう一度受けたときに
  `on_exit` → `on_enter` が走ると、実機ではモータが一瞬止まって再起動します
- **退場が先、入場が後**です。逆にするとブレーキがかかった後にモータ停止が走ります
- `state_object()` は何度呼んでも**同じアドレス**を返さなければいけません。
  状態オブジェクトはメンバを 1 つも持たないので、`static` 実体 1 個で足ります（ヒープ確保ゼロ）
- `std::visit` のラムダで分岐ごとに戻り値の型が違うとエラーになります。
  `-> std::optional<StateVariant>` のように戻り値型を明示してください
- `EmergencyStop` は「どの状態からでも Faulted へ」ですが、**既に Faulted なら遷移しません**
  （＝ `false` が返り、`brake:engage` が二重に走らない）

## テスト

```bash
./drill run dp19
```

13 個のテストがあります。遷移表の全 20 通り、アクションの順序、
遷移後も遷移元の状態オブジェクトが生きていること、`variant` 版が非多態であること、
そして**3 つの実装が同じ遷移列・同じログを返すこと**まで見ます。

## 参考

- [19. State](../../docs/patterns/19_State.md)
- [C言語編 9. 関数ポインタ](../../docs/c/09_関数ポインタ.md) — マイコン版のテーブル駆動と同じ道具
- [cppreference: std::variant](https://en.cppreference.com/w/cpp/utility/variant)
- [cppreference: std::visit](https://en.cppreference.com/w/cpp/utility/variant/visit)
