// このファイルは編集しません（インタフェースの提示）。
#pragma once

class Config
{
public:
  Config(const int & limit);

  int get_limit() const;

  const int * ptr_to_limit() const;

private:
  const int limit_;
};
