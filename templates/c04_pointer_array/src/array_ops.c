/* I AM NOT DONE
 *
 * 配列とポインタ演算の課題です。
 * 配列の要素数は引数で受け取り、確実に範囲内でアクセスする必要があります。
 * このファイルを ASAN（Address Sanitizer）の下で実行すると、
 * 範囲外アクセスがあれば即座に検出されます。
 */

#include "drill/array_ops.h"

/* TODO: 配列 arr の先頭から len 個の要素の合計を返す。
 * len が 0 の場合は 0 を返す。
 *
 * ヒント: ポインタを使ってループし、arr[i] または *(arr + i) でアクセスできます。
 * 範囲外アクセスをしないよう気をつけてください。 */
int sum_array(const int *arr, size_t len)
{
  (void)arr;
  (void)len;
  return 0;
}

/* TODO: 配列 arr の先頭から len 個の要素の最大値を返す。
 * len が 0 の場合は INT_MIN を返す。
 *
 * ヒント: 最初の要素を max として、以降をループで比較します。 */
int max_element(const int *arr, size_t len)
{
  (void)arr;
  (void)len;
  return 0;
}

/* TODO: 配列 arr の先頭から len 個の要素をそれぞれ 2倍にする。
 * len が 0 の場合は何もしない。
 *
 * ヒント: arr[i] *= 2; またはポインタで歩きながら *p *= 2; でできます。 */
void double_elements(int *arr, size_t len)
{
  (void)arr;
  (void)len;
}
