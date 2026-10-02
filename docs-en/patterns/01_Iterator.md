# 1. Iterator

> **Corresponds to Yuki's book, Chapter 1.** Please open `BookShelf` and `BookShelfIterator` next to you.
>
> **Goal of this chapter**: The Java `Iterator` has two methods, `hasNext()` and `next()`.
> The iterator of C++ `std::vector` uses `operator++`, `operator*`, and `operator!=`.
> **Why is the form different?** You first implement the GoF version yourself, then the STL version,
> and walk through the same `BookShelf` with both methods.
> The difference does not come from taste. It comes from a design decision: **how to represent the end**.

## 1.1 If you write the Java version in C++ as is

The `Iterator` interface in Yuki's book looks like this.

```java
public interface Iterator {
    public abstract boolean hasNext();
    public abstract Object next();
}
```

If you move it to C++ in the plain way, you get this.

```cpp
class Iterator
{
public:
  virtual ~Iterator() = default;
  virtual bool has_next() const = 0;
  virtual const Book & next() = 0;
};
```

There are three changes from the Java version. **If you do not make them, you get bugs.**

### Change 1: Added `virtual ~Iterator() = default;`

Java does not need it, but in C++ it is **required**.

```cpp
std::unique_ptr<Iterator> it = shelf.iterator();
// When it is destroyed, only the destructor of Iterator is called
// → the members held by BookShelfIterator are not released
```

If you `delete` through a base class pointer and the destructor is not virtual,
**the destructor of the derived class is not called**. This is undefined behavior.

**Rule**: If you write even one pure virtual function, also write a virtual destructor.
We follow this rule in all 23 chapters of this course.

### Change 2: Changed `Object` to `const Book &`

The Java version returns `Object` and casts it back. C++ has no such culture.
You use a template, or return a concrete type.
**In this chapter we write it with a concrete type.** We do templates in 1.5.

We used `&` (reference) **to avoid copying `Book`**.
In Java, returning a reference is the normal thing, but if you write `Book next()` in C++, **a copy happens every time**.

```cpp
virtual Book next();              // copies Book and returns it
virtual const Book & next();      // points directly to the Book in the shelf. No copy
```

This is the biggest difference from Java. **If you do not write it, it becomes a copy.**

### Change 3: Added `const` to `has_next()`

`has_next()` does not change the state, so we make it a `const` member function.
`next()` moves the position, so we do not add `const`.
**Java has no such distinction**, so people forget to write it.

## 1.2 Who owns the Iterator?

The Java version of `iterator()` looks like this.

```java
public Iterator iterator() {
    return new BookShelfIterator(this);
}
```

It just does `new` and returns. The GC cleans up. If you write the same thing in C++,

```cpp
Iterator * iterator() const;      // does the caller delete it? Nobody knows unless it is written
```

**The type does not say who `delete`s it.** In C++, this is a design flaw.
Return a `unique_ptr`.

```cpp
std::unique_ptr<Iterator> iterator() const;
```

Now the type says: "**the receiver owns it. It disappears by itself when it leaves the scope**".
The caller looks like this.

```cpp
auto it = shelf.iterator();       // std::unique_ptr<Iterator>
while (it->has_next()) {
  std::cout << it->next().name() << "\n";
}
// released automatically here
```

**We make the same decision in many of the 23 chapters.** Remember this:
"where Java does `new` and returns, C++ returns `std::unique_ptr`."

## 1.3 A danger that Java does not have: the iterator outlives the bookshelf

This is a pitfall specific to C++.

```cpp
std::unique_ptr<Iterator> make_iterator()
{
  BookShelf shelf;                 // local variable
  shelf.append(Book{"Design Patterns"});
  return shelf.iterator();         // shelf dies here
}                                  // the returned iterator points to a dead bookshelf

auto it = make_iterator();
it->has_next();                    // undefined behavior
```

`BookShelfIterator` holds a pointer or a reference to `BookShelf`.
**In Java, the GC keeps `shelf` alive, so it does not crash. In C++, it crashes.**

This is not a defect of the Iterator pattern. It is **a constraint that always comes with
making a type that holds a reference to an object in C++**. The same problem appears in chapter 12 (Decorator),
chapter 14 (Chain of Responsibility), and chapter 17 (Observer).

There are **three ways to deal with it**.

| Method | How to write it | When to use it |
| --- | --- | --- |
| Promise the lifetime (convention) | Write a comment: "do not let the iterator outlive the bookshelf" | This is what the STL does. Zero cost |
| Shared ownership with `shared_ptr` | Hold a `shared_ptr<const BookShelf>` | When you cannot predict the lifetime |
| Hold a copy | The iterator copies a `vector<Book>` | Only when it is small |

**The STL chooses number 1.** A `std::vector::iterator` becomes invalid when the vector dies.
This is the story called "iterator invalidation".
In this exercise we also use number 1. **Writing it in a comment** is your job.

## 1.4 The STL version: why is there no `hasNext()`?

In C++, you write a traversal like this.

```cpp
for (auto it = shelf.begin(); it != shelf.end(); ++it) {
  std::cout << it->name() << "\n";
}
```

There is no `hasNext()` anywhere. Instead there is `it != shelf.end()`.
This is a design where **the iterator does not "answer" whether it is at the end, but "is compared with" an iterator that represents the end**.

What does this difference produce?

| | GoF version (`hasNext`) | STL version (`begin` / `end`) |
| --- | --- | --- |
| How the end is represented | The iterator itself knows | A separate iterator that represents the end |
| Partial ranges | Cannot be expressed | You can write "from `it2` to `it5`" |
| Sharing algorithms | Not possible | `std::find`, `std::count_if`, and `std::sort` all work |
| Virtual function calls | Two every time (`hasNext` and `next`) | Zero (inlined) |

**The third row is the main point.** The moment you provide `begin()` / `end()`,
**more than 100 functions in `<algorithm>` become available.**

```cpp
auto found = std::find_if(shelf.begin(), shelf.end(),
                          [](const Book & b) { return b.name() == "Refactoring"; });

int n = std::count_if(shelf.begin(), shelf.end(),
                      [](const Book & b) { return b.name().size() > 10; });
```

With the GoF `Iterator`, you cannot use any of these. You write the while loop yourself.

Also, if you have `begin()` / `end()`, the **range-based for** works.

```cpp
for (const Book & book : shelf) {
  std::cout << book.name() << "\n";
}
```

The compiler just looks for `begin()` / `end()` and expands it into the loop above.
**It is not a special feature. It is a naming convention.**

### The fourth row: the cost of virtual functions

In the GoF version, `has_next()` and `next()` are virtual functions,
so **two virtual calls** happen per element. They are not inlined either.

The iterator of the STL version is (in this exercise) exactly `std::vector<Book>::const_iterator`,
so it is **effectively a pointer**. The call cost is zero.

If you loop over thousands of elements on a microcontroller, you can measure this difference.

## 1.5 We do not make it a template (in this chapter)

You probably thought: "Should it be a template instead of fixing it to `Book`?" That is correct, but
**we do not do it in this chapter.** There are two reasons.

1. The main point of chapter 1 of Yuki's book is "separate the traversal from the contents", not generalization
2. To write an iterator correctly with C++ templates, you need definitions such as `iterator_traits`,
   `value_type`, and `difference_type`. **That is not part of the Iterator pattern**

How to write an STL-compatible iterator from scratch will be covered when you need it,
after [C++ 9. Reading templates](../cpp/09_reading_templates.md).
In this exercise, we **expose the iterator of `std::vector` as it is**.
In real work, this is also the right answer in most cases.

## 1.6 Try it yourself

Before you solve the exercise, compile this one file and **predict the output** before you run it.

```cpp
#include <iostream>
#include <string>
#include <vector>

class Book
{
public:
  explicit Book(std::string name) : name_(std::move(name)) {}
  const std::string & name() const { return name_; }

private:
  std::string name_;
};

class Shelf
{
public:
  void append(Book book) { books_.push_back(std::move(book)); }

  // With just this, range-based for and <algorithm> both work
  std::vector<Book>::const_iterator begin() const { return books_.begin(); }
  std::vector<Book>::const_iterator end() const { return books_.end(); }

private:
  std::vector<Book> books_;
};

int main()
{
  Shelf shelf;
  shelf.append(Book{"Design Patterns"});
  shelf.append(Book{"Refactoring"});

  for (const Book & book : shelf) {
    std::cout << book.name() << "\n";
  }
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: how many lines are printed? And what happens if you delete <code>begin()</code> / <code>end()</code>?</summary>

Two lines are printed.

```
Design Patterns
Refactoring
```

If you delete `begin()` / `end()`, the range-based for becomes a compile error.

```
error: 'begin' was not declared in this scope
```

You can see that **range-based for just looks for the names `begin()` / `end()`**.
You need no inheritance and no interface.
</details>

## 1.7 Conclusion for microcontrollers

**Do not use the GoF `Iterator`.** There are two reasons.

1. `iterator()` returns a `std::unique_ptr` = **a heap allocation happens for every traversal**
2. Two virtual calls per element

Code that does `new` inside a loop cannot be written on a microcontroller. The heap becomes fragmented, and allocation eventually fails.

Write `begin()` / `end()` instead. **There are zero allocations and zero virtual functions.**

```cpp
// Fixed length. No dynamic allocation
template <std::size_t N>
class SensorBuffer
{
public:
  const int * begin() const { return data_; }
  const int * end() const { return data_ + size_; }

private:
  int data_[N] = {};
  std::size_t size_ = 0;
};
```

**A raw pointer is also an iterator.** If `++`, `*`, and `!=` work,
then `std::find` and range-based for work too.
You need neither `std::vector` nor `std::unique_ptr`.

If you really want to switch the traversal method at run time,
do not use `std::unique_ptr`. **Construct it in a buffer that the caller provides**,
or decide it at compile time with a template (we cover this in chapter 10, Strategy).

## 1.8 Conclusion for ROS 2 (supplement)

rclcpp also has no GoF `Iterator` class.
Where it is needed, it uses an STL iterator or range-based for.

```cpp
for (const auto & param : this->get_parameters(names)) { /* ... */ }
```

`PointCloud2Iterator`, which walks through `sensor_msgs::msg::PointCloud2`, has Iterator in its name,
but **it is an STL-compatible iterator inside**, with `operator++` and `operator*`.
It is not `hasNext()`.

## 1.9 Common pitfalls

| Symptom | Cause |
| --- | --- |
| `it->name()` copies every time | `next()` returns `Book`. Change it to `const Book &` |
| The program crashes when you release the iterator | `Iterator` has no virtual destructor |
| `error: 'begin' was not declared in this scope` | `begin()` / `end()` is not `public`, or the name is wrong |
| An error when you try to modify in range-based for | It returns `const_iterator`. To modify, you also need an `iterator` version |
| Positions get mixed up when you use two iterators at the same time | The iterator keeps the position on the **bookshelf** side. The iterator should keep the position |
| It crashed when I returned an iterator from a function that returns a bookshelf | The lifetime problem in 1.3 |

## 1.10 Matching exercise

```bash
./drill run dp01
```

In `exercises/dp01_iterator/src/book_shelf.cpp`, you implement

1. The GoF `BookShelfIterator` (`has_next()` / `next()`)
2. `BookShelf::iterator()`, which returns `std::unique_ptr<Iterator>`
3. `BookShelf::begin()` / `end()`, the STL version

The tests check that **walking through the same bookshelf with both methods gives the same result**,
and that `std::count_if` works with the STL version.

## 1.11 Summary of this chapter

- When you move a Java `interface` to C++, **add a virtual destructor**
- Where Java returns `Object`, use a concrete type. **If you do not add `&`, it becomes a copy**
- Where Java does `new` and returns, **return `std::unique_ptr`**
- **An iterator cannot outlive its container.** Java has no such constraint
- In the GoF version "the iterator knows the end". In the STL version, you "compare with an iterator that represents the end"
- **If you write `begin()` / `end()`, `<algorithm>` and range-based for come with it**
- Do not use the GoF version on microcontrollers. The `unique_ptr` allocation and virtual calls add cost

---

Previous: [0. Before you use them](00_before_you_use_them.md) / Next: [2. Adapter](02_Adapter.md)
