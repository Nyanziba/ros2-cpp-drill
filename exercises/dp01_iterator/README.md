# dp01 Iterator 〔デザインパターン編〕

結城本 第1章。GoF 版のイテレータと STL 版のイテレータを、同じ `BookShelf` に対して両方実装します。

## やること

`src/book_shelf.cpp` に 4 つ実装してください。

1. **`BookShelfIterator::has_next()`**
   - まだ次の本があれば `true`

2. **`BookShelfIterator::next()`**
   - 現在位置の本への**参照**を返し、位置を 1 つ進める
   - 進めてから返すと 1 冊ずれます

3. **`BookShelf::iterator()`**
   - `std::make_unique<BookShelfIterator>(*this)` を返す
   - Java 版の `return new BookShelfIterator(this);` に対応

4. **`BookShelf::begin()` / `BookShelf::end()`**
   - 中の `std::vector<Book>` のイテレータをそのまま返す
   - これだけで range-based for と `<algorithm>` が使えるようになります

## 動かしてみる

```bash
./drill run dp01
```

## つまずきポイント

- `next()` は `const Book &` を返します。`Book` を返すと**毎回コピー**が走ります。
  テスト「nextはコピーではなく本棚の中身を指す」がアドレスを比較して落とします
- `iterator()` は `std::unique_ptr<Iterator>` を返します。生ポインタだと
  誰が `delete` するかが型に書かれません
- 位置（`index_`）は**イテレータ側**が持ちます。本棚側に持たせると、
  2 つのイテレータを同時に使ったとき互いに干渉します
- `BookShelfIterator` は `BookShelf` への参照を持っています。
  **本棚より長生きさせられません**。Java には無い制約です

## テスト

```bash
./drill run dp01
```

7 つのテストがあります。GoF 版と STL 版が同じ順番を返すこと、
`std::find_if` / `std::count_if` が動くことまで見ます。

## 参考

- [1. Iterator](../../docs/patterns/01_Iterator.md)
- [cppreference: range-based for loop](https://en.cppreference.com/w/cpp/language/range-for)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
