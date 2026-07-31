#include "drill/stopwatch.hpp"

Stopwatch::Stopwatch(int max_time_ms)
  : max_time_(max_time_ms), elapsed_(0)
{
}

void Stopwatch::advance(int ms)
{
  elapsed_ += ms;
}

int Stopwatch::elapsed() const
{
  return elapsed_;
}

int Stopwatch::max_time() const
{
  return max_time_;
}
