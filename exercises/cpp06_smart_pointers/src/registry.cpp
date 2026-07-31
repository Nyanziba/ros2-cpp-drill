// I AM NOT DONE
//
// weak_ptr を使って Registry を実装してください。

#include "drill/registry.hpp"
#include <iostream>

void Registry::add(std::shared_ptr<Item> item)
{
  // TODO: item を weak_ptr に変換して items_ に追加してください。
}

void Registry::fire()
{
  // TODO: items_ に登録された Item をイテレートします。
  // expired() をチェックして、生きているものだけ std::cout に出力してください。
  // フォーマット: "Item { id=X, name=Y }"
}
