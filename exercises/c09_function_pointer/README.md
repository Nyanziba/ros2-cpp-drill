# c09 関数ポインタとテーブル駆動型プログラミング 〔C言語編〕

LED コントローラーのハンドラーを関数ポインタで管理し、テーブル駆動型プログラミングを学ぶ課題です。

## ヘッダは編集しません

`include/drill/function_pointer.h` は**編集しません**。まず読んでください。

```c
/* コールバック関数型。state は 0 (OFF) または 1 (ON)。 */
typedef void (*led_callback_t)(int led_id, int state);

/* LED の ON/OFF ハンドラーを登録します。 */
void led_register_handler(struct LedController * ctrl, int led_id, led_callback_t handler);

/* LED を制御します。登録されているハンドラーがあれば呼びます。
 * NULL（未登録）なら何もしません（安全）。 */
void led_set(struct LedController * ctrl, int led_id, int state);
```

## やること

編集するのは `src/function_pointer.c` だけです。

### 1. LED コントローラーの作成と破棄

```c
struct LedController * led_controller_create(void)
```

`malloc` で `LedController` を確保し、**すべてのハンドラーを NULL で初期化**してください。

```c
void led_controller_destroy(struct LedController * ctrl)
```

渡されたポインタを `free` で解放してください。

### 2. ハンドラー登録と呼び出し

```c
void led_register_handler(struct LedController * ctrl, int led_id, led_callback_t handler)
```

ハンドラーテーブルに関数ポインタを代入します。`led_id` の範囲チェックは呼び出し側で行うと仮定します。

```c
void led_set(struct LedController * ctrl, int led_id, int state)
```

登録されているハンドラーが NULL でなければ、関数ポインタ経由で呼び出してください。

```c
if (ctrl->handlers[led_id] != NULL) {
  (*ctrl->handlers[led_id])(led_id, state);
}
```

**NULL なら何もしない**（安全）。ここが最重要です。

### 3. ハンドラーの null チェック

```c
int led_handler_is_null(const struct LedController * ctrl, int led_id)
```

ハンドラーが NULL なら 1、そうでなければ 0 を返してください。

## 関数ポインタの宣言と呼び出しの読み方

### 宣言

```c
typedef void (*led_callback_t)(int, int);
```

「`led_callback_t` は `int, int` を取り `void` を返す関数へのポインタ型」

右から左に読みます：
- ポインタ `*`
- 関数 `(int, int) -> void`

### 呼び出し

```c
led_callback_t handler = ...;
handler(led_id, state);      /* 通常の書き方 */
(*handler)(led_id, state);   /* 関数ポインタを明示的に参照解除 */
```

どちらでも動作します。C では関数ポインタの参照解除は暗黙的に行われます。

## 動かしてみる

```bash
./drill run c09
```

**未着手のうちはテストが失敗します。** すべてのテストが緑になればクリアです。

## つまずきポイント

1. **NULL チェックが必須** — 未登録のスロットへの `led_set` は何もしない。クラッシュしない安全性が大事。
2. **関数ポインタの宣言は複雑** — `int (*fp)(int)` は「`int` を取り `int` を返す関数へのポインタ」。宣言を読むときは typedef で頭を整理する。
3. **テーブル駆動型** — ハンドラーをテーブル（配列）に登録しておき、イベントが起きたときに該当のハンドラーを呼び出すパターン。ディスパッチャの基本。
4. **初期化を忘れずに** — `malloc` の直後に全ハンドラーを NULL で初期化しないと、ゴミがポインタとして扱われます。

## テスト

| テスト | 見ているところ |
| --- | --- |
| `コントローラー作成と破棄` | `create` で確保、NULL チェック、全ハンドラー初期化 |
| `ハンドラーを登録できる` | `register_handler` と `is_null` |
| `登録されたハンドラーが呼ばれる` | 関数ポインタ経由での呼び出し |
| `未登録のスロットを呼んでも落ちない` | **NULL チェックの重要性** |
| `ハンドラーを複数登録して正しく呼び分ける` | テーブル駆動型の基本 |
| `ハンドラーを NULL で削除できる` | NULL 再登録で解除 |
| `ハンドラーを上書きできる` | 同じスロットへの再登録 |

## 参考

- [9. 関数ポインタ](../../docs/c/09_関数ポインタ.md)
