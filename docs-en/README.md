# Overview of the materials

`docs-en/` (English) and `docs/` (Japanese) hold the reading, and `exercises/` holds the hands-on exercises. **They match one to one.**

```
docs-en/cpp-basics/06_const.md   ←→   exercises/cppb06_const
```

> All tracks are translated. Comments in the exercise source code stay in Japanese. Test names are in English, and test failure messages switch to English with `DRILL_LANG=en`.

**If you want to start by running the exercises, see [Getting started](getting-started.md).**
You can work with Docker, or install directly on Ubuntu.

These materials come in a **Jazzy version (the default) and a Lyrical version**.
You can switch between them with the switcher at the top of each page of the site.
The Lyrical version is at <https://nyanziba.github.io/ros2-cpp-drill/lyrical/en/>.

There are two ways to move between them.

```bash
./drill list          # shows [matching chapter] to the right of each exercise
./drill read cppb06   # opens the chapter from the exercise
```

Each chapter ends with a "Matching exercise" section, so you can also go the other way.

## Three tracks

| Track | Reading | Exercises | Who it is for |
| --- | --- | --- | --- |
| **C++ Basics** | [cpp-basics/](cpp-basics/README.md) | `cppb01` to `cppb10` | People who get stuck on `const` and `static`. 10 chapters |
| **C++** | [cpp/](cpp/README.md) | `cpp01` to `cpp12` | Preparation for reading rclcpp. 15 chapters |
| **ROS 2** | [ros2/](ros2/01_start_here_course_hub.md) | `01` to `18` | 24 articles in total |

**C++ Basics and C++ do not use ROS 2.** They need only `g++` and gtest.

### How C++ Basics and C++ divide the work

**C++ Basics is the track that teaches you to read the decorations on a declaration.**
`const`, `static`, `inline`, `explicit`, and also pointers and references.
It fixes the state where you understand loops and assignment but still cannot read the code.

**The C++ track teaches you to read rclcpp code.**
It covers ownership, move, smart pointers, lambdas, and how to read templates.

C++ Basics owns the fundamentals of `const` and references. The four chapters of the C++ track
focus on "assuming you know the basics, how do you choose in rclcpp".

## Extra references for these materials

These are cross-cutting readings that are not tied to any exercise or chapter.

- [The design philosophy of rclcpp](rclcpp_design_philosophy.md) — why the API is designed this way.
  Written by actually reading the rclcpp headers (the Jazzy ones)
- [ROS 2 coding conventions](ros2_coding_conventions.md) — a summary of the official
  Code style and language versions
- [Presentation slides](slides.md) — slides for presenting these materials.
  Three PDFs are provided: 3 minutes, 5 to 10 minutes, and 15 minutes

## About the code and output shown here

**Every compile error and run result in these materials is real output from an actual run.**
Where my prediction and the real result differed, I used the real result.
The environment is Ubuntu 24.04 / g++ 13.3.0 / ROS 2 Jazzy.

The "Try it yourself" section in each chapter is written on the assumption that you **predict first, then run**.
The places where you were wrong are your own gaps, so please do not skip it.
