#include "drill/callbackmanager.hpp"

void CallbackManager::register_callback(Callback cb)
{
  callbacks_.push_back(cb);
}

void CallbackManager::fire(int value)
{
  for (auto & cb : callbacks_) {
    cb(value);
  }
}
