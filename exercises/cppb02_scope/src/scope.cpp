// I AM NOT DONE
//
// Tracer クラスを実装し、trace_something() 内で正しく使ってください。

#include "drill/scope.hpp"

std::string g_trace_log;

Tracer::Tracer(const std::string & name) : name_(name)
{
  // TODO: construct で log に "name:construct:" を追加
}

Tracer::~Tracer()
{
  // TODO: destruct で log に "name:destruct:" を追加
}

void trace_something()
{
  // TODO: Tracer オブジェクトを作成してスコープを出すことで、
  // 逆順に破棄されることを確認
  Tracer a("outer");
  {
    Tracer b("inner");
  }
}
