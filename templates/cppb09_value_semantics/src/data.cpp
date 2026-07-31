// I AM NOT DONE
//
// コピーコンストラクタと copy-assign を実装し、
// 関数引数で const& を使ってコピーを削減してください。

#include "drill/data.hpp"

int Data::copy_ctor_count = 0;
int Data::copy_assign_count = 0;

Data::Data(int value) : value_(value)
{
}

Data::Data(const Data & other) : value_(other.value_)
{
  // TODO: copy_ctor_count をインクリメント
}

Data & Data::operator=(const Data & other)
{
  // TODO: copy_assign_count をインクリメント
  if (this != &other) {
    value_ = other.value_;
  }
  return *this;
}

Data process_by_value(Data d)
{
  // TODO: Data を値で受け取る（コピー）と値で返す（別のコピー）
  return d;
}

Data process_by_const_ref(const Data & d)
{
  // TODO: const& で受け取ってコピーを避ける
  Data result(d.value() * 2);
  return result;
}
