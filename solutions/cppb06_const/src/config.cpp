#include "drill/config.hpp"

Config::Config(const int & limit) : limit_(limit)
{
}

int Config::get_limit() const
{
  return limit_;
}

const int * Config::ptr_to_limit() const
{
  return &limit_;
}
