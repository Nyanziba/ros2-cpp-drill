// 解答例
//
// 結城本 第1章 Iterator を C++ で書いたもの。
// GoF 版（has_next / next）と STL 版（begin / end）の両方を用意しています。

#include "drill/book_shelf.hpp"

namespace
{

/// GoF 版のイテレータの実装。
///
/// ヘッダに出していないのは、外から名前を知る必要が無いからです。
/// 呼び出し側は Iterator という抽象しか見ません。
class BookShelfIterator : public Iterator
{
public:
  explicit BookShelfIterator(const BookShelf & shelf)
  : shelf_(shelf)
  {
  }

  bool has_next() const override
  {
    return index_ < shelf_.size();
  }

  const Book & next() override
  {
    // 「返してから進める」。進めてから返すと 1 冊ずれます。
    // 戻り型が const Book & なので、ここでコピーは起きません。
    const Book & book = shelf_.at(index_);
    ++index_;
    return book;
  }

private:
  // 参照で持っています。つまり BookShelf より長生きできません。
  // ヘッダのコメントにある「寿命の約束」がこれです。
  const BookShelf & shelf_;
  std::size_t index_ = 0;
};

}  // namespace

void BookShelf::append(Book book)
{
  books_.push_back(std::move(book));
}

std::size_t BookShelf::size() const
{
  return books_.size();
}

const Book & BookShelf::at(std::size_t index) const
{
  return books_[index];
}

std::unique_ptr<Iterator> BookShelf::iterator() const
{
  // Java 版は new BookShelfIterator(this) を返すだけですが、
  // C++ では「受け取った人が所有する」ことを unique_ptr で型に書きます。
  return std::make_unique<BookShelfIterator>(*this);
}

BookShelf::const_iterator BookShelf::begin() const
{
  return books_.begin();
}

BookShelf::const_iterator BookShelf::end() const
{
  return books_.end();
}
