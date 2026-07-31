#include "drill/timer.hpp"

int count_ticks(std::chrono::milliseconds budget, std::chrono::milliseconds period)
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(budget).count()
       / std::chrono::duration_cast<std::chrono::milliseconds>(period).count();
}

std::chrono::milliseconds seconds_to_ms(double seconds)
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::duration<double>(seconds));
}
