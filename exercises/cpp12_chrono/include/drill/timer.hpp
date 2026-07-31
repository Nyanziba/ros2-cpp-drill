// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <chrono>

/// std::chrono を使用した時間計測。

/// milliseconds の予算から、指定された period でいくつのティックが取れるかを返す。
/// 例: 1000ms の予算を 100ms period で → 10 ticks
int count_ticks(std::chrono::milliseconds budget, std::chrono::milliseconds period);

/// 秒（double）を std::chrono::milliseconds に変換する。
/// 例: 1.5 秒 → 1500ms
std::chrono::milliseconds seconds_to_ms(double seconds);
