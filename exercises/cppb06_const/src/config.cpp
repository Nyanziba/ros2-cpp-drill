// I AM NOT DONE
//
// 定義側に末尾 const を 2 か所付けてください。ビルドが通れば OK です。

#include "drill/config.hpp"

Config::Config(const int & limit) : limit_(limit)
{
}

int Config::get_limit()
{
  return limit_;
}

const int * Config::ptr_to_limit()
{
  return &limit_;
}
