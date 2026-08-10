/* このファイルは編集しません（インタフェースの提示）。 */

#ifndef DRILL_MALLOC_FREE_H
#define DRILL_MALLOC_FREE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===== 動的配列 ===== */

/* int の配列を malloc で確保する */
int * create_array(size_t size);

/* 配列を free で解放する */
void free_array(int * arr);

/* ===== 連結リスト ===== */

/* 連結リストのノード */
struct Node {
  int value;
  struct Node * next;
};

/* 長さ length の連結リストを作成する
 * 例：create_linked_list(3) → Node(0) -> Node(1) -> Node(2) -> NULL
 * Node の value には 0 から length-1 までの値が入ります。
 */
struct Node * create_linked_list(size_t length);

/* 連結リストを再帰的に解放する */
void free_linked_list(struct Node * head);

#ifdef __cplusplus
}
#endif

#endif /* DRILL_MALLOC_FREE_H */
