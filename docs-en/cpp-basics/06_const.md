# 6. `const`

> **Goal of this chapter**: `const` appears in 4 places in a declaration. The meaning is different in each position, so you must be able to tell them apart. When you understand that **a different rule applies in each of the 4 places**, you can read code with `const` reliably. This chapter greatly improves your ability to read declarations.

## 6.1 Initializing and assigning a `const` value

**Make the value itself `const`.**

```cpp
const int x = 5;
```

Read it as "`x` is a constant `int`". Putting `const` on the right side of the type means "the thing itself" is `const` (see chapter 1).

**It must be initialized. Once decided, it cannot be changed.**

```cpp
const int x = 5;
x = 10;  // Error
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// const_assign.cpp
int main()
{
  const int x = 5;
  x = 10;  // Error
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic const_assign.cpp -o const_assign
```

</details>

<!-- measure: filter="grep -o 'error:.*'" -->
```
error: assignment of read-only variable ‘x’
```

A `const` value is "guaranteed not to change", so the compiler can optimize more easily.
It also lets you show the intent "this does not change", so the code is easier to read.

## 6.2 `const` on function parameters

**Take a `const` reference as a function argument to protect the caller's object.**

```cpp
void process(const std::string & s)
{
  // s is a reference, but it is const, so it cannot be modified
}
```

This pattern is the **most common**. When a parameter is a `std::string` or another large object,
**use `const &` to avoid a copy**. As a result, unintended changes are prevented.

```cpp
void process(const std::string & s)
{
  s = "modified";  // Error
}
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// const_string.cpp
#include <string>

void process(const std::string & s)
{
  s = "modified";  // Error
}

int main()
{
  process("hello");
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic const_string.cpp -o const_string
```

</details>

<!-- measure: filter="grep -o 'error:.*' | head -n 1" -->
```
error: no match for ‘operator=’ (operand types are ‘const std::string’ {aka ‘const std::__cxx11::basic_string<char>’} and ‘const char [9]’)
```

**The message changes with the type.** Here the type is `std::string`, so it appears as
"there is no `operator=` that assigns to a `const std::string`".
For a scalar such as `const int &`, the sentence is more direct.

```cpp
void bump(const int & n)
{
  n = 1;   // Error
}
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// const_int_reference.cpp
void bump(const int & n)
{
  n = 1;   // Error
}

int main()
{
  int value = 0;
  bump(value);
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic const_int_reference.cpp -o const_int_reference
```

</details>

<!-- measure: filter="grep -o 'error:.*'" -->
```
error: assignment of read-only reference ‘n’
```

**The wording differs, but the meaning is the same.** It says "you cannot write through `const`".
Use the word `read-only` as a landmark.

This error is **a promise made by the person who wrote the function**.
The signature declares "this object is only read, never modified",
so the caller can tell "it is safe to pass this" without reading the body.
The value of `const &` is that it avoids a copy and is still safe.

## 6.3 `const` at the end of a member function (the most important one)

**Putting `const` right after the `)` of a member function is a promise: "this member function does not modify `*this`".**

```cpp
class Point
{
public:
  Point(double x, double y) : x_(x), y_(y) {}
  
  double distance() const  // ← const on a member function
  {
    return std::sqrt(x_ * x_ + y_ * y_);
  }
  
  void move(double dx, double dy)  // ← no const = may modify
  {
    x_ += dx;
    y_ += dy;
  }
  
private:
  double x_, y_;
};
```

**A `const` member function can call only other `const` member functions.**

```cpp
class Point
{
public:
  double distance() const
  {
    return distance_squared();  // OK (const call)
  }
  
  double distance_squared() const
  {
    return x_ * x_ + y_ * y_;
  }
  
  void move(double dx, double dy)
  {
    // distance_squared();  // This is OK
    x_ += dx;
  }
  
private:
  double x_, y_;
};
```

**A `const` object can call only `const` member functions.**

```cpp
const Point p(3.0, 4.0);
std::cout << p.distance() << "\n";  // OK
// p.move(1.0, 1.0);  // Error - not a const member function
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// const_this.cpp
#include <cmath>
#include <iostream>

class Point
{
public:
  Point(double x, double y) : x_(x), y_(y) {}
  
  double distance() const  // ← const on a member function
  {
    return std::sqrt(x_ * x_ + y_ * y_);
  }
  
  void move(double dx, double dy)  // ← no const = may modify
  {
    x_ += dx;
    y_ += dy;
  }
  
private:
  double x_, y_;
};

int main()
{
  const Point p(3.0, 4.0);
  std::cout << p.distance() << "\n";  // OK
  p.move(1.0, 1.0);  // Error - not a const member function
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic const_this.cpp -o const_this
```

</details>

Error message:

<!-- measure: filter="grep -o 'error:.*'" -->
```
error: passing ‘const Point’ as ‘this’ argument discards qualifiers [-fpermissive]
```

This rule (**separation by `const`-ness**) is very important.
Read-only member functions and writing member functions are clearly distinguished.

## 6.4 `const` on pointers — the left-binding rule

When `const` is attached to a pointer, its position changes the meaning (recall left-binding from chapter 1).

**`const int * p` — the pointee is `const`**

```cpp
const int * p = &a;
*p = 10;  // Error
p = &b;   // OK - the pointer can move
```

Read it as "a pointer to `const int`", not "a `const` pointer to `int`".

**`int * const p` — the pointer itself is `const`**

```cpp
int * const p = &a;
*p = 10;  // OK - the pointee can be changed
p = &b;   // Error
```

Read it as "a `const` pointer to `int`".

**How to remember: read to the left of `const`.**

- `const int *`: to the left of `const` is `int`, so the `int` is `const`
- `int * const`: to the left of `const` is `*`, so the pointer is `const`

Here is a summary table.

| Code | Meaning | Change the pointee | Change the pointer |
| --- | --- | --- | --- |
| `const int * p` | pointee is const | ✗ | ○ |
| `int * const p` | pointer is const | ○ | ✗ |
| `const int * const p` | both are const | ✗ | ✗ |

## 6.5 Summary of the 4 positions of `const`

| Position | Example | Meaning | What is const |
| --- | --- | --- | --- |
| Declaration | `const int x = 5;` | The value is a constant | The variable itself |
| Reference parameter | `void f(const T & x)` | Promises read-only | What the parameter refers to |
| End of a member function | `void f() const` | `*this` is read-only | The state inside the member function |
| Pointer | `const int * p` / `int * const p` | The pointee / the pointer itself | Decided by the position |

## 6.6 `mutable` — allow writes from a `const` member function

Sometimes you want to modify a member inside a `const` member function.
It is for updating "internal state that the reader cannot see", such as a **cache** or an **access counter**.

```cpp
class Cached
{
public:
  Cached(double x, double y) : x_(x), y_(y), cached_(false) {}
  
  double distance() const
  {
    if (!cached_) {
      cached_ = true;           // mutable, so writing is OK
      distance_ = std::sqrt(x_ * x_ + y_ * y_);
    }
    return distance_;
  }
  
private:
  double x_, y_;
  mutable bool cached_;         // ← these two can be written from a const member function
  mutable double distance_;
};
```

**`mutable` is a last resort.** If you use it carelessly, you break the "read-only" promise.
Use it only for internal state that is invisible from outside, such as a cache or a synchronization flag.

## Try it yourself

Check the 4 positions of `const` and the limits on a `const` object.

```cpp
// const_all.cpp
#include <iostream>
#include <string>

class Rectangle
{
public:
  Rectangle(double w, double h) : width_(w), height_(h), access_count_(0) {}
  
  double area() const
  {
    ++access_count_;  // mutable, so OK
    return width_ * height_;
  }
  
  void resize(double w, double h)
  {
    width_ = w;
    height_ = h;
  }
  
  int access_count() const { return access_count_; }
  
private:
  double width_, height_;
  mutable int access_count_;
};

void print_area(const Rectangle & rect)
{
  std::cout << "area: " << rect.area() << "\n";
}

int main()
{
  std::cout << "== const object ==\n";
  const Rectangle r(3.0, 4.0);
  std::cout << "r.area() = " << r.area() << "\n";
  std::cout << "access_count: " << r.access_count() << "\n";
  
  std::cout << "\n== const reference parameter ==\n";
  print_area(r);
  
  std::cout << "\n== const pointer ==\n";
  int a = 10, b = 20;
  const int * p1 = &a;
  std::cout << "pointee is const: *p1 = " << *p1 << ", can move pointer\n";
  p1 = &b;
  std::cout << "now p1 points to b: *p1 = " << *p1 << "\n";
  
  int * const p2 = &a;
  std::cout << "pointer is const: *p2 = " << *p2 << ", can modify pointee\n";
  *p2 = 99;
  std::cout << "after *p2 = 99: a = " << a << "\n";
  
  return 0;
}
```

**Predict: what is printed for `access_count`? Which of `p1` and `p2` can be moved? Does `a` become 99?**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic const_all.cpp -o const_all && ./const_all
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/jobMjoo76)

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: -->
```
== const object ==
r.area() = 12
access_count: 1

== const reference parameter ==
area: 12

== const pointer ==
pointee is const: *p1 = 10, can move pointer
now p1 points to b: *p1 = 20
pointer is const: *p2 = 10, can modify pointee
after *p2 = 99: a = 99
```

</details>

Check these three points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. Thanks to `mutable`, `access_count` is 1 after the first call (the update inside the `const` member function succeeded)
2. `p1` is not a `const` pointer, so it can be moved to `&b`
3. `p2` is a `const` pointer, but its pointee is not `const`, so it can be changed to `99`

</details>

Next, uncomment the following lines one at a time and read the error messages.

- `r.resize(5.0, 5.0);`
- `*p1 = 30;`
- `p2 = &b;`

**Every one of them stops with `error:`. Say what will happen first, and then uncomment.**

<details markdown="1"><summary>Answer (what happens in each case)</summary>

- `r.resize(5.0, 5.0);`: a const object cannot call a non-const member function
- `*p1 = 30;`: this tries to change the pointee through a pointer to const int
- `p2 = &b;`: this tries to reassign a const pointer

</details>

## Common pitfalls

**`error: assignment of read-only variable ‘x’`**
You tried to assign to a `const` variable. Decide the value at initialization.

**`error: assignment of read-only reference ‘n’`** / **`error: no match for ‘operator=’`**
You tried to modify a `const` reference parameter.
If your intent is "read-only", do not modify what it refers to.
If your intent is "I want to modify it", remove `const`.

**`error: passing ‘const Point’ as ‘this’ argument discards qualifiers`**
You are calling a non-`const` member function on a `const` object.
Add `const` to the member function to make it "read-only".

**`error: assignment of member ‘...’ in read-only object`**
You tried to modify a member from inside a `const` member function.
Unless the member is `mutable`, a `const` member function cannot write to it.

**I cannot tell the two `const` in `const int * const p` apart**
Read it with left-binding. The left `const` means "the pointee is const". The right `const` means "the pointer is const".

## Matching exercise

After reading this chapter, practice with the matching drill.

- `cppb06_const`: match `const` between the declaration and the definition

```bash
./drill run cppb06
```

If you get stuck, run `./drill hint cppb06`. From the exercise side, `./drill read cppb06` brings you back to this chapter.

## References

- `cppreference`: [const-qualified type](https://en.cppreference.com/w/cpp/language/const_cast) and [Member functions](https://en.cppreference.com/w/cpp/language/member_functions)
- [ROS 2 coding conventions](../ros2_coding_conventions.md): implementation patterns that use `const` reference parameters as "read-only"

---

Previous → [5. Pointers 2 — Arrays and Choosing Between Them](05_pointers_2.md)
Next → [7. `static`](07_static.md)
