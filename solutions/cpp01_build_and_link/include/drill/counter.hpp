#pragma once

/// 与えられた数を 1 増やして返す。
///
/// inline を付けたので、複数の翻訳単位に同じ定義があってもリンカが 1 つを選ぶ。
/// nm -C で見ると T（唯一の定義）から W（weak symbol）に変わっている。
inline int add_one(int x)
{
  return x + 1;
}

/// 呼び出すたびに 1, 2, 3, ... と増えていく値を返す。
int next_id();
