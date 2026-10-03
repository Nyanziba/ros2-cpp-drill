# 7. Lambdas and `std::bind`

> **Goal of this chapter**: In exercise 01 you write these 2 lines.
>
> ```cpp
> timer_ = this->create_wall_timer(
>   500ms, std::bind(&MinimalPublisher::timer_callback, this));
> ```
>
> A function name with `&`, `this`, and in exercise 02 a mysterious variable called `_1`.
> This is "passing a member function as a function", the part of C++ with the most unusual syntax.
> The drill has 56 lambdas, 22 uses of `std::bind`, and 18 uses of `placeholders`.
> **Both are allowed by the ROS 2 conventions**, so you will learn to read both.

## 7.1 A function pointer is not enough

C also has a way to "pass a function".

```cpp
#include <iostream>

void greet() { std::cout << "hello\n"; }

void call_it(void (*f)()) { f(); }

int main()
{
  call_it(greet);
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/sK3Gc3PaY)

If this were enough, you would need neither `std::bind` nor lambdas. It is not enough because **it cannot hold state**.

`MinimalPublisher::timer_callback` needs `this`,
because it touches `count_` and `publisher_`.
But a function pointer has no place to store "whose object".

```cpp
void (*f)() = &MinimalPublisher::timer_callback;   // error
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// member_pointer.cpp

class MinimalPublisher
{
public:
  void timer_callback() {}
};

int main()
{
  void (*f)() = &MinimalPublisher::timer_callback;   // error
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic member_pointer.cpp -o member_pointer
```

</details>

<!-- measure: -->
```
member_pointer.cpp: In function ‘int main()’:
member_pointer.cpp:11:17: error: cannot convert ‘void (MinimalPublisher::*)()’ to ‘void (*)()’ in initialization
   11 |   void (*f)() = &MinimalPublisher::timer_callback;   // error
      |                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
member_pointer.cpp:11:10: warning: unused variable ‘f’ [-Wunused-variable]
   11 |   void (*f)() = &MinimalPublisher::timer_callback;   // error
      |          ^
```

The types are different. `void (MinimalPublisher::*)()` is a different type called a **pointer to member function**.
The type itself says "you need an object to call it".

```cpp
auto f = &MinimalPublisher::timer_callback;
MinimalPublisher obj;
(obj.*f)();          // call it by naming the object explicitly. The syntax is unusual
```

**We want something that bundles "the function" and "the target object" into one.**
`std::bind` and lambdas solve this.

## 7.2 `std::bind` — fill in arguments in advance

`std::bind` is a tool that "bundles a function and some of its arguments in advance".

```cpp
// bind_add.cpp
#include <functional>
#include <iostream>

int add(int a, int b) { return a + b; }

int main()
{
  auto add10 = std::bind(add, 10, std::placeholders::_1);
  std::cout << add10(5) << "\n";     // 15
  std::cout << add10(7) << "\n";     // 17
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/aojxTaYca)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic bind_add.cpp -o bind_add && ./bind_add" -->
```
15
17
```

`std::bind(add, 10, _1)` means "the 1st argument of `add` is fixed to 10,
and the 2nd argument is received at call time".

**`_1` is a placeholder that means "put the 1st argument of the call here".**
It lives in the `std::placeholders` namespace. It is long, so it is customary to write a `using`.

```cpp
using std::placeholders::_1;
```

Exercise 02 has exactly this form.

```cpp
// solutions/02_subscriber/src/minimal_subscriber.cpp
using std::placeholders::_1;

MinimalSubscriber::MinimalSubscriber()
: Node("minimal_subscriber")
{
  subscription_ = this->create_subscription<std_msgs::msg::String>(
    "topic", 10, std::bind(&MinimalSubscriber::topic_callback, this, _1));
}
```

### Binding a member function

There are 3 parts.

```cpp
std::bind(&MinimalSubscriber::topic_callback, this, _1)
//        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^  ^^^^  ^^
//        ① pointer to member function       ② target  ③ slot for the argument
```

**① `&ClassName::function_name`** — The `&` is required. If you forget it, you get an error.

```cpp
std::bind(MinimalSubscriber::topic_callback, this, _1)   // forgot the &
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// bind_noamp.cpp
#include <functional>
#include <string>

using std::placeholders::_1;

void subscribe(std::function<void(const std::string &)> callback) { (void)callback; }

class MinimalSubscriber
{
public:
  void topic_callback(const std::string & msg) { (void)msg; }

  void start()
  {
    subscribe(
      std::bind(MinimalSubscriber::topic_callback, this, _1)   // forgot the &
    );
  }
};

int main()
{
  MinimalSubscriber subscriber;
  subscriber.start();
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic bind_noamp.cpp -o bind_noamp
```

</details>

<!-- measure: -->
```
bind_noamp.cpp: In member function ‘void MinimalSubscriber::start()’:
bind_noamp.cpp:17:36: error: invalid use of non-static member function ‘void MinimalSubscriber::topic_callback(const std::string&)’
   17 |       std::bind(MinimalSubscriber::topic_callback, this, _1)   // forgot the &
      |                 ~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~
bind_noamp.cpp:12:8: note: declared here
   12 |   void topic_callback(const std::string & msg) { (void)msg; }
      |        ^~~~~~~~~~~~~~
```

**② `this`** — "which object's member function".
The implicit 0th argument of a member function is `this`, and this fills it in.
From the point of view of `std::bind`, `this` is treated as "the 1st argument".

**③ `_1`** — the place where the message goes.
`topic_callback(const std_msgs::msg::String & msg)` has 1 argument, so there is 1 `_1`.

A callback with 0 arguments (a timer) does not need `_1`.

```cpp
// exercise 01: timer_callback() takes no arguments
std::bind(&MinimalPublisher::timer_callback, this)
```

A callback with 2 arguments (a service) uses `_1, _2`.

```cpp
// exercise 04
std::bind(&AddTwoIntsServer::add, this, _1, _2)
```

**Make the number of `_N` match the number of arguments passed at call time.**
This is the easiest mistake to make with `std::bind`.

### `std::bind` can also reorder arguments

You will almost never use this, but it helps you understand what `_1` means.

```cpp
auto swapped = std::bind(subtract, _2, _1);
swapped(3, 10);      // subtract(10, 3) is called
```

`_1` means "the 1st argument at call time", so you can write it at any position.

## 7.3 Lambda expressions

Here is the same thing written as a lambda.

```cpp
timer_ = this->create_wall_timer(500ms, [this]() { this->timer_callback(); });
```

Let us break down the syntax.

```cpp
[this](const std_msgs::msg::String & msg) { this->topic_callback(msg); }
//^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//  ①            ②                                    ③
```

| Part | Name | Meaning |
| --- | --- | --- |
| `[this]` | **capture** | Which outer variables you can use |
| `(...)` | parameter list | Same as an ordinary function |
| `{ ... }` | body | Same as an ordinary function |

The return type is deduced. If you want to state it, add `-> type`.

```cpp
[](int x) -> double { return x / 2.0; }
```

### Kinds of capture

This is the heart of lambdas.

```cpp
// capture.cpp
#include <iostream>

int main()
{
  int a = 1;
  int b = 2;

  auto by_copy   = [a]()      { std::cout << "copy   a=" << a << "\n"; };
  auto by_ref    = [&a]()     { std::cout << "ref    a=" << a << "\n"; };
  auto all_copy  = [=]()      { std::cout << "all=   a=" << a << " b=" << b << "\n"; };
  auto all_ref   = [&]()      { std::cout << "all&   a=" << a << " b=" << b << "\n"; };
  auto none      = []()       { std::cout << "none\n"; };

  a = 99;
  b = 99;

  by_copy();
  by_ref();
  all_copy();
  all_ref();
  none();
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/zoPo8444o)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic capture.cpp -o capture && ./capture" -->
```
copy   a=1
ref    a=99
all=   a=1 b=2
all&   a=99 b=99
none
```

**`[a]` (copy) remembers the value at the moment the lambda was created.**
Even if you set `a` to 99 afterwards, it stays 1 inside the lambda.

**`[&a]` (reference) looks at the current `a`.** So it shows 99.

| Capture | Meaning | Danger level |
| --- | --- | --- |
| `[]` | Uses nothing | Safe |
| `[a]` | Copy `a` | Safe |
| `[&a]` | Reference `a` | **Watch the lifetime of `a`** |
| `[=]` | Copy everything it uses | Safe, but `this` is an exception (see below) |
| `[&]` | Reference everything it uses | **Dangerous** |
| `[this]` | Copy the `this` pointer | **Watch the lifetime of the object** |
| `[a = expr]` | Init capture (C++14) | Safe |

### The danger of `[&]` and `[this]`

A lambda that captures by reference breaks if it outlives what it refers to.

```cpp
std::function<void()> make_bad()
{
  int local = 42;
  return [&local]() { std::cout << local; };   // local disappears when this function returns
}

auto f = make_bad();
f();                                            // undefined behavior
```

The same problem as the dangling reference in chapter 4 happens inside the lambda.

**`[this]` needs even more care.** `this` is a pointer,
so even a copy capture only "copies the pointer". The object is shared.

```cpp
class Node
{
public:
  void start()
  {
    timer_ = create_wall_timer(500ms, [this]() { tick(); });
  }
private:
  void tick() { ++count_; }
  int count_ = 0;
};
```

**If the lambda is called after the node is destroyed, it touches a dead `this`.**

In rclcpp, this rarely causes real harm. As in chapter 6,
the timer is owned by the `timer_` member, and when the node dies, `timer_` dies too.
**"A timer never outlives its node", so `[this]` can be used safely.**
This safety is supported by the ownership model of rclcpp. It is not a general guarantee.

**`[=]` does not copy `this`.** It only copies the pointer.
In C++20, writing `[=, this]` explicitly is recommended, and capturing `this` implicitly is deprecated.
Writing `[this]` explicitly is the current practice.

### Init capture — passing a `unique_ptr`

Use this when you capture something you want to move.

```cpp
auto p = std::make_unique<std_msgs::msg::String>();
auto f = [msg = std::move(p)]() { std::cout << msg->data; };
```

`[p]` makes a copy, and that is an error for a `unique_ptr`.
With `[msg = std::move(p)]`, you can move ownership into the lambda (chapter 5).

### `mutable`

A variable captured by copy cannot be modified by default.

```cpp
int count = 0;
auto f = [count]() { ++count; };    // error
auto g = [count]() mutable { ++count; };   // OK (the copy inside the lambda increases)
```

A lambda with `mutable` "changes its own copy every time you call it",
so it becomes a function object that holds state.

## 7.4 `std::function` — a box that holds anything

The type of a lambda has **no name.** It is an anonymous type generated by the compiler.

```cpp
auto f = [](int x) { return x * 2; };   // the type of f is "the type of that lambda"
```

So you can only store it in a variable with `auto`.
This is a problem when you want to **keep it as a member variable** or **receive it as a function argument**.

`std::function<return_type(arguments...)>` is the container for this.

```cpp
// std_function.cpp
#include <functional>
#include <iostream>

void run_twice(const std::function<void(int)> & f)
{
  f(1);
  f(2);
}

int main()
{
  int total = 0;
  run_twice([&total](int x) { total += x; std::cout << "  x=" << x << "\n"; });
  std::cout << "total=" << total << "\n";

  std::function<void(int)> stored = [](int x) { std::cout << "  stored " << x << "\n"; };
  run_twice(stored);
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/b19cvGj1x)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic std_function.cpp -o std_function && ./std_function" -->
```
  x=1
  x=2
total=3
  stored 1
  stored 2
```

`std::function` can hold a lambda, the result of `std::bind`, or a function pointer.
This is why `create_subscription` can accept both.

There is a runtime cost. It erases the type and may put the contents on the heap,
and the call becomes indirect, so it is not inlined.
**Where you can accept a template, `auto` / a template is faster**,
but a few callbacks per control cycle is negligible.

## 7.5 Should you use a lambda or `std::bind`?

**The official ROS 2 conventions do not rank one above the other.**

See the table "which language features are allowed" in the [coding conventions](../ros2_coding_conventions.md).

| Feature | ROS 2 position |
| --- | --- |
| **Lambda / `std::function` / `std::bind`** | **No restrictions** ("No restrictions") |

**"`bind` is old, so you should fix it" is not an official rule.**
Remember this. When you see existing code, you do not need to think "I must fix this".

Here is a practical comparison.

| Point | `std::bind` | Lambda |
| --- | --- | --- |
| Look of the official tutorials | **This one** | Some places |
| When the number of arguments changes | You must fix the number of `_1` | You fix the parameter list (the compiler tells you) |
| When you want to ignore an argument | You can write it | You can write it |
| Pass only some of the arguments | **Good at it** | Write it by hand |
| Compiler optimization | Less likely | **More likely** |
| Error messages | **Very hard to read** | Easy to read |
| Stepping in a debugger | Hard to follow | Easy to follow |

**The difference in error messages is really big.**
If you get the number of `_1` wrong in `std::bind`, you get dozens of lines of errors from deep inside templates.
With a lambda, you get one line: "the number of arguments does not match".

This drill uses `std::bind` because it **prioritizes matching the look of the official tutorials**.
The judgment is that being able to follow along with the official documentation is worth more at the learning stage.

In your own code, we recommend **a lambda for new code**.

```cpp
// std::bind version (official, and this drill)
subscription_ = this->create_subscription<std_msgs::msg::String>(
  "topic", 10, std::bind(&MinimalSubscriber::topic_callback, this, _1));

// lambda version (same behavior)
subscription_ = this->create_subscription<std_msgs::msg::String>(
  "topic", 10,
  [this](const std_msgs::msg::String & msg) { this->topic_callback(msg); });
```

The lambda version is one line longer, but its advantage is that **the argument type is written out**.
You can read "what this callback receives" right there.

## Try it yourself

**You will experience the difference between captures, including a lifetime accident.**

```cpp
// lambda.cpp
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Registry
{
public:
  void on_event(std::function<void(int)> f) { handlers_.push_back(std::move(f)); }
  void fire(int value)
  {
    for (const auto & h : handlers_) {
      h(value);
    }
  }
private:
  std::vector<std::function<void(int)>> handlers_;
};

class Counter
{
public:
  explicit Counter(std::string name) : name_(std::move(name)) {}

  void subscribe(Registry & r)
  {
    r.on_event([this](int v) { add(v); });   // capture this
  }

  void add(int v) { total_ += v; }
  void show() const { std::cout << "  " << name_ << " total=" << total_ << "\n"; }

private:
  std::string name_;
  int total_ = 0;
};

int main()
{
  Registry reg;

  // ① copy capture and reference capture
  int base = 10;
  reg.on_event([base](int v) { std::cout << "  copy: base+v = " << base + v << "\n"; });
  reg.on_event([&base](int v) { std::cout << "  ref:  base+v = " << base + v << "\n"; });
  base = 1000;

  // ② capturing this
  Counter c("alive");
  c.subscribe(reg);

  std::cout << "fire(5):\n";
  reg.fire(5);
  c.show();

  // ③ equivalence with std::bind
  std::cout << "bind:\n";
  Counter d("bound");
  reg.on_event(std::bind(&Counter::add, &d, std::placeholders::_1));
  reg.fire(3);
  d.show();

  return 0;
}
```

**Make 3 predictions.**

1. Does the `copy:` line print `15` or `1005`?
2. Which does the `ref:` line print?
3. What is the last `d total=`? (`fire(3)` runs only once, but is the amount from `fire(5)` included?)

```bash
g++ -std=c++17 -Wall -Wextra lambda.cpp -o lambda && ./lambda
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/Y1YvE31T1)

Next, **cause the lifetime accident.** Replace the `②` part of `main` with the following (`c` no longer exists, so also delete the later `c.show();` line).

```cpp
  // ② capturing this (dangerous version)
  {
    Counter tmp("dead");
    tmp.subscribe(reg);
  }   // tmp dies here. But reg still holds the this of tmp
```

```bash
g++ -std=c++17 -Wall -Wextra -fsanitize=address lambda.cpp -o lambda && ./lambda
```

**Add `-fsanitize=address`.** Without it, the program may "happen to work".
It is reported as `stack-use-after-scope`.

**This is the most common accident with lambdas.**
`[this]` is "safe as long as the object is alive", and
**the side that registers the lambda is responsible for that guarantee**.

In rclcpp, the ownership model from chapter 6 guarantees this.
The timer is a member of the node, so it does not outlive the node.
That is why `[this]` is safe in the drill code.

Finally, let us look at the error message of `std::bind`.
Add one more `_1`.

```cpp
reg.on_event(std::bind(&Counter::add, &d, std::placeholders::_1, std::placeholders::_2));
```

**Count the lines of the error.**

```bash
g++ -std=c++17 -c lambda.cpp -o /dev/null 2>&1 | wc -l
```

<!-- measure: files=lambda.cpp cmd="sed -i 's/placeholders::_1));/placeholders::_1, std::placeholders::_2));/' lambda.cpp; g++ -std=c++17 -c lambda.cpp -o /dev/null 2>&1 | wc -l" -->
```
22
```

It printed **more than 20 lines** (the number in the output above). Most of them are template expansions of `std::_Bind_check_arity<...>` and `std::_Bind_helper<...>`,
and the only line that tells you the number of arguments is wrong is `static assertion failed: Wrong number of arguments for pointer-to-member`.
If you get the number of arguments wrong in a lambda, you get just 1 or 2 lines such as `too few arguments to function`.
This is the practical reason we recommend "a lambda for new code".

## Common pitfalls

**`error: invalid use of non-static member function`**
You forgot the `&` on the member function passed to `std::bind`.
Write `&MinimalSubscriber::topic_callback`.

**`std::bind` gave dozens of lines of errors**
First check the number of `_N`. This is the cause 90% of the time.

- Timer callback (0 arguments) → `_1` is not needed
- Subscription callback (1 argument) → `_1`
- Service callback (2 arguments) → `_1, _2`

If you look in the error for the line `no match for call to (std::_Bind<...>)`,
you can read how many arguments were actually passed.

**`error: ‘_1’ was not declared in this scope`**
You forgot to write `using std::placeholders::_1;`.
Or you forgot `#include <functional>`.

**A member variable is not visible inside the lambda**
Write `[this]` or `[=]` in the capture.
With `[]`, nothing from the outside is visible.

**`error: assignment of read-only variable` (inside a lambda)**
You are modifying a variable captured by copy. Add `mutable`, or use a reference capture.

**An error with a captured `unique_ptr`**
`[p]` makes a copy, so it does not compile for a `unique_ptr`.
Use an init capture: `[p = std::move(p)]`.

**It got slower after I put the lambda in a `std::function`**
This is the cost of type erasure. Where you can accept a template, keep it as `auto`.
Where "the library requires a `std::function`", such as `create_subscription`,
you cannot avoid it (and it does not matter).

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 01 | `std::bind(&MinimalPublisher::timer_callback, this)` — no `_1` |
| 02 | `std::bind(&MinimalSubscriber::topic_callback, this, _1)` — one `_1` |
| 04 | The service callback uses `_1, _2` (request and response) |
| 05 | In some places you write the completion handling of `async_send_request` as a lambda |
| 07 | Register a lambda with `ParameterEventHandler` |
| 10 | Register the 3 action callbacks `handle_goal` / `handle_cancel` / `handle_accepted` |
| 13 | Subscriptions and clients split into callback groups |
| Tests | Lambdas are used a lot inside `tools/drill_harness.hpp` (most of the 56) |

Exercise 10 is the heaviest application of this chapter. **You register 3 callbacks at the same time.**

```cpp
using namespace std::placeholders;

action_server_ = rclcpp_action::create_server<Fibonacci>(
  this,
  "fibonacci",
  std::bind(&FibonacciActionServer::handle_goal, this, _1, _2),
  std::bind(&FibonacciActionServer::handle_cancel, this, _1),
  std::bind(&FibonacciActionServer::handle_accepted, this, _1));
```

**The numbers differ: `_1, _2` / `_1` / `_1`.** Getting this wrong is the most common pitfall in exercise 10.
Check that the numbers match by looking at the declaration of each handler.

Also, inside `handle_accepted`,
you need the action-server-specific practice of **not blocking, and moving the work to another thread**.

```cpp
void handle_accepted(const std::shared_ptr<GoalHandleFibonacci> goal_handle)
{
  std::thread{std::bind(&FibonacciActionServer::execute, this, _1), goal_handle}.detach();
}
```

This connects to chapter 12 (concurrency).

## Matching exercise

After reading this chapter, practice with the matching drill exercise.

- `cpp07_lambda` — register lambdas and callbacks

```bash
./drill run cpp07
```

From the exercise, you can come back to this chapter with `./drill read`.

## References

- [ROS 2 coding conventions](../ros2_coding_conventions.md) — lambda / `std::function` / `std::bind` are all "No restrictions"
- `cppreference` [Lambda expressions](https://en.cppreference.com/w/cpp/language/lambda) and [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind)

---

Previous chapter → [6. Smart pointers](06_smart_pointers.md)
Next chapter → [8. `auto` and type deduction](08_auto_and_type_deduction.md)
