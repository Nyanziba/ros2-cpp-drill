# dp16 Mediator 〔デザインパターン編〕

結城本 第16章。ロボットの操作パネルを題材に、Colleague（部品）どうしを直接つながせず、
Mediator（`ControlPanel`）だけが調停する構造を作ります。

C++ での急所は**相互参照**です。Mediator は Colleague を持ち、Colleague は Mediator を指します。
両方を `std::shared_ptr` にすると**循環参照でどちらも解放されません**。

## 調停ルール

| 状態 | 非常停止トグル | 自動モードトグル | 手動ボタン 2 つ |
| --- | --- | --- | --- |
| どちらもオフ | 有効 | 有効 | 有効 |
| 自動モード オン | 有効 | 有効 | **無効** |
| 非常停止 オン | 有効 | **無効** | **無効** |

非常停止トグルだけは常に有効です。無効にすると解除できなくなります。

## やること

`src/control_panel.cpp` に 6 つ実装してください。

1. **`PanelWidget::set_mediator()`**
   - `mediator_` に保存するだけ。**所有しません**（生ポインタ）

2. **`PanelWidget::notify_changed()`**
   - `mediator_` が `nullptr` でなければ `widget_changed(this)` を呼ぶ
   - `nullptr` のときは何もしない（2 段階初期化の途中がありうるため）

3. **`ToggleWidget::set_checked()`**
   - 無効なら何もしない / 値が変わらないなら何もしない / 変わったら報告する

4. **`ButtonWidget::press()`**
   - 無効なら `false`。有効なら押下回数を増やして報告し `true`

5. **`ControlPanel` のコンストラクタ**
   - 4 つの部品に `set_mediator(this)` を呼び、`update_enabled_states()` で初期化

6. **`ControlPanel::widget_changed()` と `update_enabled_states()`**
   - 誰から来たかを `change_log_` に記録し、上の表どおりに有効・無効を決め直す

## 動かしてみる

```bash
./drill run dp16
```

## つまずきポイント

- **`set_enabled()` から `notify_changed()` を呼ばないこと。**
  `widget_changed()` → `update_enabled_states()` → `set_enabled()` → `widget_changed()` と
  無限再帰します。「Mediator から呼ばれる入口」と「Colleague から呼ぶ入口」は別物です
- Colleague の中から他の Colleague を触らないこと。テスト
  「Mediatorを外すとColleague間に影響が伝わらない」がそれを落とします
- `set_mediator()` で `std::shared_ptr` を持ち返さないこと。テスト
  「ColleagueはMediatorを所有しない」が `use_count()` で見ています
- コンストラクタの初期化子リストでは結線できません。Colleague を作るには Mediator が要り、
  Mediator を渡すには Colleague が要るからです。だから 2 段階初期化になります
- 値が変わっていないときに報告すると、テスト「同じ値をもう一度入れても報告されない」が落ちます

## テスト

```bash
./drill run dp16
```

12 個のテストがあります。調停ルールに加えて、
**Mediator を経由していること**・**Mediator が Colleague を所有していること**・
**破棄で全 Colleague のデストラクタが呼ばれること**まで見ます。

## 参考

- [16. Mediator](../../docs/patterns/16_Mediator.md)
- [cppreference: std::weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
