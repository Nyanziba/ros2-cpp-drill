# 1. How build and link work

> **Goal of this chapter**: C++ has no "modules" (C++20 has them, but ROS 2 Jazzy uses C++17).
> Instead it has **`#include`, which only pastes text**, and **a linker that resolves names later**.
> If you do not know this two-step design, errors such as `undefined reference` and `redefinition`
> look like mysterious errors that "will not pass even though the syntax is correct".
> This is the first thing you hit in the drill. We clear it up before we move on to syntax.

> **Prerequisites**: [C++ Basics Chapter 10 Headers and project layout](../cpp-basics/10_headers_and_project_layout.md)
> covers the separation of declarations and definitions and include guards, and [Chapter 7 `static`](../cpp-basics/07_static.md)
> covers file-scope `static` (internal linkage).
> This chapter goes one level deeper into **what the linker looks for and fails to find**.

## 1.1 The difference from Python — `#include` is not an import

Python's `import` is an operation that "finds a module, loads it, and binds it as a namespace".
C++'s `#include` is different. **It only pastes the contents of the given file there, as text.**

You can check this.

```cpp
// hello.cpp
#include <string>
int main() { return 0; }
```

```bash
g++ -std=c++17 -E hello.cpp | wc -l
```

<!-- measure: files=hello.cpp -->
```
25258
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/e1Wax65x1)

`-E` is an option that runs only the preprocessor.
A 2-line file became **25258 lines**. `<string>` and everything it `#include`s
were really expanded and pasted.

This "paste only" property decides everything about the C++ build.

## 1.2 Compilation goes "one file at a time, knowing nothing about the others"

A C++ build has two steps.

```
  minimal_publisher.cpp ──compile──▶ minimal_publisher.o  ┐
                                                          ├─link─▶ talker
  talker_main.cpp       ──compile──▶ talker_main.o        ┘
```

The important point is that **compilation is done completely independently for each `.cpp`.**
While the compiler is compiling `minimal_publisher.cpp`, it does not know
that `talker_main.cpp` exists.

This unit is called a **translation unit**.
One `.cpp` together with everything pasted into it by `#include` makes one translation unit.

Now a problem appears. You want to use `MinimalPublisher` from `talker_main.cpp`,
but its definition is in `minimal_publisher.cpp`, and it cannot be seen at compile time.

The solution is the **separation of declarations and definitions**.

## 1.3 Declarations and definitions — why we split `.hpp` and `.cpp`

- **Declaration** — tells only the type: "something with this name exists somewhere"
- **Definition** — the actual contents

To compile a call, the compiler **needs only the declaration**.
It does not need to know where the contents are. It assumes "the linker will find it later",
and writes out only the call instruction.

Exercise 01 of the drill has exactly this form.

```cpp
// exercises/01_publisher/include/drill/minimal_publisher.hpp (declaration)
class MinimalPublisher : public rclcpp::Node
{
public:
  MinimalPublisher();

private:
  void timer_callback();

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  size_t count_;
};
```

```cpp
// solutions/01_publisher/src/minimal_publisher.cpp (definition)
MinimalPublisher::MinimalPublisher()
: Node("minimal_publisher"), count_(0)
{
  publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);
  timer_ = this->create_wall_timer(
    500ms, std::bind(&MinimalPublisher::timer_callback, this));
}
```

Because the `.hpp` only says `MinimalPublisher();`, `talker_main.cpp` can write
`std::make_shared<MinimalPublisher>()`. It does not need to know the contents.

Look at the prefix `MinimalPublisher::`.
In the `.cpp` you must say which class the member belongs to.
This is because once you are outside the class definition, you have left that scope.

### A member variable declaration is also a "definition"

This is a confusing exception.
`size_t count_;` in the `.hpp` looks like a declaration, but
**it is information that decides the class layout (which member takes how many bytes)**,
so it must be written in the `.hpp` as part of the class definition.

So if you look at the `.hpp`, you can see everything the class has.
The other side of this is that **adding just one member requires recompiling every `.cpp` that includes the `.hpp`.**
This is one of the main reasons C++ builds are slow.

## 1.4 One Definition Rule — "define only once"

Remember that `#include` is "paste only".
Then an accident like the following happens.

```cpp
// a.hpp
struct Point { int x; int y; };
```

```cpp
// b.hpp
#include "a.hpp"
```

```cpp
// main.cpp
#include "a.hpp"
#include "b.hpp"   // a.hpp is pasted a second time here
int main() { return 0; }
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// a.hpp
struct Point { int x; int y; };
```

```cpp
// b.hpp
#include "a.hpp"
```

```cpp
// main.cpp
#include "a.hpp"
#include "b.hpp"   // a.hpp is pasted a second time here
int main() { return 0; }
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o main
```

</details>

<!-- measure: files=a.hpp,b.hpp,main.cpp -->
```
In file included from b.hpp:2,
                 from main.cpp:3:
a.hpp:2:8: error: redefinition of ‘struct Point’
    2 | struct Point { int x; int y; };
      |        ^~~~~
In file included from main.cpp:2:
a.hpp:2:8: note: previous definition of ‘struct Point’
    2 | struct Point { int x; int y; };
      |        ^~~~~
```

Notice that the error line points to `a.hpp:2`.
**What is wrong is the two `#include` lines in `main.cpp`, but the compiler points to the line where the text was pasted.**
The chain of `In file included from` is the route that shows "how we got here".
When you read a C++ error, the basic way is to read this route from bottom to top.

The definition of `Point` appeared twice in the same translation unit.
C++ has a rule called the **One Definition Rule (ODR)**:
**the same definition must not appear twice in one translation unit.**

The solution is an **include guard**. The drill uses `#pragma once`.

```cpp
// exercises/11_node_test/include/drill/velocity_limiter.hpp
#pragma once

double limit_velocity(double target, double previous, double max_speed, double max_delta);
```

`#pragma once` is an instruction that says "paste this file only once per translation unit".
The second `#include` does nothing.

The old way is the following form, and the meaning is the same. The ROS 2 core uses this one.

```cpp
#ifndef DRILL__VELOCITY_LIMITER_HPP_
#define DRILL__VELOCITY_LIMITER_HPP_
// ...
#endif  // DRILL__VELOCITY_LIMITER_HPP_
```

`#pragma once` is not part of the official standard, but g++, clang, and MSVC all support it.
This drill uses `#pragma once` to keep things short.

### ODR also applies across translation units

There is one more trap. If you **write the body of a function** in a `.hpp`,
each `.cpp` that includes that `.hpp` gets its own definition of the same function.

```cpp
// bad.hpp
#pragma once
int add(int a, int b) { return a + b; }   // I wrote the body by mistake
```

Even with `#pragma once`, this creates one definition in each of the **separate translation units**
`x.cpp` and `y.cpp`. Compilation passes, and the link fails.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// bad.hpp
#pragma once
int add(int a, int b) { return a + b; }   // I wrote the body by mistake
```

```cpp
// x.cpp
#include "bad.hpp"

int main() { return add(1, 2); }
```

```cpp
// y.cpp
#include "bad.hpp"
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic x.cpp y.cpp -o app
```

</details>

<!-- measure: files=bad.hpp,x.cpp,y.cpp -->
```
/usr/bin/ld: /tmp/cc51bYE3.o: in function `add(int, int)':
y.cpp:(.text+0x0): multiple definition of `add(int, int)'; /tmp/ccMSxLb3.o:x.cpp:(.text+0x0): first defined here
collect2: error: ld returned 1 exit status
```

A line that starts with `/usr/bin/ld:` is the mark of a linker message.
**Compiler errors come with a line, a column, and a source excerpt, but linker errors do not.**
The linker does not look at the source, so it can only give an offset inside the object file, such as `.text+0x0`.
This difference in appearance tells you directly "at which stage it failed".

`#pragma once` prevents only **duplicates inside the same translation unit**.
It cannot prevent duplicates across translation units.

When you want to write the body in a `.hpp`, add `inline`.
`inline` is an instruction that says "the same definition may exist in several translation units. The linker should pick one".

```cpp
// good.hpp
#pragma once
inline int add(int a, int b) { return a + b; }
```

**A member function written inside a class definition is automatically `inline`.**
So the following form is fine.

```cpp
class Foo
{
public:
  int add(int a, int b) { return a + b; }   // implicitly inline
};
```

Templates are treated as an exception in the same way (Chapter 8).

## 1.5 How to read link errors

Let us sort out how to tell build errors apart. **The place of the cause changes depending on which stage failed.**

| Message | Stage | Meaning | Where to fix |
| --- | --- | --- | --- |
| `'x' was not declared in this scope` | Compile | The **declaration** is not visible | Add `#include`, namespace, spelling |
| `no matching function for call to ...` | Compile | There is a declaration but the argument types do not match | The arguments at the call site |
| `redefinition of ...` | Compile | Two definitions in the same translation unit (ODR) | Include guard |
| `undefined reference to 'f()'` | **Link** | The **definition** is not found | Missing implementation in `.cpp`, link settings in `CMakeLists.txt` |
| `multiple definition of 'f()'` | **Link** | Definitions exist in several translation units | Add `inline`, move it to a `.cpp` |

**`undefined reference` is not a compile error.**
It says "the syntax is all correct. The contents are just nowhere to be found".
So the places to look are not the syntax of the source, but these 3 things.

1. Did you forget to write that function in the `.cpp`?
2. Did you write it but forget a qualifier such as `MinimalPublisher::`, and define a different function?
3. Did you forget to put that `.cpp` in `add_library` / `add_executable` in `CMakeLists.txt`?

The third type really happens in the drill. If you add an exercise but forget to write the
file name in `CMakeLists.txt`, it appears in this form.

### Reading name mangling

Linker messages are hard to read because C++ stores function names in a **transformed** form.

```bash
g++ -std=c++17 -c minimal_publisher.cpp -o mp.o
nm -C mp.o | grep timer_callback
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// drill/minimal_publisher.hpp
#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

/// MinimalPublisher from the official tutorial
/// "Writing a simple publisher and subscriber (C++)".
///
/// The only difference from the official one is that main() is split into another file (src/talker_main.cpp),
/// because the tests create and check this class directly.
class MinimalPublisher : public rclcpp::Node
{
public:
  MinimalPublisher();

private:
  void timer_callback();

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  std::size_t count_;
};
```

```cpp
// minimal_publisher.cpp
#include "drill/minimal_publisher.hpp"

using namespace std::chrono_literals;

MinimalPublisher::MinimalPublisher()
: Node("minimal_publisher"), count_(0)
{
  publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);
  timer_ = this->create_wall_timer(
    500ms, std::bind(&MinimalPublisher::timer_callback, this));
}

void MinimalPublisher::timer_callback()
{
  auto message = std_msgs::msg::String();
  message.data = "Hello, world! " + std::to_string(count_++);
  RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
  publisher_->publish(message);
}
```

```bash
g++ -std=c++17 -I. -c minimal_publisher.cpp -o mp.o   # the rclcpp include paths (-I) are omitted
nm -C mp.o | grep timer_callback
```

</details>

<!-- measure: env=ros files=drill/minimal_publisher.hpp,minimal_publisher.cpp cmd="g++ -std=c++17 -I. $(find /opt/ros/jazzy/include -maxdepth 1 -mindepth 1 -type d -printf '-I%p ') -c minimal_publisher.cpp -o mp.o && nm -C mp.o | grep timer_callback" filter="grep ' T '" -->
```
00000000000004b4 T MinimalPublisher::timer_callback()
```

> This output needs rclcpp, so it is an excerpt measured in an environment with ROS 2 (the repository's Docker image). Unlike the other examples in the C++ track, you cannot reproduce it with `g++` alone.

`nm` prints the list of symbols inside an object file.
Without `-C`, you get the transformed name `_ZN16MinimalPublisher14timer_callbackEv`.
With `-C` (demangle), it goes back to a form humans can read.

When you are stuck with `undefined reference`, **checking directly with `nm -C` whether that symbol really exists**
is the fastest way to narrow it down.

- `T` — this file has the definition (text section)
- `U` — this file uses it but does not have the definition (undefined)

If it stays `U` and nobody has a `T`, that is the real cause of the link error.

## 1.6 What does CMake do?

The `CMakeLists.txt` of each drill exercise is the two steps above, written out as they are.

```cmake
# exercises/01_publisher/CMakeLists.txt (excerpt, summarized)
add_library(minimal_publisher src/minimal_publisher.cpp)   # ① compile it into a library
target_include_directories(minimal_publisher PUBLIC include) # ② where to look for the .hpp
ament_target_dependencies(minimal_publisher rclcpp std_msgs) # ③ the includes and libraries of the dependencies

add_executable(talker src/talker_main.cpp)                  # ④ compile main
target_link_libraries(talker minimal_publisher)              # ⑤ link with ①
```

- If ② is missing, you get a **compile error** (the `.hpp` cannot be found)
- If ⑤ is missing, you get a **link error** (`undefined reference to MinimalPublisher::...`)

From the stage of the error message, you can work backwards to which line of `CMakeLists.txt` is missing.

The reason the class is made into a library is that we want **to link the same implementation from both the executable for `ros2 run` and gtest**.
The `.o` compiled once is reused in 2 places.

```
src/minimal_publisher.cpp ──▶ libminimal_publisher.a ──┬──▶ talker (for ros2 run)
                                                       └──▶ test_exercise (for grading)
```

## Try it yourself

**Let us cause an ODR error and a link error ourselves.** Create 3 files.

```cpp
// counter.hpp
#pragma once

int next_id();          // declaration only
int twice(int x) { return x * 2; }   // <- I wrote the body on purpose (a trap)
```

```cpp
// counter.cpp
#include "counter.hpp"

int next_id()
{
  static int id = 0;
  return ++id;
}
```

```cpp
// main.cpp
#include "counter.hpp"
#include <iostream>

int main()
{
  std::cout << next_id() << next_id() << twice(21) << "\n";
  return 0;
}
```

**Predict. `twice` is written only once, inside a `.hpp` protected by `#pragma once`.
Will the build pass?**

```bash
g++ -std=c++17 -Wall -Wextra counter.cpp main.cpp -o app && ./app
```

<details markdown="1"><summary>Answer (compile error)</summary>

<!-- measure: files=counter.hpp,counter.cpp,main.cpp -->
```
/usr/bin/ld: /tmp/cc8kIU49.o: in function `twice(int)':
main.cpp:(.text+0x0): multiple definition of `twice(int)'; /tmp/ccNwomqP.o:counter.cpp:(.text+0x0): first defined here
collect2: error: ld returned 1 exit status
```

</details>

**It fails.** `twice` is written only once in the source, but you get "multiple definition".
This is because `counter.cpp` and `main.cpp` each pasted `counter.hpp`,
and **2 copies were made in the object files**.
This is the point of Section 1.4: `#pragma once` prevents only duplicates inside one translation unit.

If you add translation units, the errors also increase.

```bash
sed 's/int main()/int other()/' main.cpp > other.cpp
g++ -std=c++17 counter.cpp main.cpp other.cpp -o app
```

<!-- measure: files=counter.hpp,counter.cpp,main.cpp cmd="sed 's/int main()/int other()/' main.cpp > other.cpp; g++ -std=c++17 counter.cpp main.cpp other.cpp -o app" -->
```
/usr/bin/ld: /tmp/ccnS7bOT.o: in function `twice(int)':
main.cpp:(.text+0x0): multiple definition of `twice(int)'; /tmp/ccQEXosz.o:counter.cpp:(.text+0x0): first defined here
/usr/bin/ld: /tmp/ccXufbXu.o: in function `twice(int)':
other.cpp:(.text+0x0): multiple definition of `twice(int)'; /tmp/ccQEXosz.o:counter.cpp:(.text+0x0): first defined here
collect2: error: ld returned 1 exit status
```

From the number of errors, you can see that one definition is made per `.cpp`.

Now let us fix it. Add `inline` to `twice` in `counter.hpp`.

```bash
sed -i 's/^int twice/inline int twice/' counter.hpp
g++ -std=c++17 counter.cpp main.cpp other.cpp -o app && ./app
```

<!-- measure: files=counter.hpp,counter.cpp,main.cpp cmd="sed 's/int main()/int other()/' main.cpp > other.cpp; sed -i 's/^int twice/inline int twice/' counter.hpp; g++ -std=c++17 counter.cpp main.cpp other.cpp -o app && ./app" -->
```
1242
```

It passed. Let us look directly with `nm` at what changed.

```bash
$ g++ -std=c++17 -c main.cpp -o main.o && nm -C main.o | grep twice
```

| Without `inline` | With `inline` |
| --- | --- |
| `0000000000000000 T twice(int)` | `0000000000000000 W twice(int)` |

**`T` changed to `W`.**
`T` means "this is the only definition", and `W` (weak symbol) means "it has a definition, but
others may also have the same one. The linker should pick any one".
`inline` is the keyword for changing this one character.

You may have seen the explanation "it expands the function call to make it faster", but
**in modern C++ the practical meaning of `inline` is this relaxation of the ODR.**
The optimizer decides by itself whether to expand, and it does not look at whether `inline` is present.

Check the output `1242` as well. `next_id()` gives `1` and `2`, and `twice(21)` is `42`, so `1242`.
The `static int id` in `next_id` is a variable that **does not disappear when the function returns**, and
because there is only one in `counter.cpp`, it grows every time you call it.

## Common pitfalls

**A change to a `.hpp` is not reflected**
If you fixed a `.hpp` but the build does not change, the record of dependencies is broken.
Run `rm -rf build install log` and start again.
CMake tracks `.hpp` dependencies automatically, but it can miss them when you move or rename a file.

**`error: 'size_t' was not declared in this scope`**
`size_t` is in `<cstddef>`.
If another header happens to include it indirectly, it passes, and it fails as soon as you tidy up the includes.
The rule is to **include the types you use yourself** ("include what you use").

**The difference between `#include "..."` and `#include <...>`**
`"..."` means "look in the same directory as this file first, then in the include path".
`<...>` means "only the include path".
The convention is `"..."` for your own headers and `<...>` for the system and external libraries.
In the drill, both `#include "drill/minimal_publisher.hpp"` and `#include <gtest/gtest.h>`
appear.

**`undefined reference to 'main'`**
You forgot to put the `.cpp` that contains `main()` into the build targets.
In the drill, `main()` is separated into `src/*_main.cpp`,
so check whether that file is in `add_executable`.

## Where it appears in the drill

| Exercise | Related place |
| --- | --- |
| All exercises | `include/drill/*.hpp` are declarations and `src/*.cpp` are definitions. You edit only the `.cpp` |
| 01 | `talker_main.cpp` and `minimal_publisher.cpp` are separate translation units. They are linked through a library |
| 03 | The `.hpp` is **auto-generated** from `.msg` / `.srv`. The generated files are created under `build/`, and you include them |
| 11 | The pure function `limit_velocity` is declared in a `.hpp`, and two `.cpp` files (a correct version and a buggy version) are prepared, and the link is swapped |
| 15 | `RCLCPP_COMPONENTS_REGISTER_NODE` embeds a registration symbol in the shared library. It is looked up with `dlopen` at run time |

The structure of exercise 11 is a comprehensive exercise for this chapter.

```
velocity_limiter.hpp (one declaration)
  ├── velocity_limiter.cpp        (correct definition) ──▶ test_exercise        … should pass
  └── velocity_limiter_mutant.cpp (buggy definition)   ──▶ test_exercise_mutant … should fail
```

With the same `.hpp` and the same test code, **you make 2 executables by swapping only the `.cpp` you link.**
You can do this because declarations and definitions are separate.
A mechanism to check mechanically "whether the test can detect the bug" sits on top of the mechanism in this chapter.

## Matching exercise

After you read this chapter, practice with the matching drill.

- `cpp01_build_and_link` — get build and link to pass

```bash
./drill run cpp01
```

From the exercise side, you can come back to this chapter with `./drill read`.

## References

- [ROS 2 coding conventions](../ros2_coding_conventions.md) — the reason for using `.hpp` / `.cpp` (so that tools can tell C and C++ apart)
- `cppreference` [Translation unit](https://en.cppreference.com/w/cpp/language/translation_phases) and [Definitions and ODR](https://en.cppreference.com/w/cpp/language/definition)
- `nm` / `c++filt` / `readelf -s` — tools to look at symbols directly

---

Next → [2. Classes and initialization](02_classes_and_initialization.md)
