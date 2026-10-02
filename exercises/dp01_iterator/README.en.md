# dp01 Iterator [Design Patterns]

Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 1. You implement both the GoF iterator and the STL iterator for the same `BookShelf`.

## What to do

Implement 4 things in `src/book_shelf.cpp`.

1. **`BookShelfIterator::has_next()`**
   - Returns `true` if there is a next book

2. **`BookShelfIterator::next()`**
   - Returns a **reference** to the book at the current position, and moves the position forward by one
   - If you move first and then return, the result is off by one book

3. **`BookShelf::iterator()`**
   - Returns `std::make_unique<BookShelfIterator>(*this)`
   - This corresponds to `return new BookShelfIterator(this);` in the Java version

4. **`BookShelf::begin()` / `BookShelf::end()`**
   - Return the iterators of the inner `std::vector<Book>` as they are
   - This alone makes range-based for and `<algorithm>` work

## Run it

```bash
./drill run dp01
```

## Common pitfalls

- `next()` returns `const Book &`. If it returns `Book`, a **copy happens every time**.
  The test "nextはコピーではなく本棚の中身を指す" (next points to the contents of the shelf, not a copy) compares addresses and fails it
- `iterator()` returns `std::unique_ptr<Iterator>`. With a raw pointer,
  the type does not say who `delete`s it
- The position (`index_`) is held by the **iterator side**. If you put it on the shelf side,
  two iterators used at the same time interfere with each other
- `BookShelfIterator` holds a reference to `BookShelf`.
  **It cannot outlive the shelf.** This is a constraint that Java does not have

## Tests

```bash
./drill run dp01
```

There are 7 tests. They check that the GoF version and the STL version return the same order,
and that `std::find_if` / `std::count_if` work.

## References

- [1. Iterator](../../docs-en/patterns/01_Iterator.md)
- [cppreference: range-based for loop](https://en.cppreference.com/w/cpp/language/range-for)
- [cppreference: std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
