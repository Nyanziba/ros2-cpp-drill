// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <string>

extern std::string g_trace_log;

class Tracer
{
public:
  explicit Tracer(const std::string & name);
  ~Tracer();
  
private:
  std::string name_;
};

void trace_something();
