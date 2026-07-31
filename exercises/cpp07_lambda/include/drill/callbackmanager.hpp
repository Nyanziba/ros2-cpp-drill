// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <functional>
#include <vector>

class CallbackManager
{
public:
  using Callback = std::function<void(int)>;

  void register_callback(Callback cb);
  void fire(int value);

private:
  std::vector<Callback> callbacks_;
};
