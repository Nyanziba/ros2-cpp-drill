#include "drill/string_ops.h"

int string_length(const char *s)
{
  /* NULL チェック */
  if (s == NULL) {
    return -1;
  }

  int len = 0;
  while (s[len] != '\0') {
    len++;
  }

  return len;
}

int string_copy(char *dest, const char *src, size_t max_len)
{
  /* NULL チェックと max_len チェック */
  if (dest == NULL || src == NULL || max_len == 0) {
    return -1;
  }

  /* 最大 (max_len - 1) バイトをコピーして NUL 終端 */
  size_t i;
  for (i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }

  /* NUL 終端を追加 */
  dest[i] = '\0';

  return 0;
}

int string_concat(char *dest, const char *src, size_t max_len)
{
  /* NULL チェックと max_len チェック */
  if (dest == NULL || src == NULL || max_len == 0) {
    return -1;
  }

  /* dest の長さを調べる */
  size_t dest_len = 0;
  while (dest[dest_len] != '\0' && dest_len < max_len) {
    dest_len++;
  }

  /* コピー可能な文字数（NUL 終端用に 1 バイト確保） */
  size_t remaining = max_len - dest_len - 1;

  if (remaining <= 0) {
    /* dest が既に max_len に達している、またはスペースが無い */
    return -1;
  }

  /* src をコピー。残り文字数 - 1 まで（NUL 用に 1 バイト確保） */
  size_t i;
  for (i = 0; i < remaining - 1 && src[i] != '\0'; i++) {
    dest[dest_len + i] = src[i];
  }

  /* NUL 終端を追加 */
  dest[dest_len + i] = '\0';

  return 0;
}
