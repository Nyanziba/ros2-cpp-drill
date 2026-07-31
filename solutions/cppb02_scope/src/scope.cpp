#include "drill/scope.hpp"

std::string g_trace_log;

Tracer::Tracer(const std::string & name) : name_(name)
{
  g_trace_log += name_ + ":construct:";
}

Tracer::~Tracer()
{
  g_trace_log += name_ + ":destruct:";
}

void trace_something()
{
  Tracer a("outer");
  {
    Tracer b("inner");
  }
}
