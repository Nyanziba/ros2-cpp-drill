#include "drill/registry.hpp"
#include <iostream>

void Registry::add(std::shared_ptr<Item> item)
{
  items_.push_back(item);
}

void Registry::fire()
{
  for (auto & weak_item : items_) {
    auto item = weak_item.lock();
    if (item) {
      std::cout << "Item { id=" << item->id << ", name=" << item->name << " }" << std::endl;
    }
  }
}
