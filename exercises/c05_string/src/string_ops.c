/* I AM NOT DONE
 *
 * C 言語の文字列処理の課題です。
 * 文字列は '\0'（NUL 文字）で終端され、長さに含みません。
 * バッファサイズとの関係に注意してください。
 */

#include "drill/string_ops.h"

/* TODO: C 文字列 s の長さを返す。NUL 文字は含まない。
 * s が NULL の場合は -1 を返す。
 *
 * 例: "hello" -> 5
 *     "" -> 0
 *
 * ヒント: while (*s != '\0') { ... } で走査できます。
 * または配列のインデックスを使ってカウントします。 */
int string_length(const char *s)
{
  (void)s;
  return 0;
}

/* TODO: src を dest にコピーする。
 * dest のバッファサイズは max_len バイト。
 * コピー後、必ず NUL 終端文字 '\0' を付加。
 * src が NULL または dest が NULL なら -1。
 * max_len が 0 なら -1（何も書き込めない）。
 * 成功時は 0。
 *
 * 例: string_copy(buf, "hi", 3) -> buf[0]='h', buf[1]='i', buf[2]='\0'
 *     src が max_len より長い場合は切り詰めて NUL 終端。 */
int string_copy(char *dest, const char *src, size_t max_len)
{
  (void)dest;
  (void)src;
  (void)max_len;
  return 0;
}

/* TODO: src を dest に追加（連結）する。
 * dest はすでに NUL 終端された文字列。
 * dest のバッファサイズは max_len バイト（NUL を含む）。
 * 連結後も必ず NUL 終端文字。
 * dest または src が NULL なら -1。
 * max_len が 0 なら -1。
 * 成功時は 0。
 *
 * ヒント: 最初に dest の長さを調べ、その後に src をコピーします。
 * バッファをオーバーフロー防止。 */
int string_concat(char *dest, const char *src, size_t max_len)
{
  (void)dest;
  (void)src;
  (void)max_len;
  return 0;
}
