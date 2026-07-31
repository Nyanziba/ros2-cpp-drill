#include "drill/data.hpp"

int Data::copy_ctor_count = 0;
int Data::copy_assign_count = 0;

Data::Data(int value) : value_(value)
{
}

Data::Data(const Data & other) : value_(other.value_)
{
  ++copy_ctor_count;
}

Data & Data::operator=(const Data & other)
{
  ++copy_assign_count;
  if (this != &other) {
    value_ = other.value_;
  }
  return *this;
}

Data process_by_value(Data d)
{
  return d;
}

Data process_by_const_ref(const Data & d)
{
  Data result(d.value() * 2);
  return result;
}
