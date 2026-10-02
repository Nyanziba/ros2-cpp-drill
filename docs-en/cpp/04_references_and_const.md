# 4. References and const — which to use in rclcpp

> **Goal of this chapter**: The function signature you write in exercise `cpp04` is this.
>
> ```cpp
> void MinimalSubscriber::topic_callback(const std_msgs::msg::String & msg) const
> ```
>
> **How to read** `const` and `&` was covered in [C++ Basics](../cpp-basics/README.md).
> This chapter is the step after "I can read it": **"which one to choose in rclcpp".**
> We measure the cost of copying, look at the cases where you must not return a reference,
> and learn to choose the argument type of a subscription callback from 4 options.

## 4.1 Prerequisites — what C++ Basics covered

**This chapter is written on the assumption that you can read the following 3 things.**
If you are not sure, go back to [C++ Basics](../cpp-basics/README.md). It takes 1 to 2 hours to fill the gap.

| Prerequisite | Where in C++ Basics |
| --- | --- |
| The procedure for reading a declaration from right to left (reading `const int * const p` aloud) | [Chapter 1 Reading declarations](../cpp-basics/01_reading_declarations.md) |
| What a reference is (an alias, cannot be reseated, cannot be null) | [Chapter 3 References](../cpp-basics/03_references.md) |
| What a pointer is (`nullptr`, dereference, when to use a pointer or a reference) | [Chapter 4](../cpp-basics/04_pointers_1.md), [Chapter 5](../cpp-basics/05_pointers_2.md) |
| That `const` changes meaning in 4 places | [Chapter 6 const](../cpp-basics/06_const.md) |

We repeat here only the quick-reference table that is handy to keep nearby.

```cpp
void MinimalSubscriber::topic_callback(const std_msgs::msg::String & msg) const
//                                     ^^^^^ ①                          ^^^^^ ②
```

| Position | Meaning |
| --- | --- |
| ① `const &` in the argument | Borrow but do not change. No copy happens |
| ② `const` after `)` | This member function does not change the member variables |
| `const int * p` | The pointed-to value is `const`. `p` can be replaced |
| `int * const p` | The pointer itself is `const`. `*p` can be changed |

**`const` attaches to "the thing immediately to its left".** If there is nothing on the left, it attaches to the right.
So `const int *` and `int const *` have the same meaning.

From here, it is about **how to use this foundation in rclcpp.**

## 4.2 Measure the cost of copying

Even if someone says "passing by value makes a copy", without a feel for it, it is not a reason to use `const &`.
Let us measure.

```cpp
#include <iostream>
#include <string>
#include <vector>

struct Tracked
{
  std::vector<int> data;

  Tracked() : data(1000, 0) { std::cout << "  [created]\n"; }
  Tracked(const Tracked & other) : data(other.data) { std::cout << "  [copied]\n"; }
  Tracked & operator=(const Tracked & other)
  {
    data = other.data;
    std::cout << "  [copy-assigned]\n";
    return *this;
  }
};

void by_value(Tracked t) { (void)t; }
void by_const_ref(const Tracked & t) { (void)t; }

int main()
{
  std::cout << "create:\n";
  Tracked t;

  std::cout << "by_value(t):\n";
  by_value(t);

  std::cout << "by_const_ref(t):\n";
  by_const_ref(t);

  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/ofvPGGPaj)

```
create:
  [created]
by_value(t):
  [copied]
by_const_ref(t):
```

**`by_const_ref` prints nothing.** This means no copy happened even once.
`data` is 1000 `int`s, so 4KB. Passing by value does a 4KB heap allocation and copy every time.

Think of ROS messages: a `sensor_msgs::msg::Image` is about 900KB even at VGA, and
a `PointCloud2` is several MB.
**If a subscription callback is called at 30Hz, passing by value is 27MB of copying per second.**
This is the reason to use `const &`.

### What should be `const &`

The rule of thumb is simple.

| Type | How to pass |
| --- | --- |
| `int` / `double` / `bool` / pointer / `enum` | **Pass by value** (faster than `const &`) |
| `std::string` / `std::vector` / ROS messages / your own classes | **Pass by `const &`** |
| You want to hand over ownership | `std::unique_ptr` by value (Chapters 5 and 6) |

Making an `int` a `const int &` is counterproductive.
An `int` is 4 bytes and fits in a register, but a reference passes an address (8 bytes) and
adds an indirection: reading the referent. **For small types, a value is faster.**

If you remember "values up to about 2 pointers' worth (16 bytes)", that is enough in practice.

This is the judgment behind `limit_velocity` of exercise 11 taking all `double`s by value.

```cpp
double limit_velocity(double target, double previous, double max_speed, double max_delta);
```

## 4.3 Dangling references — when you must not return a reference

A reference is an "alias", so **if the original dies, the reference becomes invalid.**

```cpp
#include <iostream>
#include <string>

const std::string & bad()
{
  std::string local = "danger";
  return local;             // local disappears when we leave this function
}

int main()
{
  const std::string & r = bad();
  std::cout << r << "\n";   // undefined behavior
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/7Kn6Pa77a)

g++ warns you.

```
warning: reference to local variable ‘local’ returned [-Wreturn-local-addr]
```

The scary part is that **it ends with only a warning**. When you run it, it may even work by chance.

Returning a reference to a member is the correct use. It is valid as long as the object is alive.

```cpp
const std::string & name() const { return name_; }   // OK
```

But it is dangerous if the caller keeps it.

```cpp
const std::string & n = make_node()->name();   // the node dies immediately
```

**Do not hold a reference for a long time.** Use it only on the spot,
and if you want to keep it, copy it or use a `shared_ptr` (Chapter 6).
rclcpp returns a `shared_ptr` for everything partly to enforce this judgment with the type.

### `const auto &` is a safe default

This is a form often used in a range-for.

```cpp
for (const auto & s : sensors) {
  s->report();
}
```

`const auto &` means "do not copy, do not change", so
**when in doubt, use this** and you lose nothing (Chapter 8 covers `auto`).

If you write `for (auto s : sensors)`, it copies the elements.
With `std::unique_ptr` it cannot be copied, so you get a compile error and
you notice, but with `std::string` it is copied silently.

## 4.4 Which callback argument to choose in rclcpp

Several forms can be used for the argument of a subscription callback. Two appear in the drill.

```cpp
// exercise 02
void topic_callback(const std_msgs::msg::String & msg) const;

// exercise 14
void topic_callback(std_msgs::msg::String::ConstSharedPtr msg);
```

The criterion for choosing is **"how do you want to handle that message".**

| Argument type | When to use | Copy |
| --- | --- | --- |
| `const T & msg` | You only read it on the spot | Does not happen |
| `T::ConstSharedPtr msg` | You want to keep it / you want zero-copy to work | Does not happen |
| `T::SharedPtr msg` | You want to modify it (shared with other subscribers) | May happen |
| `T::UniquePtr msg` | You want to modify it (sole ownership) | May happen |

**A copy does not happen with `const T &` either.** You can confirm this by measurement:
when intra-process communication is enabled, the address of the message is the same on the sender side and the receiver side.

You may see the explanation "receiving by value makes a copy", but
`any_subscription_callback.hpp` does not support the receive-by-value form in the first place,
so you do not need to worry about it.

If you want to keep it, use `ConstSharedPtr`. If you try to keep a `const T &`,
you get the dangling reference of Section 4.3.

## Try it yourself

**Let us count how many times a copy happens.**

```cpp
// copies.cpp
#include <iostream>
#include <string>
#include <vector>

struct Msg
{
  std::string data;

  Msg() = default;
  explicit Msg(std::string s) : data(std::move(s)) {}
  Msg(const Msg & o) : data(o.data) { std::cout << "  copy\n"; }
  Msg & operator=(const Msg & o) { data = o.data; std::cout << "  copy=\n"; return *this; }
};

Msg g_last;

void log_only(const Msg & m)      { std::cout << "  read: " << m.data << "\n"; }
void log_value(Msg m)             { std::cout << "  read: " << m.data << "\n"; }
void keep_it(const Msg & m)       { g_last = m; }          // we keep it, so a copy is needed

int main()
{
  Msg m("hello");

  std::cout << "log_only(m):\n";  log_only(m);
  std::cout << "log_value(m):\n"; log_value(m);
  std::cout << "keep_it(m):\n";   keep_it(m);

  std::vector<Msg> v;
  v.push_back(m);
  std::cout << "-- range for (copying version)\n";
  for (auto x : v) { (void)x; }
  std::cout << "-- range for (const auto &)\n";
  for (const auto & x : v) { (void)x; }
  return 0;
}
```

**Predict: how many times in total are `copy` and `copy=` printed? From which lines?**

```bash
g++ -std=c++17 -Wall -Wextra copies.cpp -o copies && ./copies
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/7Wjvqszh5)

<details markdown="1"><summary>Answer (actual output)</summary>

```
log_only(m):
  read: hello
log_value(m):
  copy
  read: hello
keep_it(m):
  copy=
  copy
-- range for (copying version)
  copy
-- range for (const auto &)
```

</details>

Check 4 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. **`log_only` is 0 times and `log_value` is 1 time.** They only read the same `m` in the same way, and
   the single character `&` decides whether a copy happens
2. `keep_it` receives by `const &`, but `copy=` is printed.
   → **If you keep it, a copy is always needed somewhere.** `const &` makes only "passing" free, and
   "saving" is not free
3. The `copy` right after `keep_it` is not from `keep_it`; it is from `v.push_back(m)`
   on the next lines. **Be careful not to misjudge the cause because of the order of the output.**
   If it is confusing, put one `std::cout` line before `push_back` to check
4. The range-for makes 1 copy with `auto x` and 0 copies with `const auto & x`

</details>

Next, let us break `const` and see the error. Change the body of `log_only` to the following.

```cpp
void log_only(const Msg & m) { m.data = "overwrite"; }
```

Check the error message (`assignment of member ... in read-only object`).

Finally, let us create a dangling reference.

```cpp
const std::string & dangling()
{
  std::string local = "danger";
  return local;
}
```

Call it from `main` and print it.
**Only one warning appears, and the compile passes.**
Also see whether the result changes each time you run it (or does not change).

## Common pitfalls

**`error: binding reference of type ‘T&’ to ‘const T’ discards qualifiers`**
You are trying to receive a `const` thing with a non-`const` reference.
Make the receiving side `const T &`, or if you really want to change it, remove the `const` on the caller side.

**`error: passing ‘const Foo’ as ‘this’ argument discards qualifiers`**
You are calling a non-`const` member function from inside a `const` member function.
Add `const` to the called function too, or remove the `const` of the caller.
**When you are told "`const` is missing", first think whether you can add it to the called function.**

**`error: cannot bind non-const lvalue reference to an rvalue`**

```cpp
void f(std::string & s);
f("hello");           // error
f(std::string("hi")); // error
```

```
error: cannot bind non-const lvalue reference of type ‘std::string&’ to an rvalue of type ‘std::string’
```

A temporary object (an rvalue) cannot be bound to a non-`const` reference.
With `const std::string & s` it passes. **This is also one of the reasons to "make `const &` the default".**
It can accept both rvalues and lvalues.

This word "rvalue" is the main player of the next chapter.

**After adding `const`, you can no longer cache a member**
A member marked `mutable` can be changed even from a `const` member function.

```cpp
mutable std::mutex mutex_;   // a legitimate example that is often used
```

A lock does not "logically change the state", so `mutable` is appropriate.
If you want to use `mutable` for anything else, it is a sign to review the design.

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 02 | `void topic_callback(const std_msgs::msg::String & msg) const` — two `const`s with different meanings |
| 04, 05 | The service's `Request::SharedPtr` / `Response::SharedPtr`. **They have no `const`** (because the response is modified) |
| 06, 07 | The form that receives the return value of `get_parameter` |
| 11 | `limit_velocity(double, double, double, double)` — all by value. For small types, a value is correct |
| 12 | Build a QoS object with a method chain |
| 13 | `const std_msgs::msg::Int32 & msg` |
| 14 | `ConstSharedPtr` — we want to keep it, so a smart pointer, not a reference |
| 14, 15 | `const rclcpp::NodeOptions & options` — a large settings object, so `const &` |

If you look at the service callback of exercise 04, you can see the judgment of this chapter.

```cpp
void add_two_ints(
  const std::shared_ptr<AddTwoInts::Request> request,
  std::shared_ptr<AddTwoInts::Response> response)
```

**`request` has `const` and `response` does not.**
The request is only read, and the response is written. The signature expresses that.

## Matching exercise

After you read this chapter, practice with the matching drill.

- `cpp04_reference_const` — receive with a reference and const

```bash
./drill run cpp04
```

From the exercise side, you can come back to this chapter with `./drill read`.

## References

- [ROS 2 coding conventions](../ros2_coding_conventions.md) — write a pointer as `char * c` (not `char* c`)
- [rclcpp design philosophy](../rclcpp_design_philosophy.md) Chapter 6 — the relation between the argument type of a subscription callback and zero-copy
- `cppreference` [Reference declaration](https://en.cppreference.com/w/cpp/language/reference) and [const qualifier](https://en.cppreference.com/w/cpp/language/cv)

---

Previous → [3. Inheritance](03_inheritance.md)
Next → [5. Move and ownership](05_move_and_ownership.md)
