// このファイルは編集しません（インタフェースの提示）。
#pragma once

// 関数内 static。プログラム全体で 1 つしかなく、外からリセットはできません。
int next_id();

class IdGenerator
{
public:
  // クラス static メンバ関数。this が無いので、インスタンス無しで呼べます。
  static int get_count();
  static void reset();

  // 定義は src/counter.cpp に書きます。
  // 「非 static メンバ関数だが、触る先は static メンバ」という形です。
  int id();

private:
  // クラス static メンバ。ここは宣言だけなので、.cpp に定義が必要です。
  static int count_;
};
