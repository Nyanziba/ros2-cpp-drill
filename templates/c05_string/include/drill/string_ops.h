/* このファイルは編集しません */

#ifndef DRILL_STRING_OPS_H
#define DRILL_STRING_OPS_H

#include <stddef.h>

/* C++ の gtest から C の関数を呼ぶための宣言。
 * これが無いと C++ 側が名前マングリングした記号を探してしまい、
 * undefined reference でリンクに失敗します。 */
#ifdef __cplusplus
extern "C" {
#endif

/* C 文字列 s の長さを返す（NUL 終端文字は含まない）。
 * s が NULL の場合は -1 を返す。
 * これは strlen() の実装。 */
int string_length(const char *s);

/* src を dest にコピーする。
 * dest のバッファサイズは max_len 以上必要。
 * NUL 終端文字必須。バッファをオーバーフロー防止。
 * 成功時は 0、エラー時は -1 を返す。
 * これは strncpy() より安全。 */
int string_copy(char *dest, const char *src, size_t max_len);

/* src を dest に追加（連結）する。
 * dest はすでに NUL 終端文字を含む文字列であると仮定。
 * dest のバッファサイズは max_len（NUL を含む）。
 * 連結後も NUL 終端文字必須。
 * 成功時は 0、エラー時は -1 を返す。
 * これは strncat() より安全。 */
int string_concat(char *dest, const char *src, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* DRILL_STRING_OPS_H */
