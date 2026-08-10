# c08 手動メモリ管理 〔C言語編〕

`malloc` と `free` を使った動的メモリ管理の基本を学ぶ課題です。

## ヘッダは既に完成しています

`include/drill/malloc_free.h` は**編集しません**。
動的配列と連結リストのインタフェースが定義されています。

## やること

編集するのは `src/malloc_free.c` だけです。以下の関数を実装してください：

### 動的配列
- `create_array(size)` — `int` 配列を `malloc` で確保する
- `free_array(arr)` — 配列を `free` で解放する

### 連結リスト
- `create_linked_list(length)` — 長さ `length` の連結リストを作成
  - 各ノードの `value` は 0, 1, 2, ..., length-1
  - `malloc` 失敗時は `NULL` を返す
- `free_linked_list(head)` — リストを再帰的に解放

## 動かしてみる

```bash
./drill run c08
```

## つまずきポイント

- **malloc 失敗処理** — `malloc` が `NULL` を返すことがあります。チェックが必須です
- **use-after-free** — `create_linked_list` で `malloc` 失敗時、既に確保したノードはリークしない（`free_linked_list` で解放）
- **再帰 free** — `free_linked_list` は再帰的ですが、`next` を**先に保存**してから `free(head)` することが重要です
  - 誤った実装：`free(head); free_linked_list(head->next);` → use-after-free
  - 正しい実装：`struct Node * next = head->next; free(head); free_linked_list(next);`

## AddressSanitizer（ASAN）

このテストには `-fsanitize=address` が有効です。

- **テストは通ったが、ドリルが赤い** ← メモリリークしている
  - すべてのノードを `free` しているか確認してください
- **リークが出ている** ← リークサニタイザーの出力を見て原因を調査

## テスト

| テスト | 見ているところ |
| --- | --- |
| `配列を確保できる` | `malloc` で配列を確保し、アクセスできるか |
| `異なるサイズで動く` | 複数の配列を独立して管理できるか |
| `リストを作成できる` | `malloc` で連結リストを作成できるか |
| `複数のリストを独立して管理` | メモリが混在していないか |

## 参考

- [8. 手動メモリ管理](../../docs/c/08_手動メモリ管理.md)
