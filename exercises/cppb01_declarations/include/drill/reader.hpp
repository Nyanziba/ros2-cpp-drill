// このファイルは編集しません（インタフェースの提示）。
#pragma once

// const int * と int * の違いを読み分ける課題です。
// 宣言は右から左に読みます。const int * p は「const int へのポインタ」。

// 読むだけ。指す先を書き換えてはいけません。
int read_value(const int * p);

// 書き込みます。指す先が const でないので書けます。
void modify_value(int * p, int new_val);

// 受け取ったポインタをそのまま返します。戻り値も const int * です。
const int * get_constant_ptr(const int * p);
