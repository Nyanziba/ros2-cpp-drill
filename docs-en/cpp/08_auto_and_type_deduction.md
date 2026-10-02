# 8. `auto` and type deduction

> **Goal of this chapter**: `auto` appears **139 times** in the drill. It is the most common syntax of all.
> Even so, `auto` is **not a feature that lets the writer skip writing types.**
> The compiler follows clear rules to decide the type, and if you do not know these rules,
> accidents happen: "I used `auto` and a copy was made" and "I used `auto` and `const` disappeared".
> This chapter is short, but it settles the content of chapters 4 and 5 in the context of `auto`.

> **Prerequisite**: This chapter assumes you have read [C++ Basics chapter 6 `const`](../cpp-basics/06_const.md) and
> [chapter 3 References](../cpp-basics/03_references.md).
> **`auto` drops `const` and `&`.** If you cannot read the original type,
> you will not notice that they were dropped, and an unintended copy will be made.

## 8.1 Why use `auto`

Type names in rclcpp are long.

```cpp
std::shared_ptr<rclcpp::Publisher<std_msgs::msg::String, std::allocator<void>>> publisher =
  this->create_publisher<std_msgs::msg::String>("topic", 10);
```

Write this with `auto`.

```cpp
auto publisher = this->create_publisher<std_msgs::msg::String>("topic", 10);
```

To keep the 100-character line limit ([coding conventions](../ros2_coding_conventions.md)), `auto` is practically required.

**`auto` does not mean "no type is decided". It only means "you do not write the type".**
The type is still fixed statically to exactly one type. It is completely different from a Python variable.

Here are 3 values of `auto`.

1. **You do not write long type names** — the example above
2. **You can handle types you cannot write** — the type of a lambda has no name (chapter 7)
3. **Fewer places to fix when a type changes** — even if the return type changes, the caller stays the same

A real example from exercise 01.

```cpp
// solutions/01_publisher/src/minimal_publisher.cpp
auto message = std_msgs::msg::String();
```

This is almost the same as `std_msgs::msg::String message;` (the `auto` version calls the
constructor explicitly). We follow it because the official tutorial writes it this way.

## 8.2 `auto` drops references and `const`

**This is the only important pitfall of `auto`.**

```cpp
// auto_ref.cpp
#include <iostream>
#include <string>
#include <vector>

int main()
{
  std::vector<std::string> v{"hello", "world"};

  const std::string & ref = v[0];

  auto a = ref;          // std::string (a copy! both const and & are dropped)
  auto & b = ref;        // const std::string & (stays a reference)
  const auto & c = ref;  // const std::string & (explicit and easy to read)

  a = "changed";         // it is a copy, so v[0] does not change
  std::cout << v[0] << " " << a << "\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/6hrhfe17d)

<!-- measure: files=auto_ref.cpp cmd="g++ -std=c++17 auto_ref.cpp -o auto_ref && ./auto_ref" -->
```
hello changed
```

**`auto a = ref;` becomes `std::string`, and a copy is made.**
`auto` does not carry over that `ref` is a reference or that it is `const`.

The rule fits in one line. **`auto` deduces as a "value".**
If you want a reference or `const`, you add `&` or `const` yourself.

| How you write it | Deduced type | Copy |
| --- | --- | --- |
| `auto x = expr;` | value (`const` and `&` are dropped) | **happens** |
| `auto & x = expr;` | reference (`const` is kept) | does not happen |
| `const auto & x = expr;` | `const` reference | does not happen |
| `auto && x = expr;` | forwarding reference (accepts both rvalues and lvalues) | does not happen |

### This matters in range-based for

We touched on this in chapter 4, but this is where it appears most in real work.

```cpp
std::vector<std::string> messages = /* large data */;

for (auto s : messages) { }         // copies every iteration
for (auto & s : messages) { }       // reference (can modify)
for (const auto & s : messages) { } // reference (read only) <- the form that should be your default
```

As we measured with `copies.cpp` in chapter 4, `auto x` copies once per element.
If the elements are `sensor_msgs::msg::Image`, each iteration copies 900KB.

**Make `const auto &` your default.**
Use `auto &` only when you want to modify, and `auto` only when you really want a copy.

The drill has 26 places with `const auto`. Most of them are range-based for.

```cpp
// pattern such as tools/drill_harness.hpp
for (const auto & log : captured_logs) {
  // ...
}
```

### A typical case where a copy by `auto` hurts

```cpp
auto msg = subscription_msg;         // copy of a shared_ptr (count +1, atomic operation)
const auto & msg = subscription_msg; // reference (count unchanged)
```

As in chapter 6, copying a `shared_ptr` involves an atomic count change.
In a loop that runs at high frequency, this is a difference you cannot ignore.

## 8.3 Where it is better not to use `auto`

**Write the type when "the type is important information for the reader".**

```cpp
auto count = get_count();       // int? size_t? double?
size_t count = get_count();     // you can tell by reading
```

Integer types are especially dangerous.

```cpp
auto n = v.size();              // size_t (unsigned)
auto diff = a.size() - b.size();  // subtraction of size_t -> does not become negative, becomes a huge value
```

```cpp
// size_diff.cpp
#include <iostream>
#include <vector>

int main()
{
  std::vector<int> a{1, 2};
  std::vector<int> b{1, 2, 3, 4, 5};
  auto diff = a.size() - b.size();
  std::cout << diff << "\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/Y9fanjsfP)

<!-- measure: files=size_diff.cpp cmd="g++ -std=c++17 -Wall -Wextra size_diff.cpp -o size_diff && ./size_diff" -->
```
18446744073709551613
```

`2 - 5 = -3` should be the result, but `size_t` is unsigned, so **it wraps around to a huge positive value.**

This is more a property of unsigned integers than the fault of `auto`, but
the problem is that **`auto` hides the fact that the type is unsigned**.

```cpp
std::ptrdiff_t diff = static_cast<std::ptrdiff_t>(a.size()) - static_cast<std::ptrdiff_t>(b.size());
```

`-Wall -Wextra` warns about this kind of thing with `-Wsign-compare`,
but the subtraction case above gives no warning. **Be careful when you take the difference of sizes.**

### `auto` and initializer lists

```cpp
auto a = 1;        // int
auto b = 1.0;      // double
auto c = {1, 2};   // std::initializer_list<int> (!)
auto d{1};         // int (C++17 and later)
```

It is worth remembering that `auto c = {1, 2};` becomes a `std::initializer_list`.
It is the same kind of trap as the `std::vector<int> b{3, 0};` story in chapter 2.

## 8.4 Structured bindings (C++17)

A relative of `auto`. You can receive several values at once.

```cpp
// structured.cpp
#include <iostream>
#include <map>
#include <string>

int main()
{
  std::map<std::string, int> params{{"speed", 10}, {"accel", 3}};

  for (const auto & [name, value] : params) {
    std::cout << name << " = " << value << "\n";
  }
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/73xjn9ann)

<!-- measure: files=structured.cpp cmd="g++ -std=c++17 -Wall -Wextra structured.cpp -o structured && ./structured" -->
```
accel = 3
speed = 10
```

Before this existed, you wrote `it->first` / `it->second`.
**The form `const auto & [a, b]` also avoids copies**, so the same principle as range-based for applies.

You can also use it to receive the result of a function that returns a `std::pair`.

```cpp
auto [ok, value] = try_get_parameter("speed");
```

Structured bindings do not appear in the drill, but you need them when you read C++17 code.

## 8.5 `decltype` — take the type of an expression

`auto` "decides the type from the initializer",
but `decltype` takes "the type of the expression itself".

```cpp
int a = 1;
const int & r = a;

auto x = r;              // int (const and & are dropped)
decltype(r) y = a;       // const int & (kept as is)
```

**`decltype` does not drop anything.**
You need it when you write templates, but it does not appear in the drill.
It is enough to know the difference from `auto`.

There is also the form `decltype(auto)`, which means "use the rules of `decltype` with the syntax of `auto`".
You will almost never use it outside of library implementations.

## Try it yourself

**You will confirm by measurement that `auto` makes copies.**

```cpp
// autocopy.cpp
#include <iostream>
#include <string>
#include <vector>

struct Big
{
  std::string tag;
  std::vector<int> payload;

  explicit Big(std::string t) : tag(std::move(t)), payload(1000, 0) {}
  Big(const Big & o) : tag(o.tag), payload(o.payload)
  {
    std::cout << "  [copy " << tag << "]\n";
  }
  Big & operator=(const Big &) = default;
};

int main()
{
  std::vector<Big> items;
  items.reserve(3);                 // prevent copies caused by reallocation
  items.emplace_back("a");
  items.emplace_back("b");
  items.emplace_back("c");

  std::cout << "-- for (auto x : items)\n";
  for (auto x : items) { (void)x; }

  std::cout << "-- for (auto & x : items)\n";
  for (auto & x : items) { (void)x; }

  std::cout << "-- for (const auto & x : items)\n";
  for (const auto & x : items) { (void)x; }

  std::cout << "-- auto y = items[0]\n";
  auto y = items[0];
  (void)y;

  std::cout << "-- const auto & z = items[0]\n";
  const auto & z = items[0];
  (void)z;

  std::cout << "-- the unsigned trap\n";
  std::vector<int> p{1, 2};
  std::vector<int> q{1, 2, 3, 4, 5};
  auto diff = p.size() - q.size();
  std::cout << "  auto diff = " << diff << "\n";
  std::cout << "  correct = "
            << static_cast<std::ptrdiff_t>(p.size()) - static_cast<std::ptrdiff_t>(q.size())
            << "\n";
  return 0;
}
```

**Predict: how many times in total is `[copy ...]` printed? From which lines?**

```bash
g++ -std=c++17 -Wall -Wextra autocopy.cpp -o autocopy && ./autocopy
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/T5rz8jarj)

Check 3 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. 3 times for `for (auto x : items)`, and 0 times for `auto &` and `const auto &`
2. 1 time for `auto y = items[0]`, and 0 times for `const auto & z`
3. `auto diff` becomes `18446744073709551613`

</details>

Delete `items.reserve(3)` and run it again.
**Every `emplace_back` reallocates, and `[copy]` increases.**
When `std::vector` goes over its capacity, it allocates a new area and moves all elements.
`Big` has no move constructor (because you wrote a copy constructor, it is not
generated automatically. The Rule of Five in chapter 5), so the elements are copied.

**Remember the table in chapter 5.** The moment you wrote `Big(const Big &)`,
the move constructor stopped being generated.
Add `Big(Big &&) noexcept = default;` and check that `[copy]` disappears.

## Common pitfalls

**The contents do not change after I used `auto`**
`auto x = container[i];` receives a copy. Use `auto & x`.

**`error: assignment of read-only reference ‘x’` (inside a range-based for)**
You receive it with `const auto &` but try to modify it. Use `auto &`.

**I want to put `nullptr` in an `auto` variable**
`auto p = nullptr;` becomes the special type `std::nullptr_t`, and you cannot assign to it later.
Write the type.

**`error: unable to deduce ‘auto’ from ...`**
There is no initializer, or it has a form that cannot be deduced. You cannot write `auto x;`.

**A warning appears when comparing unsigned integers**

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// signcompare.cpp
#include <iostream>
#include <vector>

int main()
{
  std::vector<int> v{10, 20, 30};

  for (int i = 0; i < v.size(); ++i) {
    std::cout << v[i] << "\n";
  }
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic signcompare.cpp -o signcompare && ./signcompare
```

</details>

<!-- measure: files=signcompare.cpp -->
```
signcompare.cpp: In function ‘int main()’:
signcompare.cpp:9:21: warning: comparison of integer expressions of different signedness: ‘int’ and ‘std::vector<int>::size_type’ {aka ‘long unsigned int’} [-Wsign-compare]
    9 |   for (int i = 0; i < v.size(); ++i) {
      |                   ~~^~~~~~~~~~
10
20
30
```

You are comparing `int i` with `v.size()`. Make it `size_t i`, or align them
explicitly with `static_cast`. `-Wall -Wextra` is enabled for all drill exercises,
so you will really see this.

**`auto` became a `std::initializer_list`**
This is the form `auto c = {1, 2};`. Write the type, as in `auto c = std::vector<int>{1, 2};`.

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 01 | `auto message = std_msgs::msg::String();` |
| 01, 02 | Receiving the return value of `create_publisher` / `create_subscription` with `auto` is also fine (but keep it in a member. Chapter 6) |
| 04, 05 | `auto request = std::make_shared<AddTwoInts::Request>();` |
| 05 | `auto future = client_->async_send_request(request);` — the type of the future is long, so `auto` is practical |
| 10 | `auto result = std::make_shared<Fibonacci::Result>();` |
| 13 | `auto future = client_->async_send_request(request);` |
| 14 | `auto msg = std::make_unique<std_msgs::msg::String>();` |
| Tests | `const auto &` appears 26 times. Mostly range-based for |

Exercise 05 is the best example of why `auto` is necessary.
If you write the return type of `async_send_request` by hand, it looks like this.

```cpp
std::shared_future<std::shared_ptr<example_interfaces::srv::AddTwoInts_Response_<std::allocator<void>>>>
```

It is over 100 characters. You have no choice but to use `auto`.

```cpp
auto future = client_->async_send_request(request);
```

**This line does not use `auto` as "a shortcut to avoid writing the type".
It is a legitimate use: handling "a type you cannot or should not write".**

## References

- [ROS 2 coding conventions](../ros2_coding_conventions.md) — 100-character line limit. The reason `auto` is practically required
- `cppreference` [Placeholder type specifiers (auto)](https://en.cppreference.com/w/cpp/language/auto) and [Structured binding](https://en.cppreference.com/w/cpp/language/structured_binding)

---

Previous chapter → [7. Lambdas and `std::bind`](07_lambdas_and_std_bind.md)
Next chapter → [9. Reading templates](09_reading_templates.md)
