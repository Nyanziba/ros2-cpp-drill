# C Track — C You Actually Write on Microcontrollers (12 chapters)

This course is for **people who write microcontroller code, drive trains, and CAN.**
If you are going on to ROS 2 / autonomous navigation, you can skip it.

## Who this is for

- **You can write loops, assignments, declarations, and functions.** It does not matter whether you learned them in another language or in C
- But you do not know the "decorations on declarations" in C (such as `static`)
- You are not sure what pointers are really doing

**This is not for people who have never programmed.** It is for people who know the basics but feel unsure about the way C does things.

## What kind of course this is

**The C track is an independent track.** It is a course for people who write microcontrollers, drivers, and CAN communication.
It is not a preparation step for C++.

If you are learning ROS 2 (which is built on C++), start with [C++ Basics](../cpp-basics/README.md).
If you are going any other way, this track alone is enough.
This course covers **every C topic you really need on a microcontroller**:
pointers, bit operations, and memory layout.

## Contents

### Part I — Files and types (chapters 1-2)

| Chapter | Title | Question this chapter answers | Exercise |
| --- | --- | --- | --- |
| [1](01_separate_compilation_and_header_guards.md) | Separate compilation and header guards | Why split code into `.h` and `.c`? The two meanings of `static` | `c01` |
| [2](02_fixed_width_integers_and_implicit_conversions.md) | Fixed-width integers and implicit conversions | Why does adding `uint8_t` values give an `int`? | `c02` |

### Part II — Pointers (chapters 3-5)

**This is the center of the C track.** You cannot write C without pointers.

| Chapter | Title | Question this chapter answers | Exercise |
| --- | --- | --- | --- |
| [3](03_pointers_1.md) | Pointers 1 — addresses and dereferencing | What do `&` and `*` do? Which one acts first? | `c03` |
| [4](04_pointers_2.md) | Pointers 2 — arrays and pointer arithmetic | How far does `p + 1` move? Why are arrays and `char *` treated the same? | `c04` |
| [5](05_strings.md) | Strings | Why is `strncpy` dangerous? What is the null terminator for? | `c05` |

### Part III — Memory in practice (chapters 6-8)

| Chapter | Title | Question this chapter answers | Exercise |
| --- | --- | --- | --- |
| [6](06_structs_and_alignment.md) | Structs and alignment | Why does `sizeof` change when you reorder the members? | `c06` |
| [7](07_bit_operations_and_registers.md) | Bit operations and register access | How do you change one bit without breaking the others? What is masking? | `c07` |
| [8](08_manual_memory_management.md) | Manual memory management | Why is use-after-free so hard to notice? | `c08` |

### Part IV — Embedded practice (chapters 9-12)

| Chapter | Title | Question this chapter answers | Exercise |
| --- | --- | --- | --- |
| [9](09_function_pointers.md) | Function pointers | How do you read `int (*fp)(int)`? What are function pointers used for? | `c09` |
| [10](10_volatile_and_interrupt_safety.md) | volatile and interrupt safety | What does `volatile` not guarantee? Why are interrupt handlers hard? | `c10` |
| [11](11_endianness_and_serialization.md) | Endianness and serialization | How do you pack a struct or a `float` into the 8 bytes of a CAN frame? | `c11` |
| [12](12_preprocessor_and_debugging.md) | Preprocessor and debugging | Why are macros full of parentheses? When do you use `#if` and when `#ifdef`? | `c12` |

## Docs and the drill are one to one

Chapter numbers and exercise numbers match.

```
docs-en/c/07_bit_operations_and_registers.md   ←→   exercises/c07_bit_ops
```

```bash
./drill list    # shows the list of exercises
./drill read c07
```

## You do not need ROS 2

**These 12 chapters do not use ROS 2 at all.** Every code example runs with just `gcc`.

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic try.c -o try && ./try
```

Some chapters use extra options:

- **Chapter 4 (arrays and pointer arithmetic) and chapter 8 (manual memory management)**:
  ```bash
  gcc -std=c99 -Wall -Wextra -Wpedantic -fsanitize=address try.c -o try && ./try
  ```
  Memory errors only show up at run time, so AddressSanitizer is required.

- **Chapter 12 (preprocessor and debugging)**:
  ```bash
  gcc -std=c11 -Wall -Wextra -Wpedantic try.c -o try && ./try
  ```
  It uses `_Static_assert`, so the c11 standard is required.

All code and output shown here are values from actually compiling and running with gcc on Ubuntu 24.04 (gcc 13.3.0 for the Jazzy version, 15.2.0 for the Lyrical version).

## Estimated time

| Part | Chapters | Estimate |
| --- | --- | --- |
| Part I | 1-2 | 60-90 minutes |
| Part II | 3-5 | 150-210 minutes (do not rush here) |
| Part III | 6-8 | 120-150 minutes |
| Part IV | 9-12 | 120-150 minutes |

**About 8-10 hours in total. It is not meant to be finished in one day.**

Even if you are in a hurry, **you cannot skip chapters 3, 4, and 7.** Pointers and bit operations
are the core of microcontroller programming.

## After this course

You move on to the microcontroller-side implementation (motor drivers, CAN communication).

If you are going on to ROS 2 / autonomous navigation, go through [C++ Basics](../cpp-basics/README.md) and then
on to [C++](../cpp/README.md).

The big picture is in the [overview of the materials](../README.md).
