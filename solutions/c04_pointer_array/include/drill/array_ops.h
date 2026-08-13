/* このファイルは編集しません */

#ifndef DRILL_ARRAY_OPS_H
#define DRILL_ARRAY_OPS_H

#include <stddef.h>
#include <limits.h>

/* C++ の gtest から C の関数を呼ぶための宣言。
 * これが無いと C++ 側が名前マングリングした記号を探してしまい、
 * undefined reference でリンクに失敗します。 */
#ifdef __cplusplus
extern "C" {
#endif

/* 配列の全要素の合計を計算する。
 * len が 0 の場合は 0 を返す。
 * 出力引数を使わず、戻り値で直接返す。 */
int sum_array(const int *arr, size_t len);

/* 配列の最大値を返す。
 * len が 0 の場合は INT_MIN を返す。
 * const int * を使って読み取り専用アクセスを指定。 */
int max_element(const int *arr, size_t len);

/* 配列の全要素を 2倍にする。
 * 書き込みが必要なので const 修飾子は不可。
 * len が 0 の場合は何もしない。 */
void double_elements(int *arr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* DRILL_ARRAY_OPS_H */
