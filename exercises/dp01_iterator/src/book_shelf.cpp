// I AM NOT DONE
//
// 結城本 第1章 Iterator を C++ で書きます。
// GoF 版（has_next / next）と STL 版（begin / end）の両方を実装してください。

#include "drill/book_shelf.hpp"

namespace
{

/// GoF 版のイテレータの実装。
///
/// このクラスをヘッダに出していないのは、外から名前を知る必要が無いからです。
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
    // TODO: index_ が本棚の冊数に達していなければ true を返してください。
    return false;
  }

  const Book & next() override
  {
    // TODO: 現在位置の本への参照を返し、位置を 1 つ進めてください。
    //
    // 注意: 「進めてから返す」と 1 冊ずれます。返すのは進める前の位置の本です。
    //       shelf_.at(index_) の戻りは const Book & なので、
    //       ここで Book にコピーして返さないよう気をつけてください。
    return shelf_.at(index_);
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
  // TODO: BookShelfIterator を作って返してください。
  //
  // Java 版は new BookShelfIterator(this) を返すだけですが、
  // C++ では「誰が解放するか」を型で表明します。std::make_unique を使ってください。
  return nullptr;
}

BookShelf::const_iterator BookShelf::begin() const
{
  // TODO: 中の vector の先頭イテレータを返してください。
  return books_.end();
}

BookShelf::const_iterator BookShelf::end() const
{
  // TODO: 中の vector の終端イテレータを返してください。
  return books_.end();
}
