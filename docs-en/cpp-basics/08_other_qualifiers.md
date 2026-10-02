# 8. Other qualifiers

> **Goal of this chapter**: `inline`, `explicit`, `mutable`, and `constexpr` are instructions to the compiler and the linker: "please do it this way". Each one solves a completely different problem. Like `const` and `static`, they are qualifiers you need to know.

## 8.1 `inline` — relax the ODR (One Definition Rule)

**`inline` tells the compiler: "it is OK if this definition appears in several translation units."**

Suppose you write a function definition in a header file and `#include` it from several `.cpp` files. Then the definition is duplicated in every translation unit. If you link without `inline`, you get a **multiple definition error**.

```cpp
// helper.h
int add_one(int x) {
  return x + 1;
}
```

```cpp
// file1.cpp
#include "helper.h"
int get_from_file1() { return add_one(10); }
```

```cpp
// file2.cpp
#include "helper.h"
int get_from_file2() { return add_one(20); }
```

If you link the files above, you get:

```
/usr/bin/ld: file2.o: in function `add_one(int)':
file2.cpp:(.text+0x0): multiple definition of `add_one(int)';
file1.o:file1.cpp:(.text+0x0): first defined here
```

**With `inline`, the linker allows the duplicates and merges them into one.**

```cpp
// helper.h
inline int add_one(int x) {
  return x + 1;
}
```

Now the link succeeds. Check with `nm -C`: without `inline` the symbol is `T` (strong symbol), with `inline` it is `W` (weak symbol):

```
=== Without inline ===
0000000000000000 T add_one(int)

=== With inline ===
0000000000000000 W add_one(int)
```

Weak symbols are merged at link time.

**In modern C++, a definition in a header should be either `inline` or `constexpr`.**
Templates are treated as `inline` automatically.

## 8.2 `explicit` — block implicit type conversion

**A constructor that takes one argument can be used by the caller for an implicit type conversion. `explicit` prevents this.**

```cpp
#include <iostream>

class IsEnabled {
public:
  IsEnabled(bool value) : value_(value) {}
  
  bool is_on() const { return value_; }
  
private:
  bool value_;
};

void configure(IsEnabled enabled) {
  if (enabled.is_on()) {
    std::cout << "enabled\n";
  } else {
    std::cout << "disabled\n";
  }
}

int main() {
  configure(true);   // implicitly converted to IsEnabled(true)
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/xrzb879bP)

```
enabled
```

Is this what you meant to write? If you pass `true` directly, you will not notice a mistake.

**With `explicit`, the implicit conversion is blocked.**

```cpp
class IsEnabled {
public:
  explicit IsEnabled(bool value) : value_(value) {}
  
  bool is_on() const { return value_; }
  
private:
  bool value_;
};

// ...
configure(true);  // ERROR
```

```
error: could not convert 'true' from 'bool' to 'IsEnabled'
```

The correct call uses the constructor explicitly:

```cpp
configure(IsEnabled(true));   // OK
```

**This is why you see `explicit Node(...)` in ROS 2 code.** Some versions of the node constructor take many parameters, and `explicit` protects them from being called with a wrong type.

## 8.3 `mutable` — allow changes from a `const` member function

**A `const` member function cannot change members. `mutable` is the exception.** A `mutable` member can be changed even from a `const` member function.

```cpp
#include <iostream>

class CachedValue {
public:
  CachedValue(int value) : value_(value), access_count_(0) {}
  
  int get() const {
    ++access_count_;   // mutable, so it can change even in a const function
    return value_;
  }
  
  int access_count() const {
    return access_count_;
  }
  
private:
  int value_;
  mutable int access_count_;
};

int main() {
  const CachedValue cv(42);
  std::cout << cv.get() << "\n";
  std::cout << cv.get() << "\n";
  std::cout << "access count: " << cv.access_count() << "\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/rPcrf4a9c)

```
42
42
access count: 2
```

The accesses are counted even inside the `const` function `get()`.

**Legitimate uses:**
- **Cache**: a `const` member function still updates cache-hit information
- **Mutex**: protect the execution of a `const` member function in multithreaded code
- **Debug counter**: quietly record how many times a function was called

**Warning:** If you want to use `mutable` for any other purpose, **rethink your design.** `mutable` is a last resort: "this should not change, but for implementation reasons it does".

## 8.4 `constexpr` — compile-time computation

**A `constexpr` function can be computed at compile time.** You can prove it with `static_assert` (`static_assert` accepts only compile-time expressions).

```cpp
#include <iostream>

constexpr int factorial(int n) {
  return n <= 1 ? 1 : n * factorial(n - 1);
}

// If factorial(5) is not computed at compile time, this is a compile error
static_assert(factorial(5) == 120, "factorial must be compile-time");

int main() {
  std::cout << "factorial(5) = " << factorial(5) << "\n";
  return 0;
}
```

```
factorial(5) = 120
```

If `static_assert` passes, **everything was computed at compile time.**

**Conditions: a `constexpr` function**
- works with literal types (`int`, `double`, and so on)
- may use recursion, loops, and branches
- does not accept input that is known only at run time

In C++20, `constexpr` can do even more (for example, dynamic memory allocation).

## Try it yourself

This program checks all four qualifiers at once.

```cpp
#include <iostream>

// === inline ===
inline int add_one(int x) {
  return x + 1;
}

// === explicit ===
class IsEnabled {
public:
  explicit IsEnabled(bool value) : value_(value) {}
  
  bool is_on() const { return value_; }
  
private:
  bool value_;
};

void configure(IsEnabled enabled) {
  if (enabled.is_on()) {
    std::cout << "enabled\n";
  } else {
    std::cout << "disabled\n";
  }
}

// === mutable ===
class CachedValue {
public:
  CachedValue(int value) : value_(value), access_count_(0) {}
  
  int get() const {
    ++access_count_;
    return value_;
  }
  
  int access_count() const {
    return access_count_;
  }
  
private:
  int value_;
  mutable int access_count_;
};

// === constexpr ===
constexpr int factorial(int n) {
  return n <= 1 ? 1 : n * factorial(n - 1);
}

static_assert(factorial(5) == 120, "factorial must be compile-time");

int main() {
  std::cout << "=== inline ===\n";
  std::cout << "add_one(10) = " << add_one(10) << "\n";
  
  std::cout << "\n=== explicit ===\n";
  configure(IsEnabled(true));
  configure(IsEnabled(false));
  
  std::cout << "\n=== mutable ===\n";
  const CachedValue cv(42);
  std::cout << "value: " << cv.get() << "\n";
  std::cout << "value: " << cv.get() << "\n";
  std::cout << "access count: " << cv.access_count() << "\n";
  
  std::cout << "\n=== constexpr ===\n";
  std::cout << "factorial(5) = " << factorial(5) << "\n";
  
  return 0;
}
```

**Predict: Is the implicit conversion blocked by `explicit`? Does the access counter go up with `mutable`? Does `static_assert` pass?**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/dh6MKMMG5)

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/PPsxn1bPz)

<details markdown="1"><summary>Answer (actual output)</summary>

```
=== inline ===
add_one(10) = 11

=== explicit ===
enabled
disabled

=== mutable ===
value: 42
value: 42
access count: 2

=== constexpr ===
factorial(5) = 120
```

</details>

## Common pitfalls

**`error: could not convert 'true' from 'bool' to 'MyClass'`**
You put `explicit` on the constructor. It blocks the implicit conversion.
At the call site, write `MyClass(value)` explicitly.

**`error: assignment of read-only member`**
You forgot `mutable`. Put `mutable` on the member variable that you want to change inside a `const` member function.

**A `constexpr` function is not computed at compile time**
The function contains an operation that cannot be computed at compile time.
Examples: using `std::vector`, or dereferencing a pointer. Check the new features of C++20.

**`undefined reference to ...` (the linker cannot find an `inline` function)**
Did you define an `inline` function in a `.cpp` file and call it from another `.cpp` file?
The definition of an `inline` or `constexpr` function should be in the header.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `cppb08_qualifiers` — put the qualifiers in the right places

```bash
./drill run cppb08
```

If you get stuck, run `./drill hint cppb08`. From the exercise side, `./drill read cppb08` brings you back to this chapter.

## References

- `cppreference`: [inline specifier](https://en.cppreference.com/w/cpp/language/inline), [explicit specifier](https://en.cppreference.com/w/cpp/language/explicit), [mutable](https://en.cppreference.com/w/cpp/language/cv), [constexpr](https://en.cppreference.com/w/cpp/language/constexpr)
- ROS 2 [C++ coding style](https://docs.ros.org/en/rolling/The-ROS2-Project/Contributing/Code-Style-Language-Versions.html)

---

Previous chapter → [7. `static`](07_static.md)
Next chapter → [9. Value semantics](09_value_semantics.md)
