// このファイルは編集しません（インタフェースの提示）。
#pragma once

class Data
{
public:
  static int copy_ctor_count;
  static int copy_assign_count;
  
  explicit Data(int value = 0);
  Data(const Data & other);
  Data & operator=(const Data & other);
  
  int value() const { return value_; }
  static void reset() { copy_ctor_count = copy_assign_count = 0; }
  
private:
  int value_;
};

Data process_by_value(Data d);
Data process_by_const_ref(const Data & d);
