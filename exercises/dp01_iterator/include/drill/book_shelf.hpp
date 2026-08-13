// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

/// 本。名前しか持たない。
class Book
{
public:
  explicit Book(std::string name)
  : name_(std::move(name))
  {
  }

  const std::string & name() const { return name_; }

private:
  std::string name_;
};

/// GoF 版のイテレータ（結城本 第1章の Iterator インタフェースに対応）。
///
/// Java 版との違い:
///   - 仮想デストラクタが必須。無いと unique_ptr で解放したとき派生の
///     デストラクタが呼ばれません（未定義動作）。
///   - next() の戻りは Object ではなく const Book &。
///     値で返すとコレクションの中身を毎回コピーします。
class Iterator
{
public:
  virtual ~Iterator() = default;

  /// まだ次の要素があるか。位置は変えないので const。
  virtual bool has_next() const = 0;

  /// 現在の要素を返し、位置を 1 つ進める。
  virtual const Book & next() = 0;
};

/// 本棚。GoF 版と STL 版、2 通りの走査手段を提供します。
///
/// 【寿命の約束】
/// iterator() が返すイテレータも begin()/end() が返すイテレータも、
/// この BookShelf より長生きさせてはいけません。
/// append() を呼ぶと、それ以前に取得したイテレータはすべて無効になります
/// （std::vector と同じ規約です）。
class BookShelf
{
public:
  /// 本を末尾に追加する。
  void append(Book book);

  /// 何冊あるか。
  std::size_t size() const;

  /// index 番目の本。範囲外は呼ばない前提。
  const Book & at(std::size_t index) const;

  /// GoF 版のイテレータを作る。
  /// 所有権は呼び出し側に移ります（だから unique_ptr を返します）。
  std::unique_ptr<Iterator> iterator() const;

  /// STL 版。この 2 つがあるだけで range-based for と <algorithm> が使えます。
  using const_iterator = std::vector<Book>::const_iterator;
  const_iterator begin() const;
  const_iterator end() const;

private:
  std::vector<Book> books_;
};
