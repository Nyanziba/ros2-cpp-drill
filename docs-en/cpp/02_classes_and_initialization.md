# 2. Classes and initialization

> **Goal of this chapter**: All 15 exercises in the drill ask you to write the body of a constructor such as
> `MinimalPublisher::MinimalPublisher()`. And every exercise has **an unfamiliar line that starts with a colon**:
> `: Node("minimal_publisher"), count_(0)`.
> This is called a member initializer list, and it is the heart of a C++ constructor.
> This chapter covers how to initialize, and **the fact that destructors are called automatically (RAII)**.
> RAII is the base for the ownership topics in the following chapters.

> **Prerequisites**: We assume you have read [C++ Basics Chapter 6 `const`](../cpp-basics/06_const.md) and
> [Chapter 2 Scope and lifetime](../cpp-basics/02_scope_and_lifetime.md).
> The reason a `const` member **can be initialized only in the initializer list**, and
> when an object is constructed and destroyed, are the base of this chapter.

## 2.1 The shortest class

```cpp
// counter.cpp
#include <iostream>

class Counter
{
public:
  Counter()
  {
    std::cout << "Created\n";
    count_ = 0;
  }

  void tick() { ++count_; }
  int value() const { return count_; }

private:
  int count_;
};

int main()
{
  Counter c;        // Counter() runs here
  c.tick();
  c.tick();
  std::cout << c.value() << "\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/7x5n7K9Kh)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic counter.cpp -o counter && ./counter" -->
```
Created
2
```

Notice that `Counter c;` has no `new`.
Unlike Java or C#, **C++ classes can usually be placed directly as values.**
`c` is just a variable on the stack, and it is removed automatically when you leave `main`.

`public:` and `private:` are labels that change the access of the members below them.
**The default for `class` is `private`**, so if you do not write `public:`, nothing can be touched from outside.
`struct` defaults to `public`, and is otherwise exactly the same as `class`.

The `const` at the end of `value() const` is a declaration that "this member function does not change the member variables".
We cover it in detail in Chapter 3.

## 2.2 The member initializer list — what the colon is

The `Counter` above wrote `count_ = 0;` in the body. This is an **assignment**.
C++ distinguishes **initialization** from assignment.

```cpp
Counter()
: count_(0)     // <- member initializer list (initialization)
{
  // by the time we get here, count_ is already 0
}
```

The part before `{` that starts with `:` is the member initializer list.
The members written here are initialized **before the constructor body starts**.

Exercise 01 of the drill has this form.

```cpp
// solutions/01_publisher/src/minimal_publisher.cpp
MinimalPublisher::MinimalPublisher()
: Node("minimal_publisher"), count_(0)
{
  publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);
  timer_ = this->create_wall_timer(
    500ms, std::bind(&MinimalPublisher::timer_callback, this));
}
```

`count_(0)` is the initialization of the member variable `count_`.
`Node("minimal_publisher")` is **a call to the base class constructor**, which is the topic of the next chapter.

### Why use initialization instead of assignment

There are 3 reasons. Reasons 1 and 3 matter in the drill.

**① Some members cannot be assigned**

`const` members and reference members can be initialized, but they cannot be assigned.

```cpp
class Config
{
public:
  Config(int limit)
  {
    limit_ = limit;    // <- error
  }
private:
  const int limit_;
};
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// config.cpp
class Config
{
public:
  Config(int limit)
  {
    limit_ = limit;    // <- error
  }
private:
  const int limit_;
};

int main()
{
  Config config(10);
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic config.cpp -o config
```

</details>

<!-- measure: -->
```
config.cpp: In constructor ‘Config::Config(int)’:
config.cpp:5:3: error: uninitialized const member in ‘const int’ [-fpermissive]
    5 |   Config(int limit)
      |   ^~~~~~
config.cpp:10:13: note: ‘const int Config::limit_’ should be initialized
   10 |   const int limit_;
      |             ^~~~~~
config.cpp:7:12: error: assignment of read-only member ‘Config::limit_’
    7 |     limit_ = limit;    // <- error
      |     ~~~~~~~^~~~~~~
```

With an initializer list it passes.

```cpp
Config(int limit) : limit_(limit) {}
```

**② It does the work twice**

A class-type member such as `std::string` has its **default constructor run first**,
even if you do not write it in the initializer list. If you assign in the body, it takes 2 steps: "make an empty string, then assign".
If you write it in the initializer list, it takes 1 step.

**③ It prevents the accident of reading an uninitialized member**

This is the scariest reason. **A member of a built-in type such as `int`
is not initialized if you do not write it.** It does not even become zero.

```cpp
#include <iostream>

class Bad
{
public:
  Bad() {}          // count_ is not initialized
  int value() const { return count_; }
private:
  int count_;
};

int main()
{
  Bad b;
  std::cout << b.value() << "\n";   // we cannot tell what is printed
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/qbd9forbh)

This is undefined behavior. A different value may appear each time you run it, or 0 may appear by chance.
It is a kind of bug where **"it worked on my machine" gives no guarantee at all**.

If you add `-Wall`, g++ may warn you,
but it cannot detect this when the constructor is in another file.
Do not depend on `-Wall`. Make it a habit to **write all members in the initializer list**.

### Match the order to the order of member declarations

The order you write in the initializer list is ignored.
**The real initialization order is the order in which you declared the members in the class definition.**

```cpp
class Ordered
{
public:
  Ordered()
  : b_(1), a_(b_ + 1)    // the written order is b_ -> a_
  {
  }
  int a_;                 // the declaration order is a_ -> b_
  int b_;
};
```

`a_(b_ + 1)` reads `b_`, but **in the declaration order `a_` is initialized first**,
so `b_` is not initialized yet. This is undefined behavior.

g++ tells you this with `-Wall`.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// ordered.cpp
class Ordered
{
public:
  Ordered()
  : b_(1), a_(b_ + 1)    // the written order is b_ -> a_
  {
  }
  int a_;                 // the declaration order is a_ -> b_
  int b_;
};

int main()
{
  Ordered ordered;
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic ordered.cpp -o ordered && ./ordered
```

</details>

<!-- measure: -->
```
ordered.cpp: In constructor ‘Ordered::Ordered()’:
ordered.cpp:10:7: warning: ‘Ordered::b_’ will be initialized after [-Wreorder]
   10 |   int b_;
      |       ^~
ordered.cpp:9:7: warning:   ‘int Ordered::a_’ [-Wreorder]
    9 |   int a_;                 // the declaration order is a_ -> b_
      |       ^~
ordered.cpp:5:3: warning:   when initialized here [-Wreorder]
    5 |   Ordered()
      |   ^~~~~~~
ordered.cpp:6:15: warning: member ‘Ordered::b_’ is used uninitialized [-Wuninitialized]
    6 |   : b_(1), a_(b_ + 1)    // the written order is b_ -> a_
      |               ^~
ordered.cpp: In constructor ‘Ordered::Ordered()’:
ordered.cpp:6:15: warning: ‘*this.Ordered::b_’ is used uninitialized [-Wuninitialized]
    6 |   : b_(1), a_(b_ + 1)    // the written order is b_ -> a_
      |               ^~
```

**If a `-Wreorder` warning appears, fix the order.**
We enable `-Wall -Wextra -Wpedantic` in every exercise in the drill to catch this kind of problem.

### Default values for member variables (C++11 and later)

You can also write them directly at the declaration.

```cpp
class Counter
{
private:
  int count_ = 0;                    // default value
  std::string name_ = "unnamed";
};
```

If you specify a value in the initializer list, that wins. If you do not, this value is used.
**The accident of "forgetting to initialize" structurally cannot happen**,
so it is especially effective for classes with several constructors.

The drill code uses the initializer list form because it matches the wording of the official tutorial,
but you may use this form in your own code.

## 2.3 `this` and `this->`

Inside a member function you can use `this`. It is **a pointer to the object itself**.

The drill code writes `this->create_publisher(...)`, with `this->` every time.
In fact, **it is the same if you omit it**.

```cpp
publisher_ = create_publisher<std_msgs::msg::String>("topic", 10);   // this also works
publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);  // the official tutorial uses this
```

If you grep, the drill has `this->` in 65 places. All of them can be omitted.
We just match the official tutorial, which writes it this way.

You cannot omit it in the following 2 cases.

**① When a parameter name collides with a member name**

```cpp
void set_count(int count_)
{
  this->count_ = count_;     // the left side is the member, the right side is the parameter
}
```

However, the ROS 2 naming convention adds a `_` suffix to members, so this collision almost never happens.
**The `count_` suffix itself is a device that makes `this->` unnecessary.**

**② When you refer to a member of a template base class**

This is a topic for the templates in Chapter 9, but it does not appear in rclcpp.

### `this` is a pointer, so use `->`

`this` is a pointer, not a reference. So it is `this->foo`, not `this.foo`.
You can also write `(*this).foo`, but nobody does.

`std::shared_ptr` is the same: the `->` in `publisher_->publish(message)` has the same meaning,
"a member of the thing the pointer points to" (Chapter 6).

## 2.4 RAII — destructors are called automatically

C++ has no syntax equivalent to Python's `with` or Java's `try-with-resources`.
**It does not need one.** When you leave a scope, the destructor is called automatically.

```cpp
// noisy.cpp
#include <iostream>

class Noisy
{
public:
  explicit Noisy(const char * name) : name_(name)
  {
    std::cout << name_ << " created\n";
  }

  ~Noisy()
  {
    std::cout << name_ << " destroyed\n";
  }

private:
  const char * name_;
};

int main()
{
  std::cout << "-- main starts\n";
  Noisy a("a");
  {
    Noisy b("b");
    std::cout << "-- end of the inner scope\n";
  }
  std::cout << "-- main ends\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/d77eo7xPz)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic noisy.cpp -o noisy && ./noisy" -->
```
-- main starts
a created
b created
-- end of the inner scope
b destroyed
-- main ends
a destroyed
```

Check 3 points.

1. `b` is destroyed at the inner `}`, and `a` lives until the `}` of `main`
2. The destructors are called even though we wrote no `delete`
3. **Destruction is in the reverse order of construction** (`a` then `b` are constructed, and `b` then `a` are destroyed)

Managing resources by using this property, "when you leave the scope, cleanup is certain", is
**RAII (Resource Acquisition Is Initialization)**.
If you wrap a resource in a type that "acquires in the constructor and releases in the destructor",
the caller does not need to write the release.

RAII works because **the destructor is always called, even if an exception is thrown and wherever the `return` is.**
It is called even if you jump out with `goto`.

```cpp
void f()
{
  Noisy a("a");
  if (something_wrong) {
    return;              // even if we leave here, a is destroyed
  }
  may_throw();           // even if an exception is thrown, a is destroyed
}
```

In C, people wrote `goto cleanup;` because this did not exist.

### RAII appears in many places in ROS 2

| What is wrapped with RAII | What is acquired and released |
| --- | --- |
| `std::shared_ptr` / `std::unique_ptr` | Heap memory (Chapter 6) |
| `std::lock_guard` | A mutex lock (Chapter 12) |
| `rclcpp::Node` | The rcl node handle, the DDS participant |
| `rclcpp::Publisher` | A DDS DataWriter |
| `std::ofstream` | A file descriptor |

Writing `rclcpp::init()` / `rclcpp::shutdown()` as a pair in `main` is
**the one place that is not RAII** (`rclcpp::Context` is RAII, but
the global init/shutdown remains as a procedural API).

```cpp
// exercises/01_publisher/src/talker_main.cpp
int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MinimalPublisher>());
  rclcpp::shutdown();
  return 0;
}
```

### Do not throw exceptions from a destructor

You must not throw an exception from inside `~Noisy()`.
A destructor is also called "while unwinding because of an exception",
and if another exception is thrown there, the process is killed with `std::terminate`.

The [ROS 2 coding conventions](../ros2_coding_conventions.md) also state it clearly:
"avoid exceptions in destructors".

## 2.5 `explicit` — stop unintended conversions

The drill has `explicit` in 3 places. All of them have this form.

```cpp
// exercises/14_zero_copy/include/drill/zero_copy_nodes.hpp
explicit ZeroCopyTalker(const rclcpp::NodeOptions & options);
```

**A constructor with one argument is used as an implicit type conversion by default.**
This causes a troublesome accident.

```cpp
// meters_implicit.cpp
#include <iostream>

class Meters
{
public:
  Meters(double v) : v_(v) {}     // no explicit
  double value() const { return v_; }
private:
  double v_;
};

void move_robot(Meters distance)
{
  std::cout << "move " << distance.value() << " m\n";
}

int main()
{
  move_robot(Meters(1.5));   // the intended way to call it
  move_robot(1.5);           // this also passes
  move_robot(true);          // this also passes (bool -> double -> Meters)
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/K5ajfc5ae)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic meters_implicit.cpp -o meters_implicit && ./meters_implicit" -->
```
move 1.5 m
move 1.5 m
move 1 m
```

`move_robot(true)` became "move 1 meter".
The compiler converts in 2 steps, `bool` -> `double` -> `Meters`, and silently accepts it.

If you add `explicit`, it stops.

```cpp
explicit Meters(double v) : v_(v) {}
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// meters.cpp
#include <iostream>

class Meters
{
public:
  explicit Meters(double v) : v_(v) {}
  double value() const { return v_; }
private:
  double v_;
};

void move_robot(Meters distance)
{
  std::cout << "move " << distance.value() << " m\n";
}

int main()
{
  move_robot(Meters(1.5));   // the intended way to call it
  move_robot(1.5);   // error
  move_robot(true);   // error
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic meters.cpp -o meters
```

</details>

<!-- measure: -->
```
meters.cpp: In function ‘int main()’:
meters.cpp:21:14: error: could not convert ‘1.5e+0’ from ‘double’ to ‘Meters’
   21 |   move_robot(1.5);   // error
      |              ^~~
      |              |
      |              double
meters.cpp:22:14: error: could not convert ‘true’ from ‘bool’ to ‘Meters’
   22 |   move_robot(true);   // error
      |              ^~~~
      |              |
      |              bool
```

`move_robot(Meters(1.5))` passes, and both `move_robot(1.5)` and `move_robot(true)` fail.

Remember: **in principle, add `explicit` to a constructor with one argument.**
The only exception is when it is "really a different representation of the same thing" (such as `std::string` accepting a `const char *`).

`ZeroCopyTalker(const rclcpp::NodeOptions &)` has `explicit` because
it would make no sense if a `NodeOptions` were converted to a node by accident, so this is a natural measure.

## 2.6 Copy and move are generated automatically

Even if you do not write them, the compiler generates the following 6 automatically.

| What is generated | When it is called |
| --- | --- |
| Default constructor | `Foo f;` |
| Destructor | When you leave the scope |
| Copy constructor | `Foo b = a;` |
| Copy assignment operator | `b = a;` |
| Move constructor | `Foo b = std::move(a);` |
| Move assignment operator | `b = std::move(a);` |

The generated ones only "do the same operation on every member".
If the members are an `int` and a `std::string`, it copies the `int` and copies the `std::string`.

**In many cases this is correct and you do not need to write them yourself.**
The difference between move and copy is covered in Chapter 5.

When you want to stop them, write `= delete`.

```cpp
class NonCopyable
{
public:
  NonCopyable() = default;
  NonCopyable(const NonCopyable &) = delete;             // copy is prohibited
  NonCopyable & operator=(const NonCopyable &) = delete; // copy assignment is prohibited
};
```

`std::unique_ptr` has exactly this form (Chapter 6).
To enforce "there is only one owner" with the type, it removes copy and leaves only move.

## Try it yourself

**Let us see an uninitialized member and the initialization-order warning ourselves.**

```cpp
// init.cpp
#include <iostream>
#include <string>

class Sloppy
{
public:
  Sloppy()
  : b_(1), a_(b_ + 1)      // trap 1: the reverse of the declaration order
  {
  }
  int a_;
  int b_;
  int c_;                  // trap 2: not initialized anywhere
  std::string s_;          // a class type, so the default constructor runs
};

int main()
{
  Sloppy s;
  std::cout << "a_=" << s.a_ << " b_=" << s.b_
            << " c_=" << s.c_ << " s_=[" << s.s_ << "]\n";
  return 0;
}
```

**Predict: what will `a_` be? What will `c_` be? What is in `s_`?**

```bash
g++ -std=c++17 -Wall -Wextra init.cpp -o init && ./init
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/s8Mbf66vx)

Read the warnings before you run it.
Run it a few times and see whether the value of `c_` changes (or does not change).
**Even if it does not change, it is still undefined behavior.**
The most dangerous thing is to remember "it happened to be 0" as "it becomes 0".

Next, fix `Sloppy`.

- Reorder the initializer list to the declaration order (`a_`, `b_`, `c_`)
- Initialize everything, like `a_(1), b_(2), c_(3)`

Check that the warnings disappear and the output becomes stable.

Next we try RAII. Replace the `main` of the `Noisy` class above with the following, and check
**the order of destruction, and that destructors are called even when you jump over with `return`.**

```cpp
int main()
{
  Noisy outer("outer");
  for (int i = 0; i < 2; ++i) {
    Noisy loop("loop");
    if (i == 1) {
      std::cout << "-- do return\n";
      return 0;              // what happens to outer and loop?
    }
  }
  return 0;
}
```

**Predict: how many times is `loop destroyed` printed? Is `outer destroyed` printed after the `return`?**

## Common pitfalls

**`error: no matching function for call to ‘Foo::Foo()’`**
You wrote `Foo f;` for a class that has no default constructor.
If you write even one constructor with arguments yourself, **the default constructor is not generated automatically.**
Add `Foo() = default;`, or pass arguments.

**`error: ‘class Foo’ has no member named ‘bar’` even though the member exists**
It is `private`. The default for `class` is `private`, so check whether you forgot to write `public:`.

**Forgetting `Foo::` in the definition of a member function**

```cpp
// minimal_publisher.cpp
void timer_callback()      // forgot Foo::
{
}
```

This is not an error. You have **defined a completely different global function**, so
the compile passes, and the link fails with `undefined reference to MinimalPublisher::timer_callback()`
(Chapter 1).
When you see "I implemented it but get undefined reference", suspect this pattern.

**Writing `~Foo()` removed the move**
If you write a destructor yourself, **the move constructor and the move assignment are no longer generated automatically.**
The copy is still generated, so everything may be copied without you noticing.
It is called the "Rule of Zero": **the best design is one where you do not need to write a destructor (make all members RAII types).**

**A trap when you initialize with `{}`**

```cpp
std::vector<int> a(3, 0);   // 3 elements, all 0
std::vector<int> b{3, 0};   // 2 elements, 3 and 0
```

`()` and `{}` are not the same. The difference shows up especially in `std::vector`.
`{}` prefers to be interpreted as an initializer list.

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 01 | `: Node("minimal_publisher"), count_(0)` — the initializer list. If you forget to initialize `count_`, the number in the log starts from a strange value |
| 04, 05 | Create the service in the constructor and keep it in a member |
| 06 | Call `declare_parameter` in the constructor. If you call `get_parameter` before declaring, you get an exception (Chapter 13) |
| 11 | `limit_velocity` is **a free function, not even a class**. We do this on purpose so it is easy to test |
| 14, 15 | `explicit ZeroCopyTalker(const rclcpp::NodeOptions & options)` — a real example of `explicit` |
| All exercises | `this->` appears in 65 places. All of them can be omitted |

The design decision of exercise 11 is closely related to this chapter, so let us mention it.
`limit_velocity` is not a member of a class. It is **a free function whose result is decided only by its arguments.**

```cpp
// exercises/11_node_test/include/drill/velocity_limiter.hpp
double limit_velocity(double target, double previous, double max_speed, double max_delta);
```

If you make it a member function of a node, testing it requires
"create a node -> declare parameters -> put it on an Executor -> spin".
A free function can be called in one line.

**When something feels hard to test, first cut out "the part that does not touch state" into a free function.**
This goes in the opposite direction from the initialization topic of this chapter, but it is a judgment that is just as useful in practice.

## Matching exercise

After you read this chapter, practice with the matching drill.

- `cpp02_class_init` — initialize a class

```bash
./drill run cpp02
```

From the exercise side, you can come back to this chapter with `./drill read`.

## References

- [ROS 2 coding conventions](../ros2_coding_conventions.md) — Google asks for "all members private" for access control, but ROS 2 removes that requirement
- `cppreference` [Constructors and member initializer lists](https://en.cppreference.com/w/cpp/language/constructor)
- `cppreference` [RAII](https://en.cppreference.com/w/cpp/language/raii)

---

Previous → [1. How build and link work](01_how_build_and_link_work.md)
Next → [3. Inheritance — what does it mean to inherit `rclcpp::Node`?](03_inheritance.md)
