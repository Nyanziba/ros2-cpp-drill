// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <memory>
#include <string>
#include <vector>

struct Item
{
  int id;
  std::string name;

  Item(int id, const std::string & name) : id(id), name(name) {}
};

class Registry
{
public:
  void add(std::shared_ptr<Item> item);
  void fire();

private:
  std::vector<std::weak_ptr<Item>> items_;
};
