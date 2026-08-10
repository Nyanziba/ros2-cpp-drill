#include <stdlib.h>
#include "drill/malloc_free.h"

/* ===== 動的配列 ===== */

int * create_array(size_t size)
{
  return malloc(size * sizeof(int));
}

void free_array(int * arr)
{
  if (arr != NULL) {
    free(arr);
  }
}

/* ===== 連結リスト ===== */

struct Node * create_linked_list(size_t length)
{
  if (length == 0) {
    return NULL;
  }

  struct Node * head = malloc(sizeof(struct Node));
  if (head == NULL) {
    return NULL;
  }

  head->value = 0;
  head->next = NULL;
  struct Node * current = head;

  for (size_t i = 1; i < length; i++) {
    struct Node * next = malloc(sizeof(struct Node));
    if (next == NULL) {
      /* malloc 失敗時はここまで確保した分を解放する */
      free_linked_list(head);
      return NULL;
    }
    next->value = i;
    next->next = NULL;
    current->next = next;
    current = next;
  }

  return head;
}

void free_linked_list(struct Node * head)
{
  if (head == NULL) {
    return;
  }

  struct Node * next = head->next;
  free(head);
  free_linked_list(next);
}
