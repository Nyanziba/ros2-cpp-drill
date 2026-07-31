#include "drill/counter.hpp"

int next_id()
{
  // static ローカル変数は最初の呼び出しで 1 度だけ初期化され、
  // 関数を抜けても値が残る。だから呼ぶたびに増える。
  static int id = 0;
  return ++id;
}
