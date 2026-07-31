#include "drill/limiter.hpp"
#include <algorithm>

double clamp_velocity(double value, double min_val, double max_val)
{
  return std::clamp(value, min_val, max_val);
}

std::optional<int> find_user_id(const std::map<std::string, int> & users, const std::string & name)
{
  auto it = users.find(name);
  if (it != users.end()) {
    return it->second;
  }
  return std::nullopt;
}
