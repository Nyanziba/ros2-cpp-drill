# C++ Basics — learn to read the decorations on a declaration (10 chapters)

Read this course **before the [C++ track](../cpp/README.md).**

## Who it is for

- **You can write loops, assignments, declarations, and functions.** It can be in another language or in C++
- But you stop when `const` or `static` appears
- You cannot yet tell pointers and references apart

**It is not for people who do not know the syntax.** It fixes the state where you know the syntax but cannot read the code.

## Why this structure

Not understanding `const` or `static` does not mean you do not know the syntax. It means
**you do not know what the decorations on a declaration mean.**

```cpp
static const int kMaxSpeed = 10;
inline int add(int x);
explicit Node(const rclcpp::NodeOptions & options);
double length_squared() const;
```

Even someone who understands loops and assignment cannot read these four lines.
This is because **there are unfamiliar words before and after the type.**
So this course is not arranged by feature. It is built to **remove the decorations one at a time.**

The other axis is **pointers.** The [C++ track](../cpp/README.md) does not deal with pointers head on
(it only compares them with references in chapter 4). This Basics course fills that gap.

## Contents

### Part I — Reading declarations (chapters 1 to 2)

| Chapter | Title | The question this chapter answers | Exercise |
| --- | --- | --- | --- |
| [1](01_reading_declarations.md) | Reading declarations | How do you read `const int * const p`? | `cppb01` |
| [2](02_scope_and_lifetime.md) | Scope and lifetime | What disappears when you leave `{}`? | `cppb02` |

### Part II — References and pointers (chapters 3 to 5)

**This is one of the two centers of the Basics course.** The C++ track assumes it.

| Chapter | Title | The question this chapter answers | Exercise |
| --- | --- | --- | --- |
| [3](03_references.md) | References | What is the `&` in `int & x`? It is the same symbol as the address-of `&` | `cppb03` |
| [4](04_pointers_1.md) | Pointers 1 — addresses and dereferencing | Why do we need pointers? What is `nullptr`? | `cppb04` |
| [5](05_pointers_2.md) | Pointers 2 — arrays and when to use which | Should you use a reference or a pointer? | `cppb05` |

### Part III — Qualifiers (chapters 6 to 8)

**This is the other center of this course.**

| Chapter | Title | The question this chapter answers | Exercise |
| --- | --- | --- | --- |
| [6](06_const.md) | **`const`** | The same `const` appears in four places. Do they all mean different things? | `cppb06` |
| [7](07_static.md) | **`static`** | `static` has three meanings. How do you tell them apart? | `cppb07` |
| [8](08_other_qualifiers.md) | Other qualifiers | What are `inline`, `explicit`, `mutable`, and `constexpr` for? | `cppb08` |

### Part IV — Values and files (chapters 9 to 10)

| Chapter | Title | The question this chapter answers | Exercise |
| --- | --- | --- | --- |
| [9](09_value_semantics.md) | Value semantics | After `b = a;`, if you change `a`, does `b` change too? | `cppb09` |
| [10](10_headers_and_project_layout.md) | Headers and project layout | Why do we split code into `.hpp` and `.cpp`? | `cppb10` |

## Docs and the drill match one to one

Chapter numbers and exercise numbers are the same.

```
docs/cpp-basics/06_const.md   ←→   exercises/cppb06_const
```

```bash
./drill list           # shows [matching chapter] to the right of each exercise
./drill read cppb06    # opens the chapter from the exercise
```

## You do not need ROS 2

**These 10 chapters do not use ROS 2 at all.** The "Try it yourself" section of each chapter is a single file.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

All the code and output shown here are values from actually compiling and running
with g++ 13.3.0 on Ubuntu 24.04.

## Estimated time

| Part | Chapters | Estimate |
| --- | --- | --- |
| Part I | 1 to 2 | 60 to 75 minutes |
| Part II | 3 to 5 | 120 to 150 minutes (do not rush here) |
| Part III | 6 to 8 | 120 to 150 minutes |
| Part IV | 9 to 10 | 60 to 75 minutes |

The total is 6 to 8 hours. **It is not meant to be finished in one day.**

Even if you are in a hurry, **you cannot skip chapters 3, 4, and 6.** References, pointers, and `const`
are the basis of everything that follows.

## After this course

Go on to the [C++ track](../cpp/README.md) (15 chapters).
After that comes the [ROS 2 track](../ros2/01_start_here_course_hub.md).

The big picture is in the [Overview of the materials](../README.md).
