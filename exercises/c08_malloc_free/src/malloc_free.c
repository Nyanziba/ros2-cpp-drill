/* I AM NOT DONE
 *
 * 手動メモリ管理の課題です。
 * malloc と free を使って、動的にメモリを確保・解放してください。
 *
 * ビルドされているコンパイル オプションに -fsanitize=address が含まれています。
 * free を忘れると AddressSanitizer が「Direct leak」として検出し、
 * プロセスの終了コードが非ゼロになります。テストが通っていても、
 * リークを放置しているとドリルは赤と判定されます。
 */

#include <stdlib.h>
#include "drill/malloc_free.h"

/* ===== 動的配列 ===== */

int * create_array(size_t size)
{
  /* TODO: int の配列を malloc で size 個分確保してください。
   * メモリ確保に失敗した場合（malloc が NULL を返した場合）は
   * NULL を返してください。
   *
   * return malloc(size * sizeof(int)); */
  (void)size;
  return NULL;
}

void free_array(int * arr)
{
  /* TODO: arr を free してください。
   * NULL チェックも必要です（NULL に free を呼ぶのは安全ですが、
   * コードを明確にするため明示的にチェックしてください）。 */
  (void)arr;
}

/* ===== 連結リスト ===== */

struct Node * create_linked_list(size_t length)
{
  /* TODO: length 個のノードを持つ連結リストを malloc で作成してください。
   * 各ノードの value には 0, 1, 2, ... length-1 が入ります。
   * 最後のノードの next は NULL です。
   *
   * 例：create_linked_list(3)
   *   Node(value=0, next=...) -> Node(value=1, next=...) -> Node(value=2, next=NULL)
   *
   * malloc に失敗した場合はどの時点でも NULL を返してください。
   */
  (void)length;
  return NULL;
}

void free_linked_list(struct Node * head)
{
  /* TODO: 連結リストを再帰的に解放してください。
   *
   * 注意：next を保存してから head を free しないと、
   * free 後に next にアクセスすることになり、use-after-free になります。
   *
   * struct Node * next = head->next;
   * free(head);
   * free_linked_list(next);
   */
  (void)head;
}
