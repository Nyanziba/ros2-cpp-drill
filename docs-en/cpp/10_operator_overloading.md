# 10. Operator overloading

> **Goal of this chapter**: At the end of the previous chapter, we showed the error "`Point` has no `operator<`"
> and wrote "define `operator<`". **We did not explain how to define it.**
> This chapter does that.
>
> `operator+` and `operator<` are **just functions**. Only the name is a symbol. It is not special syntax.
> Once you see that, you can explain with the same principle the condition for `std::sort` to work, why `std::cout << x` works,
> what the `->` in `publisher_->publish(...)` is, and
> why you can chain `qos.reliable().transient_local()`.

> **Prerequisite**: This chapter assumes you have read the trailing `const` in [C++ Basics chapter 6 `const`](../cpp-basics/06_const.md) and
> [chapter 3 References](../cpp-basics/03_references.md).
> Operators are functions, so the decisions about **taking arguments by `const &` and whether to return a reference**
> come up as they are.

## 10.1 An operator is a function

First, look at this.

```cpp
// vec2_add.cpp
#include <iostream>

struct Vec2
{
  double x;
  double y;
};

Vec2 operator+(const Vec2 & a, const Vec2 & b)
{
  return Vec2{a.x + b.x, a.y + b.y};
}

int main()
{
  Vec2 a{1.0, 2.0};
  Vec2 b{10.0, 20.0};

  Vec2 c = a + b;                  // <- operator+(a, b) is called
  Vec2 d = operator+(a, b);        // <- exactly the same thing. You can write it this way too

  std::cout << c.x << "," << c.y << "\n";
  std::cout << d.x << "," << d.y << "\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/817vq8q15)

<!-- measure: files=vec2_add.cpp cmd="g++ -std=c++17 vec2_add.cpp -o vec2_add && ./vec2_add" -->
```
11,22
11,22
```

**`a + b` is only a rewrite of `operator+(a, b)`.**
It is an ordinary function whose name is `operator+`.
Because it is a function, everything from the earlier chapters works as is: declaration, definition, overloading, `const`, references.

Try reading it again as a function declaration (the right-to-left procedure from chapter 4).

```cpp
Vec2 operator+(const Vec2 & a, const Vec2 & b)
//^^^^         ^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^
// return type  a is a reference to a Vec2 that cannot be changed (same for b)
```

- The return type is `Vec2` (**a value**. You must not return a reference. That would be the dangling reference of section 4.3)
- The arguments are `const Vec2 &` (avoid copies, and promise not to modify)

**This form is the standard form of operator overloading.** When in doubt, copy it.

## 10.2 Write it as a member or as a free function

You can also write the same `operator+` inside the class.

```cpp
struct Vec2
{
  double x;
  double y;

  Vec2 operator+(const Vec2 & other) const     // <- member version
  {
    return Vec2{x + other.x, y + other.y};
  }
};
```

**There is now 1 argument.** The left-hand side is `this`.

```cpp
a + b;
// free function version -> operator+(a, b)
// member version        -> a.operator+(b)
```

| | Free function | Member function |
| --- | --- | --- |
| Number of arguments (binary operator) | 2 | 1 (the left side is `this`) |
| The left side may be a type other than your own | **Yes** | No |
| Can touch private members | No (needs `friend`) | Yes |
| Trailing `const` | Not needed | **Needed** (it does not modify the left side) |

### The rule for choosing

**The criterion is "can the left side be a type other than your own?"**

Suppose you want to write `2.0 * v` (scalar times vector). With the member version this is **impossible**.
The left side is a `double`, so you would have to write it as a member function of `double`,
and you cannot extend `double` yourself.

```cpp
// you can write it as a free function
Vec2 operator*(double s, const Vec2 & v)
{
  return Vec2{s * v.x, s * v.y};
}

Vec2 operator*(const Vec2 & v, double s)   // the reverse order is a separate function
{
  return Vec2{v.x * s, v.y * s};
}
```

**`v * 2.0` and `2.0 * v` are different functions.** If you want both, write both.
Writing only one and getting an error for the other is a common oversight in practice.

The practical rule is this.

| Operator | Which to use |
| --- | --- |
| `+` `-` `*` `/` `==` `!=` `<` `>` `<=` `>=` | **Free function** |
| `+=` `-=` `*=` `/=` `=` `[]` `()` `->` | **Member function** (the language requires it for some) |
| `<<` `>>` (stream) | **Free function** (the left side is `std::ostream`, so there is no choice) |

The standard says `=` `[]` `()` `->` **must be members**.

## 10.3 Comparison operators and `std::sort`

This is the homework from the previous chapter. What does `std::sort` require?

```cpp
#include <algorithm>
#include <vector>

struct Point { int x; int y; };

int main()
{
  std::vector<Point> v{{3, 4}, {1, 2}};
  std::sort(v.begin(), v.end());     // Point has no <
  return 0;
}
```

[⚠ See this error in your browser (gcc 13.3)](https://godbolt.org/z/98ffMj7M1)

As we saw in the previous chapter, this gives a long error, and it is 3 lines (all with the same cause) if you narrow it down with `grep "error:"`. The output looks like this.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// sortpoint.cpp
#include <algorithm>
#include <vector>

struct Point { int x; int y; };

int main()
{
  std::vector<Point> v{{3, 4}, {1, 2}};
  std::sort(v.begin(), v.end());     // Point has no <
  return 0;
}
```

```bash
g++ -std=c++17 sortpoint.cpp -o sortpoint 2>&1 | grep "error:"
```

</details>

<!-- measure: -->
```
/usr/include/c++/13/bits/predefined_ops.h:45:23: error: no match for ‘operator<’ (operand types are ‘Point’ and ‘Point’)
/usr/include/c++/13/bits/predefined_ops.h:98:22: error: no match for ‘operator<’ (operand types are ‘Point’ and ‘Point’)
/usr/include/c++/13/bits/predefined_ops.h:69:22: error: no match for ‘operator<’ (operand types are ‘Point’ and ‘Point’)
```

**`std::sort` sorts using `a < b` by default.** That is why `operator<` is needed.

```cpp
bool operator<(const Point & a, const Point & b)
{
  return a.x < b.x;
}
```

Now it compiles. The return type is `bool`.

### `operator<` must be a "strict weak ordering"

This is the trap. **The comparison you pass to `std::sort` must satisfy these conditions.**

1. `a < a` is always `false` (nothing is smaller than itself)
2. If `a < b`, then `b < a` is `false`
3. If `a < b` and `b < c`, then `a < c`

This is called a **strict weak ordering**.
If you break it, `std::sort` has **undefined behavior**. It can really crash.

The most common violation is this.

```cpp
bool operator<(const Point & a, const Point & b)
{
  return a.x <= b.x;     // <- you wrote <=
}
```

When `a.x == b.x`, both `a < b` and `b < a` are `true`, which breaks condition 2.
**Write `<`, not `<=`.**

When you want to sort by several keys, `std::tie` is safe and easy.

```cpp
#include <tuple>

bool operator<(const Point & a, const Point & b)
{
  return std::tie(a.x, a.y) < std::tie(b.x, b.y);   // compare x, and if equal compare y
}
```

The `operator<` of `std::tuple` implements lexicographic comparison correctly,
so you make fewer mistakes than writing `if` yourself.

### `operator==` and `operator!=`

In C++17 you **must write both.**

```cpp
bool operator==(const Vec2 & a, const Vec2 & b)
{
  return a.x == b.x && a.y == b.y;
}

bool operator!=(const Vec2 & a, const Vec2 & b)
{
  return !(a == b);      // write it using ==
}
```

Implementing `!=` with `==` is the standard approach. It is so that you **do not write the logic in 2 places**.

(In C++20, if you write one line `auto operator<=>(const Vec2 &) const = default;`,
all 6 comparison operators are generated. **This drill uses C++17, so we do not use it.**
Just know that such a way of writing exists.)

### Be careful with `==` on floating point

`Vec2` holds `double`. We wrote `a.x == b.x` in `operator==`, but
**exact comparison of floating point numbers is dangerous.**

```cpp
// float_equal.cpp
#include <iostream>
int main()
{
  double a = 0.1 + 0.2;
  double b = 0.3;
  std::cout << (a == b) << "\n";
  std::cout << a - b << "\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/Msqzoec4d)

<!-- measure: files=float_equal.cpp cmd="g++ -std=c++17 float_equal.cpp -o float_equal && ./float_equal" -->
```
0
5.55112e-17
```

`0.1 + 0.2 != 0.3`. This is because **0.1 cannot be represented exactly as a binary fraction**.

When you compare floating point numbers in a test, use gtest's `EXPECT_DOUBLE_EQ` (allows an error of a few ULP) or
`EXPECT_NEAR(a, b, tol)`.
This is why the sample solution of exercise 11 uses `EXPECT_DOUBLE_EQ`.

```cpp
// solutions/11_node_test/test/test_exercise.cpp
EXPECT_DOUBLE_EQ(limit_velocity(5.0, 0.0, 100.0, 1.0), 1.0);
```

## 10.4 `operator<<` — make it work with `std::cout`

The mechanism that makes `std::cout << x` work is also an operator.

```cpp
// ostream_output.cpp
#include <iostream>
#include <sstream>

struct Vec2
{
  double x;
  double y;
};

std::ostream & operator<<(std::ostream & os, const Vec2 & v)
{
  os << "(" << v.x << ", " << v.y << ")";
  return os;
}

int main()
{
  Vec2 v{1.5, -2.5};
  std::cout << v << "\n";

  std::ostringstream ss;
  ss << "pos=" << v;
  std::cout << ss.str() << "\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/az1zv1dMo)

<!-- measure: files=ostream_output.cpp cmd="g++ -std=c++17 ostream_output.cpp -o ostream_output && ./ostream_output" -->
```
(1.5, -2.5)
pos=(1.5, -2.5)
```

**Memorize this form. There are 3 points.**

```cpp
std::ostream & operator<<(std::ostream & os, const Vec2 & v)
//         ^①              ^②                  ^③
```

**① The return type is `std::ostream &` (a reference)** — this is why `<<` can be chained.

```cpp
std::cout << v << "\n";
// ((std::cout << v) << "\n")
//  ^^^^^^^^^^^^^^^^ this returns std::ostream &, so you can continue with <<
```

This is the only case where **returning a reference is correct**. In section 4.3 we wrote "do not return a reference", but
`os` is an existing object passed in as an argument,
so it does not die when the function returns. It does not dangle.

**② The 1st argument is `std::ostream &` (non-const)** — it writes, so it cannot be `const`.

**③ The 2nd argument is `const Vec2 &`** — it only reads, so this.

**And you must always write `return os;`.** If you forget, you cannot chain `<<`,
and you get a confusing error such as `error: invalid operands to binary expression`.

### Why it cannot be a member

The left side of `std::cout << v` is `std::cout`, that is, a `std::ostream`.
To make it a member function, you would have to write it as a member of `std::ostream`,
and **you cannot add members to a standard library class afterwards.**

So a free function is the only choice. This is exactly the criterion from section 10.2.

### Use `friend` when you want to output `private` members

```cpp
class Vec2
{
public:
  Vec2(double x, double y) : x_(x), y_(y) {}

  friend std::ostream & operator<<(std::ostream & os, const Vec2 & v);

private:
  double x_;
  double y_;
};

std::ostream & operator<<(std::ostream & os, const Vec2 & v)
{
  os << "(" << v.x_ << ", " << v.y_ << ")";   // can touch private
  return os;
}
```

`friend` is a permission that says "only this function may see private".
**You write `friend` inside the class, but it does not become a member function.** It stays a free function.

If you use `friend` too much, encapsulation breaks. Keep it to things like `operator<<` and `operator==`.
If there is a public accessor, you do not need `friend`.

## 10.5 `+=` and method chaining

Write `+=` as a member.

```cpp
struct Vec2
{
  double x;
  double y;

  Vec2 & operator+=(const Vec2 & other)
  {
    x += other.x;
    y += other.y;
    return *this;             // <- return a reference to itself
  }
};
```

**The return type is `Vec2 &` (a reference to itself).** This gives 2 effects.

**① You can write it in a row**

```cpp
Vec2 v{0, 0};
v += a += b;      // works (you rarely write it, though)
```

**② You can do method chaining**

This is important in practice. The QoS of rclcpp has exactly this form.

```cpp
// the form you write in exercise 12
rclcpp::QoS qos(rclcpp::KeepLast(1));
qos.transient_local();
qos.reliable();

// you can chain them (same meaning)
auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).transient_local().reliable();
```

`transient_local()` returns `QoS &`, so you can continue with `.reliable()`.
**It is not an operator, but it is the same mechanism: `return *this;`.**

When you want both `+` and `+=`, implement `+` with `+=`.

```cpp
Vec2 operator+(Vec2 a, const Vec2 & b)   // the trick is to take the 1st argument "by value"
{
  a += b;         // modify the copy
  return a;       // NRVO applies (section 5.4)
}
```

We deliberately take the 1st argument by value. **One copy is needed anyway, so
letting the caller make it as the argument is easier for the compiler to optimize.** This is the standard trick.

## 10.6 What not to do

Operator overloading **becomes unreadable if you overdo it.**

**Rule 1: Do not change the meaning of a symbol.**
If `operator+` did subtraction, nobody could read it.
Using `operator<<` as a shift operator is legitimate,
but be careful about giving it your own meaning such as "append to the log".

**Rule 2: Do not overload `&&` `||` `,`.**
The built-in versions do **short-circuit evaluation** (`a && b` does not evaluate `b` if `a` is false).
If you overload them, **they become plain function calls, so both are evaluated.**
The behavior changes silently, so avoid it.

**Rule 3: When in doubt, use a named function.**

```cpp
Vec2 operator^(const Vec2 & a, const Vec2 & b);   // cross product? XOR? unclear
double cross(const Vec2 & a, const Vec2 & b);      // readable
```

**`cross(a, b)` is better code.**
Make something an operator only when "its meaning as a mathematical symbol is obvious".

**Rule 4: Some operators cannot be overloaded.**
`.` `::` `?:` `sizeof` are not possible.

## 10.7 Operators that appear in ROS 2 code

You rarely write them yourself, but **you read them on every line.**

| Expression | Which operator | Where |
| --- | --- | --- |
| `publisher_->publish(msg)` | `shared_ptr::operator->` | all exercises |
| `*msg` | `shared_ptr::operator*` | exercise 14 |
| `msg->data` | `unique_ptr::operator->` | exercise 14 |
| `if (auto p = w.lock())` | `shared_ptr::operator bool` | chapter 6 |
| `p == nullptr` | `shared_ptr::operator==` | chapter 6 |
| `std::cout << x` | `ostream::operator<<` | tests |
| `v[i]` | `vector::operator[]` | chapter 11 |
| `500ms + 1s` | `chrono::duration::operator+` | chapter 12 |
| `qos.reliable()` | (not an operator, but `return *this`) | exercise 12 |

**`operator->` is quietly important.** `shared_ptr` overloads `->`
so that it looks like "access a member of the pointed-to object".
So you can write `publisher_->publish(...)` without
being aware that `publisher_` is a `shared_ptr`.

`operator bool` is also a handy mechanism.

```cpp
if (auto p = w.lock()) { }      // true if p is not nullptr
```

Here `p.operator bool()` is called.
It is defined as `explicit operator bool()`, so
an implicit conversion such as `bool b = p;` does not happen (the `explicit` from section 2.5).
**There is a special rule: only inside the condition of an `if`, the conversion works even with `explicit`.**

## Try it yourself

**You will add operators to `Vec2` step by step, until `std::sort` and `std::cout` work.**
This is the same content as the C++ exercise cpp10, so if you work through it by hand first, the exercise becomes easier.

```cpp
// ops.cpp
#include <algorithm>
#include <iostream>
#include <sstream>
#include <tuple>
#include <vector>

struct Vec2
{
  double x;
  double y;

  // TODO 1: implement operator+= as a member (return Vec2 &)

  double length_squared() const { return x * x + y * y; }
};

// TODO 2: implement operator+ as a free function
// TODO 3: implement 2 versions of operator* (Vec2 * double and double * Vec2)
// TODO 4: implement operator== and operator!=
// TODO 5: implement operator< (smaller length_squared() first. You may use std::tie)
// TODO 6: implement operator<< (in the form "(x, y)". Return std::ostream &)

int main()
{
  Vec2 a{1.0, 2.0};
  Vec2 b{10.0, 20.0};

  std::cout << "a      = " << a << "\n";
  std::cout << "a + b  = " << (a + b) << "\n";
  std::cout << "a * 3  = " << (a * 3.0) << "\n";
  std::cout << "3 * a  = " << (3.0 * a) << "\n";

  Vec2 c = a;
  c += b;
  std::cout << "c      = " << c << "\n";

  std::cout << "a == a : " << (a == a) << "\n";
  std::cout << "a != b : " << (a != b) << "\n";

  std::vector<Vec2> v{{3.0, 4.0}, {1.0, 0.0}, {0.0, 2.0}};
  std::sort(v.begin(), v.end());
  std::cout << "sorted :";
  for (const auto & e : v) {
    std::cout << " " << e;
  }
  std::cout << "\n";

  std::ostringstream ss;
  ss << "pos=" << a;
  std::cout << "stream : " << ss.str() << "\n";
  return 0;
}
```

The expected output is this.

<details markdown="1"><summary>Full program that produced this output (the TODOs filled in. It contains the answer to exercise cpp10, so solve it first)</summary>

```cpp
// ops.cpp
#include <algorithm>
#include <iostream>
#include <sstream>
#include <tuple>
#include <vector>

struct Vec2
{
  double x;
  double y;

  Vec2 & operator+=(const Vec2 & other)
  {
    x += other.x;
    y += other.y;
    return *this;
  }

  double length_squared() const { return x * x + y * y; }
};

Vec2 operator+(Vec2 a, const Vec2 & b)
{
  a += b;
  return a;
}

Vec2 operator*(const Vec2 & v, double s)
{
  return Vec2{v.x * s, v.y * s};
}

Vec2 operator*(double s, const Vec2 & v)
{
  return Vec2{s * v.x, s * v.y};
}

bool operator==(const Vec2 & a, const Vec2 & b)
{
  return a.x == b.x && a.y == b.y;
}

bool operator!=(const Vec2 & a, const Vec2 & b)
{
  return !(a == b);
}

bool operator<(const Vec2 & a, const Vec2 & b)
{
  return a.length_squared() < b.length_squared();
}

std::ostream & operator<<(std::ostream & os, const Vec2 & v)
{
  os << "(" << v.x << ", " << v.y << ")";
  return os;
}

int main()
{
  Vec2 a{1.0, 2.0};
  Vec2 b{10.0, 20.0};

  std::cout << "a      = " << a << "\n";
  std::cout << "a + b  = " << (a + b) << "\n";
  std::cout << "a * 3  = " << (a * 3.0) << "\n";
  std::cout << "3 * a  = " << (3.0 * a) << "\n";

  Vec2 c = a;
  c += b;
  std::cout << "c      = " << c << "\n";

  std::cout << "a == a : " << (a == a) << "\n";
  std::cout << "a != b : " << (a != b) << "\n";

  std::vector<Vec2> v{{3.0, 4.0}, {1.0, 0.0}, {0.0, 2.0}};
  std::sort(v.begin(), v.end());
  std::cout << "sorted :";
  for (const auto & e : v) {
    std::cout << " " << e;
  }
  std::cout << "\n";

  std::ostringstream ss;
  ss << "pos=" << a;
  std::cout << "stream : " << ss.str() << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic ops.cpp -o ops && ./ops
```

</details>

<!-- measure: -->
```
a      = (1, 2)
a + b  = (11, 22)
a * 3  = (3, 6)
3 * a  = (3, 6)
c      = (11, 22)
a == a : 1
a != b : 1
sorted : (1, 0) (0, 2) (3, 4)
stream : pos=(1, 2)
```

**Fill in the TODOs from the top, and compile each time.**
If you add them one at a time, you can see **what error appears before you add each one**. That is the purpose.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic ops.cpp -o ops && ./ops
```

[✎ Fill in the TODOs and run in your browser (gcc 13.3)](https://godbolt.org/z/hj98bs1rP)

**Things to check.**

1. What error do you get when you call `3.0 * a` with only one of the `operator*` written?
2. What happens if you write `operator<` with `<=`? (Increase the elements to about 10 and run `std::sort`)
3. What error do you get if you delete the `return os;` in `operator<<`?
4. What happens if you make the return type of `operator<<` `std::ostream` (a value, not a reference)?

**4 is especially worth seeing.** `std::ostream` cannot be copied, so
you get `error: use of deleted function`. The no-copy story from chapter 3 appears here.

## Common pitfalls

**`error: no match for ‘operator<’ (operand types are ‘X’ and ‘X’)`**
You pass your own type to `std::sort` / `std::map` / `std::set`.
Define `operator<`, or pass a comparison function.

**`error: no match for ‘operator*’ (operand types are ‘double’ and ‘Vec2’)`**
You did not write `operator*(double, Vec2)`. **The reverse order is a separate function.**

**`error: passing ‘const Vec2’ as ‘this’ argument discards qualifiers`**
You forgot the trailing `const` on the member version of the operator.

```cpp
Vec2 operator+(const Vec2 & other) const   // <- this const
```

**You wrote `operator<<`, but `std::cout << v` does not compile**
Check 3 things.

1. Is it written as a free function? (A member does not work)
2. Is the 1st argument `std::ostream &`?
3. Do you have `#include <ostream>` or `<iostream>`?

**`error: ‘operator<<’ must have exactly one argument`**
You wrote it as a member function. Move it outside the class.

**`<<` does not chain**
You forgot `return os;`. Or the return type is `void`.

**`error: ‘operator=’ must be a non-static member function`**
`=` `[]` `()` `->` can only be members. You cannot make them free functions.

**`std::sort` crashes / the result is strange**
`operator<` is not a strict weak ordering. Check that you are not using `<=`.
Rewriting it with `std::tie` is safe.

**`==` on floating point gives false unexpectedly**
`0.1 + 0.2 != 0.3`. See section 10.3. In tests, use `EXPECT_DOUBLE_EQ` / `EXPECT_NEAR`.

## Where it appears in the drill

**The only exercise where you "define" operators is cpp10 in the C++ track.**
In the 15 exercises of the ROS 2 track, you do not need to define any operator.

On the other hand, **there are places where you "use" them in every exercise.**

| Exercise | Operators used |
| --- | --- |
| 01 | `publisher_->publish(...)` (`shared_ptr::operator->`), `"Hello, world! " + std::to_string(...)` (`string::operator+`) |
| 02 | `msg.data.c_str()` (`.` is member access. It is not an operator) |
| 04, 05 | `request->a`, `response->sum` |
| 06, 07 | accessors such as `param.as_int()` |
| 11 | `EXPECT_DOUBLE_EQ` — the reason not to compare floating point with `==` (section 10.3) |
| 12 | `qos.reliable().transient_local()` — chaining by `return *this` |
| 13 | `future.wait_for(2s) != std::future_status::ready` (`!=` on an `enum`) |
| 14 | `msg->data`, `*msg`, `p == nullptr` |
| cpp10 | **The defining side.** `+` `-` `*` `==` `!=` `<` `<<` `+=` |

Try reading the QoS of exercise 12 again with the eyes of this chapter.

```cpp
rclcpp::QoS qos(rclcpp::KeepLast(1));
qos.transient_local();
qos.reliable();
```

`transient_local()` and `reliable()` return `QoS &`, so
**it is the same whether you split it into 3 lines or chain it into 1 line**.
If you look at the header of `rclcpp::QoS`, this is really how it is.

```bash
grep -n "QoS &" /opt/ros/jazzy/include/rclcpp/rclcpp/qos.hpp | head
```

## Matching exercise

After reading this chapter, practice with the matching drill exercise.

- `cpp10_operators` — overload operators

```bash
./drill run cpp10
```

From the exercise, you can come back to this chapter with `./drill read`.

## References

- `/opt/ros/jazzy/include/rclcpp/rclcpp/qos.hpp` — a real example of method chaining that returns `QoS &`
- `cppreference` [Operator overloading](https://en.cppreference.com/w/cpp/language/operators) — the full list of overloadable operators and the standard way to write each
- `cppreference` [Strict weak ordering](https://en.cppreference.com/w/cpp/named_req/Compare) — the condition that `std::sort` requires
- C++ Core Guidelines C.160 to C.168 — guidelines for operator overloading ("make symmetric operators free functions", etc.)

---

Previous chapter → [9. Reading templates](09_reading_templates.md)
Next chapter → [11. The standard library toolbox](11_standard_library_toolbox.md)
