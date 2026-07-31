#include "drill/counter.hpp"

// (1) 関数内 static。初回の呼び出しでだけ 0 に初期化され、以降は残り続けます。
int next_id()
{
  static int id = 0;
  return ++id;
}

// (2) クラス static メンバの定義。ヘッダの宣言に対する実体は、ここ 1 か所だけです。
int IdGenerator::count_ = 0;

// (3) 非 static メンバ関数。インスタンスごとに呼ぶが、触る先は共有の count_。
int IdGenerator::id()
{
  return ++count_;
}

// (4) クラス static メンバ関数。this が無いので count_ に直接触ります。
int IdGenerator::get_count()
{
  return count_;
}

void IdGenerator::reset()
{
  count_ = 0;
}
