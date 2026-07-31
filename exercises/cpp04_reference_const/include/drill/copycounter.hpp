// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <string>

/// コピー回数を数える構造体。
struct CopyCounter
{
  int copies = 0;

  std::string copy_and_uppercase(const std::string & text);

  std::string get_description() const;
};
