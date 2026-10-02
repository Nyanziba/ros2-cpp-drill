# 9. Reading templates

> **Goal of this chapter**: If you grep `exercises/` and `solutions/` in the drill,
> there are **0 places where you write a `template` yourself**.
> Even so, `<>` such as `create_publisher<std_msgs::msg::String>` appears in every exercise.
> **This chapter is not about "learning to write templates". It is about "learning to read them".**
> There are only 3 goals. To be able to read out what `<>` decides,
> to understand type members such as `::SharedPtr`, and
> **to find the 1 real cause line in a 100-line template error message.**

## 9.1 A template is "a function that takes a type as an argument"

First, write just one yourself. This is enough groundwork for reading.

```cpp
// larger.cpp
#include <iostream>
#include <string>

template<typename T>
T larger(T a, T b)
{
  return (a > b) ? a : b;
}

int main()
{
  std::cout << larger(3, 7) << "\n";              // int version
  std::cout << larger(1.5, 0.5) << "\n";          // double version
  std::cout << larger<std::string>("a", "b") << "\n";  // explicit
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/T3qq17cvv)

<!-- measure: files=larger.cpp cmd="g++ -std=c++17 larger.cpp -o larger && ./larger" -->
```
7
1.5
b
```

**`larger` is not one function. 3 separate functions are generated.**
They are `larger<int>`, `larger<double>`, and `larger<std::string>`.

This is called **instantiation**.
At compile time the code is duplicated, and `T` is replaced by a concrete type.

So

- **the runtime cost is zero** (unlike `virtual`, it is not an indirect call)
- **compile time and binary size increase** (code grows for each type you use)
- **errors appear at instantiation** (if you do not use it, there is no error)

The last property is the cause of those hard-to-read error messages.

### A type can be deduced or stated explicitly

```cpp
larger(3, 7);                  // T = int is deduced from the arguments
larger<std::string>("a", "b"); // state T explicitly ("a" is a const char *, so deduction would be a problem)
```

**It cannot be deduced from the return type.** A type that does not appear in the arguments must be stated explicitly.

This is why you write `<>` in rclcpp.

```cpp
this->create_publisher<std_msgs::msg::String>("topic", 10);
```

The arguments are only `"topic"` and `10`, so **the message type appears nowhere.**
So you have no choice but to state it.

On the other hand, `publish` can tell from its argument, so it does not need `<>`.

```cpp
publisher_->publish(message);   // decided from the type of message
```

## 9.2 Reading out the `<>` in rclcpp

Here are all the forms that appear in the drill.

```cpp
this->create_publisher<std_msgs::msg::String>("topic", 10);
this->create_subscription<std_msgs::msg::String>("topic", 10, callback);
this->create_service<example_interfaces::srv::AddTwoInts>("add_two_ints", callback);
this->create_client<example_interfaces::srv::AddTwoInts>("add_two_ints");
rclcpp_action::create_server<example_interfaces::action::Fibonacci>(...);
std::make_shared<MinimalPublisher>();
std::make_unique<std_msgs::msg::String>();
```

**There is one rule. Inside `<>` is "what it handles".**

| Function | What goes in `<>` |
| --- | --- |
| `create_publisher<T>` | the type of message to send |
| `create_subscription<T>` | the type of message to receive |
| `create_service<T>` / `create_client<T>` | the service type (`.srv`) |
| `create_server<T>` (action) | the action type (`.action`) |
| `make_shared<T>` / `make_unique<T>` | **the type of object to create** |

Note that the last 2 mean something different from the others.
The `<>` of `make_shared<MinimalPublisher>()` is not "the type it handles" but "the type it creates".

### Reading the return type

The return value of `create_publisher<std_msgs::msg::String>` is this.

```cpp
std::shared_ptr<rclcpp::Publisher<std_msgs::msg::String>>
```

**The `<>` are nested.** Read from the inside.

1. `rclcpp::Publisher<std_msgs::msg::String>` — a Publisher that sends String
2. `std::shared_ptr<that>` — a shared_ptr to it

When you keep it as a member, write it with the type alias from chapter 6.

```cpp
rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
```

**`::SharedPtr` is "an alias for a type defined inside that class".**
It is not a function. Inside `Publisher<T>` there is

```cpp
using SharedPtr = std::shared_ptr<Publisher<T>>;
```

so you can write `Publisher<T>::SharedPtr`.
The [coding conventions](../ros2_coding_conventions.md) say "write nested templates as `set<list<string>>`"
(no space between the `>`), because this kind of nesting appears every day.

## 9.3 `typename` and `::` — reading type members

This is the most confusing part when you read templates.

`Foo::Bar` is one of 3 things.

```cpp
Foo::Bar        // ① static member variable
Foo::Bar()      // ② static member function
Foo::Bar        // ③ type alias (using / typedef / nested class)
```

**They are written the same, so you can only tell them apart by context.**
A human decides "it starts with a capital letter and is not used like a variable, so it is probably a type".

The compiler also cannot decide inside a template. So you add `typename`.

```cpp
template<typename T>
void f()
{
  typename T::SharedPtr p;    // tells the compiler "T::SharedPtr is a type"
}
```

**This `typename` is needed only inside templates.**
When the concrete type is already known, you do not need it.

```cpp
rclcpp::Publisher<std_msgs::msg::String>::SharedPtr p;   // typename is not needed
```

So **`typename` does not appear in the drill code.** It appears a lot when you read rclcpp headers.
It is enough to skip it as "it only says that this is a type".

### Members that grow on message types

A type generated from a `.msg` gets a fixed set of members.

```cpp
std_msgs::msg::String
  ├── ::SharedPtr           std::shared_ptr<String>
  ├── ::ConstSharedPtr      std::shared_ptr<const String>
  ├── ::UniquePtr           std::unique_ptr<String>
  └── data                  field (the one defined in the .msg)
```

When you write a `.msg` in exercise 03, the fields grow as members like this.

```
# Num.msg
int64 num
```

```cpp
auto msg = drill_03_custom_interface::msg::Num();
msg.num = 42;        // <- the field from the .msg is there as is
```

**If you change the `.msg`, the fields do not appear until you rebuild.**
In exercise 03, the build fails with many "has no member named ..." errors
because the `.msg` is unfinished and the fields have not been generated.
The drill runner prints a special message for this case.

```
This exercise does not build until the interface definition (.msg / .srv)
is finished. The fields do not exist in the C++ types yet, so you will
see many "has no member named ..." errors.
```

## 9.4 How to read error messages

**This is the most practical part of this chapter.**

Template errors are long. First, let us make a real one.

```cpp
// sorterr.cpp
#include <algorithm>
#include <vector>

struct Point { int x; int y; };

int main()
{
  std::vector<Point> v{{1, 2}, {3, 4}};
  std::sort(v.begin(), v.end());     // Point has no <
  return 0;
}
```

```bash
g++ -std=c++17 sorterr.cpp -o sorterr 2>&1 | wc -l
```

<!-- measure: files=sorterr.cpp -->
```
78
```

[⚠ See this error in your browser (gcc 13.3)](https://godbolt.org/z/9d56fKxaM)

It prints **78 lines**. But the real cause is just one: "`Point` has no `operator<`".

Let us fix a procedure for reading.

**Step 1: Extract only the lines that start with `error:`.**

```bash
g++ -std=c++17 sorterr.cpp 2>&1 | grep "error:"
```

<!-- measure: files=sorterr.cpp -->
```
/usr/include/c++/13/bits/predefined_ops.h:45:23: error: no match for ‘operator<’ (operand types are ‘Point’ and ‘Point’)
/usr/include/c++/13/bits/predefined_ops.h:98:22: error: no match for ‘operator<’ (operand types are ‘Point’ and ‘Point’)
/usr/include/c++/13/bits/predefined_ops.h:69:22: error: no match for ‘operator<’ (operand types are ‘Point’ and ‘Point’)
```

**It became 3 lines.** All 3 have the same cause (`Point` has no `operator<`).
The other 75 lines are `note:` (a list of candidates), `In instantiation of` (the call path), and so on.

**Step 2: Find the lines that show your own file name.**

```bash
g++ -std=c++17 sorterr.cpp 2>&1 | grep "sorterr.cpp"
```

<!-- measure: files=sorterr.cpp -->
```
                 from sorterr.cpp:2:
sorterr.cpp:10:12:   required from here
sorterr.cpp:10:12:   required from here
sorterr.cpp:10:12:   required from here
```

**`required from here` tells you "which line of your code is the trigger".**
It is the `std::sort` on line 10.

**Step 3: Follow `required from here` from bottom to top.**
The error appears "deep inside the library", but if you follow the chain of `In instantiation of`,
you come back to your own code. **The bottom-most line that shows your own file name is the starting point.**

In summary:

| What you want | Command |
| --- | --- |
| See the real cause | `2>&1 \| grep "error:"` |
| Which line of your code is the trigger | `2>&1 \| grep "your_file.cpp"` |
| See only the first error | `-fmax-errors=1` |
| Limit the depth of template expansion | `-ftemplate-backtrace-limit=0` (show all) / `=1` (only 1 level) |

**`-fmax-errors=1` is worth remembering.**
Often, fixing only the first error makes the rest disappear in a chain,
so 78 lines become 28 lines, which is easier to read.

```bash
g++ -std=c++17 -fmax-errors=1 sorterr.cpp
```

### Template errors that often appear in rclcpp

**① The message type and the callback argument type do not match**

```cpp
this->create_subscription<std_msgs::msg::String>(
  "topic", 10,
  [this](const std_msgs::msg::Int32 & msg) { });   // it became Int32
```

The error comes out as dozens of lines from inside `create_subscription`.
If you narrow it down with `grep "error:"`, you see `no matching function for call to ...`,
and **just comparing the type in `<>` with the lambda argument type** fixes it.

**② The number of `_1` does not match** (chapter 7)

This is the 22-line error of `std::bind`. You can narrow it down with the same procedure.

**③ A missing header include**

```cpp
this->create_publisher<std_msgs::msg::String>("topic", 10);
```

If you forget `#include "std_msgs/msg/string.hpp"`,
you get `incomplete type` or `has not been declared`.
**If you pass an incomplete type to the `<>` of a template, the error appears at the place of instantiation**,
so it may appear far from the line you wrote.

## 9.5 Related features that help you read

You do not need to write them yourself, but they appear in rclcpp headers, so here are their meanings.

| Notation | Meaning | Where you see it |
| --- | --- | --- |
| `template<typename T, typename Alloc = std::allocator<void>>` | default template argument | the declaration of `create_publisher` |
| `template<typename... Args>` | variadic template | the implementation of `make_shared` |
| `std::enable_if_t<...>` | enabled only for types that meet a condition | overload selection in rclcpp |
| `static_assert(condition, "message")` | compile-time check | validation of message types |
| `constexpr` | computed at compile time | constants of `rclcpp::QoS` |
| `std::remove_reference_t<T>` | transforms a type (type trait) | the implementation of `std::move` (chapter 5) |

**`static_assert` is useful for your own code too.**
For example, if you use `static_assert` to mechanically guarantee that the value of an error code you defined
matches the definition on the side of an external library such as `move_base_flex`,
you notice at compile time when only one of them changes.

```cpp
static_assert(ec::kNoPathFound == GetPathResult::NO_PATH_FOUND);
```

**You can detect it at compile time without running anything**, so it is very effective for things like tables that map constants.

## Try it yourself

**You will practice narrowing a 78-line error down to 3 lines.** This is the skill you use most in real work.

```cpp
// tmplerr.cpp
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

struct Point
{
  int x;
  int y;
};

int main()
{
  std::vector<Point> v{{3, 4}, {1, 2}};
  std::sort(v.begin(), v.end());
  for (const auto & p : v) {
    std::cout << p.x << "," << p.y << "\n";
  }
  return 0;
}
```

First, print it as is and count the lines.

```bash
g++ -std=c++17 tmplerr.cpp -o tmplerr 2>&1 | wc -l
```

[⚠ See this error in your browser (gcc 13.3)](https://godbolt.org/z/qWzxsEjaG)

Next, try the 3 ways to narrow it down.

```bash
g++ -std=c++17 tmplerr.cpp 2>&1 | grep "error:"
g++ -std=c++17 tmplerr.cpp 2>&1 | grep "tmplerr.cpp"
g++ -std=c++17 -fmax-errors=1 tmplerr.cpp 2>&1 | wc -l
```

**Check that the result of `grep "error:"` is 3 lines (all with the same cause).**
In the 78 lines of output, those 3 lines are the only meaningful information.

There are 2 ways to fix it. Try both.

```cpp
// ① pass a comparison function
std::sort(v.begin(), v.end(), [](const Point & a, const Point & b) { return a.x < b.x; });

// ② define operator<
bool operator<(const Point & a, const Point & b) { return a.x < b.x; }
```

**② is the fix most faithful to the error message** (it said "`operator<` is missing", so we add it).
But in real work, ① is often the better choice.
When you doubt "does this type have one natural order", it is safer to state the comparison function explicitly.

Here is one more. **Let us look at the error when you get a type wrong.**

```cpp
std::map<std::string, int> m;
m.insert({1, "one"});      // key and value are swapped
```

Again, narrow it down with `grep "error:"` before reading.
You will see `no matching function for call to ‘std::map<...>::insert(...)’`.

## Common pitfalls

**The error is 100 lines and you lose the will to read it**
Do `grep "error:"` first. This is section 9.4 of this chapter.
If you still do not understand, add `-fmax-errors=1`.

**`error: ‘SharedPtr’ in ‘class rclcpp::Publisher<...>’ does not name a type`**
This is a context where `typename` is needed (inside a template).
If you see this in drill code, you most likely got the type inside `<>` wrong.

**`error: template argument 1 is invalid`**
You wrote something that is not a type (such as a variable name) inside `<>`.

**`error: wrong number of template arguments`**
The count inside `<>` is wrong. `create_publisher<T>` takes 1.

**`undefined reference` for a template function**
You wrote the template definition in a `.cpp` file.
**A template cannot be instantiated unless its definition is visible where it is used**,
so write the definition in the `.hpp` (this is the ODR exception from chapter 1. You do not need to add `inline`).

**It says a member of a message type does not exist**
You changed the `.msg` but did not rebuild, or the `.msg` is unfinished (exercise 03).
Run `rm -rf build install log` and start again.

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 01 | `create_publisher<std_msgs::msg::String>` / `Publisher<T>::SharedPtr` |
| 02 | `create_subscription<std_msgs::msg::String>` |
| 03 | **Types are generated from `.msg` / `.srv`.** Fields grow as members |
| 04, 05 | `create_service<T>` / `create_client<T>`. `T::Request` / `T::Response` |
| 10 | `create_server<Fibonacci>`. `Fibonacci::Goal` / `::Feedback` / `::Result` |
| 11 | **An exercise where no template appears at all.** You can write it with `double` only |
| 12 | Method chaining of `rclcpp::QoS` (not a template) |
| 14 | `make_unique<std_msgs::msg::String>` / `String::ConstSharedPtr` |

Exercise 04 is the one where "a type inside a type" appears the most.

```cpp
void add_two_ints(
  const std::shared_ptr<example_interfaces::srv::AddTwoInts::Request> request,
  std::shared_ptr<example_interfaces::srv::AddTwoInts::Response> response)
{
  response->sum = request->a + request->b;
}
```

`AddTwoInts::Request` and `AddTwoInts::Response` are
**2 types generated from the part above and the part below the `---` in the `.srv` file**.

```
# AddTwoInts.srv
int64 a
int64 b
---
int64 sum
```

Above the `---` is `Request` (`a`, `b`), and below is `Response` (`sum`).
Once you see that **the structure of the `.srv` becomes the structure of the types as is**,
`request->a` and `response->sum` are no longer things to memorize.

## References

- [ROS 2 coding conventions](../ros2_coding_conventions.md) — write nested templates as `set<list<string>>` (not `> >`)
- `/opt/ros/jazzy/include/rclcpp/rclcpp/create_publisher.hpp` — the actual template declaration
- `cppreference` [Templates](https://en.cppreference.com/w/cpp/language/templates)
- the g++ manual: `-fmax-errors` / `-ftemplate-backtrace-limit`

---

Previous chapter → [8. `auto` and type deduction](08_auto_and_type_deduction.md)
Next chapter → [10. Operator overloading](10_operator_overloading.md)
