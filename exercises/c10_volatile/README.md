# c10 volatile と割り込み安全性 〔C言語編〕

`volatile` キーワードが何を保証し、何を保証しないかを学ぶ課題です。

## 重要：volatile はスレッドセーフではありません

`volatile` は**コンパイラの最適化を防ぐだけ**です。

- ✅ 最適化での読み込み削除を防ぐ
- ✅ メモリマップドハードウェアレジスタへのアクセス
- ❌ 複数スレッド間の同期
- ❌ アトミック性（分割できない操作）

誤った使い方：`volatile int x; x++` をスレッドセーフにはしません。`volatile` だけでは**データレースです。**

## ヘッダは編集しません

`include/drill/volatile_state.h` は**編集しません**。

外部から監視される共有変数を使った状態機械です：

```c
extern volatile state_t g_machine_state;
```

## やること

編集するのは `src/volatile_state.c` だけです。

### 1. 初期化

```c
void machine_init(void)
```

`g_machine_state` を `STATE_IDLE` に設定してください。

### 2. 状態遷移

```c
void machine_start(void)
void machine_stop(void)
```

`g_machine_state` を変更してください。

### 3. 状態確認

```c
state_t machine_get_state(void)
int machine_is_idle(void)
int machine_is_running(void)
int machine_is_stopped(void)
```

`g_machine_state` から読み込んで現在の状態を返してください。

### 4. 外部からの変更（テスト用）

```c
void machine_simulate_external_change(state_t new_state)
```

割り込みハンドラーや別のプロセスからの変更をシミュレートします。
`g_machine_state = new_state` と書いてください。

## volatile はなぜ必要か

### 例：without volatile

```c
state_t state = STATE_IDLE;

/* コンパイラの最適化 */
if (state == STATE_IDLE) {
  // ここに入ったから、この{ }内では state は STATE_IDLE だと確定
  // 外部からの変更があっても、コンパイラは知りません（最適化）
  while (state == STATE_IDLE) {
    /* 外部から state が変わっても、コンパイラは
     * 「state は STATE_IDLE で止まっている」と思い込む。
     * while ループを削除してしまうかもしれません。*/
  }
}
```

### 例：with volatile

```c
volatile state_t state = STATE_IDLE;

if (state == STATE_IDLE) {
  /* volatile があるので、
   * 毎回メモリから実際に state を読む。
   * 外部からの変更を確実に見る。*/
  while (state == STATE_IDLE) {
    /* 毎回メモリから読むので、外部からの変更に気づく */
  }
}
```

## 動かしてみる

```bash
./drill run c10
```

**未着手のうちはテストが失敗します。** すべてのテストが緑になればクリアです。

## つまずきポイント

1. **`volatile` はスレッドセーフではない** — データレース対策には `volatile` ではなく `std::atomic` や mutex を使ってください
2. **最適化の側面だけ** — `volatile` は「読み書きをしろ」と言うだけ。順序保証やアトミック性は提供しません
3. **単一スレッド前提** — この課題は単一スレッド上で「外部からの変更」（割り込みハンドラー等）をシミュレートしています
4. **初期化を忘れずに** — `machine_init()` を最初に呼んで `STATE_IDLE` で初期化してください

## テスト

| テスト | 見ているところ |
| --- | --- |
| `初期化と状態確認` | `machine_init` と状態確認関数 |
| `IDLE_から_RUNNING_に遷移` | `machine_start` |
| `RUNNING_から_STOPPED_に遷移` | `machine_stop` |
| `複数回の状態遷移` | 複数回の状態変更が正しく動作 |
| `外部から状態が変更されたことを検出できる` | **volatile の重要性** — 外部からの変更を正しく見る |
| `外部から複数回の変更を検出できる` | 複数回の外部変更 |
| `ポーリングループで状態を監視できる` | 典型的なポーリングパターン |
| `get_state_は毎回読み込みをしている` | volatile による毎回の読み込み保証 |

## 参考

- [10. volatile と割り込み安全性](../../docs/c/10_volatileと割り込み安全性.md)

## さらに学ぶ

- **並行処理** — `volatile` ではなく `std::atomic` や mutex を使ってください（C++ では `#include <atomic>`）
- **割り込み安全性** — RTOS で使われる。ハードウェア割り込みが発生しても安全に状態を共有する
- **メモリマップドハードウェア** — ハードウェアレジスタに volatile でアクセス
