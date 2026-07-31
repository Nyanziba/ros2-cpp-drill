// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <optional>
#include <map>
#include <string>

/// std::clamp を使って速度を制限する。
double clamp_velocity(double value, double min_val, double max_val);

/// std::optional を使って map から値を探す。見つからない場合は nullopt を返す。
std::optional<int> find_user_id(const std::map<std::string, int> & users, const std::string & name);
