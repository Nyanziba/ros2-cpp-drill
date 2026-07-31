// I AM NOT DONE
//
// static の3つの意味を、それぞれ手を動かして確かめる課題です。

#include "drill/counter.hpp"

// TODO(1) 関数内 static。
// 初回の呼び出しでだけ初期化され、関数を抜けても値が残るようにしてください。
// 1 回目に 1、2 回目に 2、… を返します。
int next_id()
{
  return 0;
}

// TODO(2) クラス static メンバの「定義」。
// ヘッダにあるのは宣言だけなので、ここに実体を置かないと
// undefined reference でリンクに失敗します。下の行を有効にしてください。
// int IdGenerator::count_ = 0;

// TODO(3) 非 static メンバ関数だが、触る先は static メンバ。
// count_ を 1 つ進めて、その値を返してください。
// gen1 と gen2 で count_ が共有されることをテストが確認します。
int IdGenerator::id()
{
  return 0;
}

// TODO(4) クラス static メンバ関数。this が無いので count_ に直接触ります。
int IdGenerator::get_count()
{
  return 0;
}

void IdGenerator::reset()
{
  // TODO(5) count_ を 0 に戻してください。
  // next_id() の関数内 static はここからは戻せません（戻さなくて正解です）。
}
