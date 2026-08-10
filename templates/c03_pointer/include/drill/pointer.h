/* このファイルは編集しません */

#ifndef DRILL_POINTER_H
#define DRILL_POINTER_H

/* C++ の gtest から C の関数を呼ぶための宣言。
 * これが無いと C++ 側が名前マングリングした記号を探してしまい、
 * undefined reference でリンクに失敗します。 */
#ifdef __cplusplus
extern "C" {
#endif

/* 2つの変数の値を入れ替える。
 * 成功時は 0、a または b が NULL の場合は -1 を返す。
 * ポインタを使ってアドレスで値を変更する。 */
int swap_values(int *a, int *b);

/* x を 2倍にして result に格納する。
 * 成功時は 0、result が NULL の場合は -1 を返す。
 * result はポインタ経由で呼び出し側に値を返す出力引数。 */
int multiply(int x, int *result);

/* ポインタが指す値を 3倍にする。
 * 成功時は 0、p が NULL の場合は -1 を返す。
 * ポインタの指す先を直接変更する。 */
int triple_pointer(int *p);

#ifdef __cplusplus
}
#endif

#endif /* DRILL_POINTER_H */
