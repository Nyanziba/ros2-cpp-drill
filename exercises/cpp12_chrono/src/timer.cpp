// I AM NOT DONE
//
// std::chrono を使って時間計測の関数を実装してください。

#include "drill/timer.hpp"

int count_ticks(std::chrono::milliseconds budget, std::chrono::milliseconds period)
{
  // TODO: budget を period で割り、いくつのティックが取れるかを返してください。
  // duration_cast<T>(d) でキャストします。
  return 0;
}

std::chrono::milliseconds seconds_to_ms(double seconds)
{
  // TODO: 秒を milliseconds に変換してください。
  // 例えば 1.5秒 なら 1500ms になります。
  // duration_cast<std::chrono::milliseconds>() を使いましょう。
  return std::chrono::milliseconds(0);
}
