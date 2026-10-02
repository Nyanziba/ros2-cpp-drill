# C++ to Learn Before ROS 2 (15 chapters)

Read this course **before you start the ROS 2 drill**.

rclcpp code depends heavily on a few specific C++ features.
`std::shared_ptr`, lambda expressions, `std::bind`, `std::move`, templates, and `std::chrono` literals.
If you read `create_publisher<std_msgs::msg::String>("topic", 10)` without knowing them,
you cannot tell whether you are learning a ROS concept or fighting C++ syntax.
That is the least efficient state, so we clear up the C++ side first.

On the other hand, **this is not a course that covers every C++ feature.**
Inheritance design theory, how to write your own templates, `constexpr` metaprogramming, and exception-safety design.
These are important in C++, but they do not appear in this drill. We do not cover them.

## What we cover was decided by measurement

The chapter list is not "the table of contents of a C++ textbook". It is decided from **the result of
actually running grep on this drill's `exercises/` and `solutions/`.**

| Syntax | Count | How this course treats it |
| --- | --- | --- |
| `auto` | 139 | Chapter 8 (one chapter) |
| `make_shared` / `make_unique` | 91 | Chapter 6 (one chapter) |
| `this->` | 65 | Chapter 2 |
| Lambda expressions | 56 | Chapter 7 (one chapter) |
| `future` | 50 | Chapter 12 |
| `shared_ptr` | 33 | Chapter 6 (one chapter) |
| `std::string` | 33 | Chapter 10 |
| `std::bind` / `placeholders` | 40 | Chapter 7 (one chapter) |
| `chrono_literals` | 28 | Chapter 11 (one chapter) |
| `std::vector` | 17 | Chapter 10 |
| `nullptr` | 13 | Chapter 6 |
| `unique_ptr` / `std::move` | 12 | Chapters 5 and 6 |
| `mutex` | 9 | Chapter 12 |
| `explicit` | 3 | Chapter 2 |
| `override` | 2 | Chapter 3 (one chapter) |
| `std::optional` | 2 | Chapter 13 |
| **Writing your own `template`** | **0** | Chapter 9 teaches "how to read" only |
| **`try` / `catch` / `throw`** | **0** | Chapter 13 explains "why they do not appear" |

The last two rows decide the character of this course.
**The template chapter is not a chapter where you learn to write templates. It is a chapter where you learn to read them.**
It is enough if you can read `create_publisher<T>` aloud and find a lead from a template error message.
The drill never asks you to write one yourself.

Exceptions are the same. None of the 15 exercises in the drill has a `throw`.
We still have Chapter 13, because "why ROS 2 code does not throw exceptions" is connected to
the design of `declare_parameter` and the reason `rclcpp::ok()` exists.

**There are only 2 places with `override`, but a whole chapter (Chapter 3) is spent on inheritance.
The reason is something you cannot measure by counting.**
All 15 exercises in the drill start with `class X : public rclcpp::Node`.
You rarely write `override`, but **writing code inside an inherited class** happens 100% of the time.
In addition, the autonomous navigation code beyond the drill (`SimplePlanner`, and the
`SystemInterface` plugin implementations) is itself the implementation of pure virtual functions, so you suddenly need it there.

## Who this is for

- You have written programs in another language (Python, C, Java, Rust, and so on)
- You have "seen C++ but not written it yourself", or "touched it a little at school"
- **We assume you have already learned how to read references, pointers, and `const`.**

**These 15 chapters do not start from the basics.** What a reference is, what a pointer is, how `const` changes meaning in
4 places, and why `static` has 3 meanings — all of these are in the
[C++ Basics (10 chapters)](../cpp-basics/README.md).

| If you are unsure about | Where in C++ Basics |
| --- | --- |
| You cannot read `const int * const p` | [Chapter 1 Reading declarations](../cpp-basics/01_reading_declarations.md) |
| You cannot tell references and pointers apart | [Chapter 3](../cpp-basics/03_references.md), [Chapter 4](../cpp-basics/04_pointers_1.md), [Chapter 5](../cpp-basics/05_pointers_2.md) |
| You get stuck on `const` or `static` | [Chapter 6](../cpp-basics/06_const.md), [Chapter 7](../cpp-basics/07_static.md) |

**It takes 1 to 2 hours to fill the gap.** If you do C++ Basics first, this course goes faster.

If you already write C++ every day, it is enough to read **only Chapter 6 (smart pointers) and Chapter 7 (lambdas and `std::bind`)**,
and then check for gaps with the checklist in Chapter 15.

## You do not need ROS 2

**These 15 chapters do not use ROS 2 at all.**
The "Try it yourself" in each chapter is a single file, and you can compile it with `g++` alone.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<!-- only: jazzy -->
`-std=c++17` is required. ROS 2 Jazzy targets C++17, so this course also uses C++17.
<!-- /only -->
<!-- only: lyrical -->
`-std=c++17` is required. The ament of ROS 2 Lyrical asks for C++20 by default (`ament_ros_cxx_standard` in `ament_ros_core` requires `cxx_std_20`), but this course does not use ROS 2 and sticks to C++17.
<!-- /only -->
`-Wall -Wextra -Wpedantic` are also enabled in the `CMakeLists.txt` of every exercise in the drill,
and we use them here so that you get used to writing under the same conditions from the start.

All the code and output shown here were actually compiled and checked with g++ 13.3.0 on Ubuntu 24.04.

## Table of contents

### Part I — The shape of the C++ language (Chapters 1-3)

| Chapter | Title | Question it answers |
| --- | --- | --- |
| [1](01_how_build_and_link_work.md) | How build and link work | Why do we split `.hpp` and `.cpp`? What is `undefined reference` telling us? |
| [2](02_classes_and_initialization.md) | Classes and initialization | What is the colon in `: Node("minimal_publisher"), count_(0)`? |
| [3](03_inheritance.md) | **Inheritance** — what does it mean to inherit `rclcpp::Node`? | Read `class MinimalPublisher : public rclcpp::Node` from left to right. Why is everything a `shared_ptr`? |

**Chapter 3 is longer than the others.** All 15 exercises in the drill start with this one line,
so we go as far as `virtual` / `override` / virtual destructors / slicing / copy prohibition.
When you read the autonomous navigation code (`SimplePlanner`, the `SystemInterface` plugins),
the second half of this chapter is what you need.

### Part II — Ownership (Chapters 4-6)

This is the center of this course. **Most of the trouble with rclcpp is about ownership.**

| Chapter | Title | Question it answers |
| --- | --- | --- |
| [4](04_references_and_const.md) | References and const | In `const std_msgs::msg::String & msg`, do we need both `&` and `const`? |
| [5](05_move_and_ownership.md) | Move and ownership | What does `std::move` in `publish(std::move(msg))` move? |
| [6](06_smart_pointers.md) | Smart pointers | Why does it not work unless you store the return value of `create_publisher()` in a member variable? |

### Part III — Passing functions as values (Chapter 7)

| Chapter | Title | Question it answers |
| --- | --- | --- |
| [7](07_lambdas_and_std_bind.md) | Lambdas and `std::bind` | What is `this` for in `std::bind(&MinimalPublisher::timer_callback, this)`? What is `_1`? |

### Part IV — Types and tools (Chapters 8-12)

| Chapter | Title | Question it answers |
| --- | --- | --- |
| [8](08_auto_and_type_deduction.md) | `auto` and type deduction | When does writing `auto` make a copy? |
| [9](09_reading_templates.md) | Reading templates | What does the `<>` in `create_publisher<std_msgs::msg::String>` decide? |
| [10](10_operator_overloading.md) | Operator overloading | How do you define `operator<`? Why can you chain `qos.reliable().transient_local()`? |
| [11](11_standard_library_toolbox.md) | The standard library toolbox | Why do we need `c_str()` in `msg.data.c_str()`? |
| [12](12_chrono_and_time.md) | `std::chrono` and time | Why can we write `500ms` directly? What does `using namespace std::chrono_literals` do? |

### Part V — Pitfalls (Chapters 13-14)

| Chapter | Title | Question it answers |
| --- | --- | --- |
| [13](13_minimal_concurrency.md) | Minimal concurrency | Why does `future.wait_for(2s)` cause a deadlock? |
| [14](14_error_handling.md) | Error handling | Why is there almost no `try` / `catch` in ROS 2 code? |

### Wrap-up

| Chapter | Title |
| --- | --- |
| [15](15_checklist.md) | Checklist before you start the drill |

## How to proceed

**Read from the top, in order.** Chapter 5 depends on Chapter 4, and Chapter 6 depends on Chapter 5.

Each chapter has the following structure.

- **Goal of this chapter** — one paragraph. Use it to decide whether to skip
- **Main text** — in each section: "working code" → "output" → "why it happens"
- **Try it yourself** — a single file that works when you copy and paste it. **Predict the output, then run it**
- **Common pitfalls** — mistakes people actually make with the feature, and the messages the compiler prints
- **Where it appears in the drill** — the matching exercise number and the actual lines of that file
- **Matching exercise** — the exercise to solve after reading the chapter (shown in the form `./drill run cpp06`)

Do not skip "Try it yourself".
Ownership and move are things you get wrong when you write them, even if you understood them by reading.
**Predict the output and run it. The places where you were wrong are your own gaps.**

## Estimated time

| Part | Chapters | Estimate |
| --- | --- | --- |
| Part I | 1-3 | 120-150 min (Chapter 3 is long) |
| Part II | 4-6 | 120-150 min (do not rush here) |
| Part III | 7 | 45-60 min |
| Part IV | 8-12 | 120-150 min |
| Part V | 13-14 | 45-60 min |
| Wrap-up | 15 | 15 min |

The total is 8 to 11 hours. **It is not meant to be finished in one day.**
A realistic plan is Part I plus Part II on day one, and the rest on day two.

The shortest path when you are in a hurry is **Chapter 2 → 3 → 6 → 7 → 11** (about 4 hours).
Only these 5 chapters get you into the beginner exercises `01` to `05` of the ROS 2 track.
However, you need Chapter 5 (move) before you start exercise `14` (zero-copy).
**You cannot skip Chapter 3.** The first line of every exercise is that chapter.

## After this course

When you have filled in the checklist in Chapter 15, start the drill.

```bash
./drill watch
```

The course on ROS 2 itself is in the [ROS 2 track](../ros2/01_start_here_course_hub.md) (24 pages in total).
The design philosophy of the ROS 2 side is in [rclcpp design philosophy](../rclcpp_design_philosophy.md).
**This C++ course covers "how the language works", and that one covers "how the library is designed".**
Both explain "why we keep the return value as a member",
but this course explains it from the C++ feature `weak_ptr`, and that one explains it from how the rclcpp headers are implemented.

The coding conventions are in [ROS 2 coding conventions](../ros2_coding_conventions.md).
The code in this course follows them: pointers are written `char * c`, indentation is 2 spaces,
and braces are always used.
