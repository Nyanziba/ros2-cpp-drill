// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "drill/book_shelf.hpp"
#include "drill_i18n.hpp"
#include "drill_i18n.hpp"

namespace
{

BookShelf make_shelf()
{
  BookShelf shelf;
  shelf.append(Book{"Design Patterns"});
  shelf.append(Book{"Refactoring"});
  shelf.append(Book{"Effective C++"});
  return shelf;
}

}  // namespace

TEST(IteratorTest, GofIteratorTraversesInOrder)
{
  const BookShelf shelf = make_shelf();

  auto it = shelf.iterator();
  ASSERT_NE(it, nullptr) << drill::localized("iterator() が nullptr を返しています", "iterator() returned nullptr");

  std::vector<std::string> names;
  while (it->has_next()) {
    names.push_back(it->next().name());
  }

  const std::vector<std::string> expected = {
    "Design Patterns", "Refactoring", "Effective C++"};
  EXPECT_EQ(names, expected);
}

TEST(IteratorTest, EmptyShelfHasNoNextFromStart)
{
  const BookShelf shelf;

  auto it = shelf.iterator();
  ASSERT_NE(it, nullptr);
  EXPECT_FALSE(it->has_next());
}

TEST(IteratorTest, TwoIteratorsDoNotInterfere)
{
  const BookShelf shelf = make_shelf();

  auto first = shelf.iterator();
  auto second = shelf.iterator();
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);

  // first だけ 2 つ進める。位置をイテレータ側が持っていれば second は先頭のまま。
  first->next();
  first->next();

  EXPECT_EQ(second->next().name(), "Design Patterns");
  EXPECT_EQ(first->next().name(), "Effective C++");
}

TEST(IteratorTest, NextReturnsReferenceNotCopy)
{
  const BookShelf shelf = make_shelf();

  auto it = shelf.iterator();
  ASSERT_NE(it, nullptr);

  const Book & from_iterator = it->next();
  const Book & from_shelf = shelf.at(0);

  // 参照を返せていれば、同じオブジェクトを指しているはず。
  EXPECT_EQ(&from_iterator, &from_shelf)
    << drill::localized("next() が Book をコピーして返しています。const Book & を返してください", "next() returns a copy of Book. Return const Book &");
}

TEST(IteratorTest, StlIteratorWorksWithRangeBasedFor)
{
  const BookShelf shelf = make_shelf();

  std::vector<std::string> names;
  for (const Book & book : shelf) {
    names.push_back(book.name());
  }

  const std::vector<std::string> expected = {
    "Design Patterns", "Refactoring", "Effective C++"};
  EXPECT_EQ(names, expected);
}

TEST(IteratorTest, StlIteratorWorksWithAlgorithms)
{
  const BookShelf shelf = make_shelf();

  const auto found = std::find_if(
    shelf.begin(), shelf.end(),
    [](const Book & book) { return book.name() == "Refactoring"; });

  ASSERT_NE(found, shelf.end());
  EXPECT_EQ(found->name(), "Refactoring");

  const auto count = std::count_if(
    shelf.begin(), shelf.end(),
    [](const Book & book) { return book.name().size() > 11; });
  EXPECT_EQ(count, 2);

  EXPECT_EQ(std::distance(shelf.begin(), shelf.end()), 3);
}

TEST(IteratorTest, GofAndStlIteratorsReturnSameOrder)
{
  const BookShelf shelf = make_shelf();

  std::vector<std::string> gof;
  auto it = shelf.iterator();
  ASSERT_NE(it, nullptr);
  while (it->has_next()) {
    gof.push_back(it->next().name());
  }

  std::vector<std::string> stl;
  for (const Book & book : shelf) {
    stl.push_back(book.name());
  }

  EXPECT_EQ(gof, stl);
}
