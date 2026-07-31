#include "drill/copycounter.hpp"
#include <cctype>

std::string CopyCounter::copy_and_uppercase(const std::string & text)
{
  std::string result;
  for (char c : text) {
    result += std::toupper(c);
  }
  return result;
}

std::string CopyCounter::get_description() const
{
  return "コピー回数: " + std::to_string(copies);
}
