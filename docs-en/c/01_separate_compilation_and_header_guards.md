# 1. Separate compilation and header guards

> **Goal of this chapter**: As a program grows, you split it into several files instead of one `.c` file. It is important to understand why you split code into `.h` (header) and `.c` (implementation), what `#include` does, and **the difference between a declaration and a definition**. You will learn the "invisible connections" such as header guards, `static` (file scope), and `extern`. After that, you will be able to read the code you see in team projects.

## 1.1 Why split — separation of responsibilities

As a program grows, it becomes hard to write everything in one `.c` file.

**One-file version:**

```c
#include <stdio.h>

int add(int a, int b)
{
  return a + b;
}

int multiply(int a, int b)
{
  return a * b;
}

int main()
{
  printf("add(5, 3) = %d\n", add(5, 3));
  printf("multiply(4, 7) = %d\n", multiply(4, 7));
  return 0;
}
```

This works, but the implementations of `add` and `multiply` are mixed with `main`.

**Split version:** Move the implementations to separate files.

```c
// math.h - declarations only
int add(int a, int b);
int multiply(int a, int b);
```

```c
// math.c - implementation
#include "math.h"

int add(int a, int b)
{
  return a + b;
}

int multiply(int a, int b)
{
  return a * b;
}
```

```c
// main.c - the user side
#include <stdio.h>
#include "math.h"

int main()
{
  printf("add(5, 3) = %d\n", add(5, 3));
  printf("multiply(4, 7) = %d\n", multiply(4, 7));
  return 0;
}
```

`main.c` learns the **declarations** of `add` and `multiply` through `#include "math.h"`.
The implementations are in `math.c`, and they are joined at **link** time.

## 1.2 The difference between a declaration and a definition

**Declaration**
It only says "this function exists". It has no body (`{}`).

```c
int add(int a, int b);  // declaration
```

**Definition**
It writes the implementation: "this is the body of the function".

```c
int add(int a, int b)   // definition
{
  return a + b;
}
```

**Key points:**

- What you write in a header (`.h`) is a **declaration**
- What you write in a source file (`.c`) is a **definition**
- The same function can be declared in many `.c` files (through `#include`)
- But there must be **only one definition in the whole program**

Several declarations are fine, but two or more definitions cause a "duplicate definition" error at link time.

## 1.3 The two forms of `#include`

**`#include <stdio.h>`**
Looks for the system standard library. It searches the standard paths such as `/usr/include`.

**`#include "math.h"`**
Looks in the current directory (or the project paths).

Use `"` for files in the project. Use `<>` for external libraries.

## 1.4 Header guards — preventing double includes

When you `#include` a header, its contents are expanded at that place.
If you include the same header twice, the declarations appear twice.

```c
#include "math.h"  // first time
#include "math.h"  // second time → the same declarations are duplicated
```

The **header guard** prevents this.

```c
// math.h
#ifndef MATH_H
#define MATH_H

int add(int a, int b);
int multiply(int a, int b);

#endif
```

On the first `#include "math.h"`, `MATH_H` is defined.
On the second `#include "math.h"`, `MATH_H` is already defined, so the condition of `#ifndef MATH_H` is false and the contents are skipped.

**By convention, the guard name is "the file name in upper case plus `_H`".**
For `config.h` it is `CONFIG_H`, and for `driver_can.h` it is `DRIVER_CAN_H`.

## 1.5 `static` — file scope (internal linkage)

Global variables and functions are normally visible from every file (external linkage).
If several files use the same name, they collide at link time.

**Problem example:**

```c
// file1.c
int counter = 0;  // external linkage

int get_count_1(void)
{
  return ++counter;
}
```

```c
// file2.c
int counter = 0;  // same name → link error
```

**Solution: add `static`**

```c
// file1.c
static int counter = 0;  // file scope

int get_count_from_file1(void)
{
  return ++counter;
}
```

```c
// file2.c
static int counter = 0;  // a different variable from the counter in file1

int get_count_from_file2(void)
{
  return ++counter;
}
```

With `static`, the variable is valid only inside that file.
Other files cannot see it, so names do not collide.

**Note: `static` can be used on functions too.**

```c
// internal.c
static int helper(int x)
{
  return x * 2;
}
```

A `static` function can be used only inside that file.

## 1.6 `static` — inside a function (the value stays)

If you declare a `static` variable inside a function, its value is kept across calls.

```c
int next_id(void)
{
  static int counter = 100;
  return ++counter;
}
```

On the first call, `counter` is initialized to 100.
From the second call on, the initialization is skipped and it starts from the previous value.

**In other words:**
- `static int counter = 100;` is "run only the first time"
- `return ++counter;` is "run every time"

The **scope is narrower and safer** than a global variable.

## 1.7 `extern` — using a definition in another file

Normally you learn declarations through `#include`, so `extern` is not needed.
But if you write the same declaration in several `.c` files without a header guard, you may use `extern` to state clearly that "this variable is defined in another file".

```c
extern int global_counter;  // declaration (not a definition)
```

**The difference between `extern` and a normal declaration:**

- `int global_counter;` — at global scope, this counts as a "definition"
- `extern int global_counter;` — even at global scope, this is "only a declaration"

In real code, using `#include` is recommended. Use of `extern` is becoming less common.

## Try it yourself

Compile several files and check how header guards, `static`, and `static` inside a function behave.

### Example 1: Basic separate compilation

**Predict**: What are the results of `add(5, 3)` and `multiply(4, 7)`?

```c
// math.h
#ifndef MATH_H
#define MATH_H

int add(int a, int b);
int multiply(int a, int b);

#endif
```

```c
// math.c
#include "math.h"

int add(int a, int b)
{
  return a + b;
}

int multiply(int a, int b)
{
  return a * b;
}
```

```c
// main.c
#include <stdio.h>
#include "math.h"

int main()
{
  printf("add(5, 3) = %d\n", add(5, 3));
  printf("multiply(4, 7) = %d\n", multiply(4, 7));
  return 0;
}
```

Compile:

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic main.c math.c -o multi_file && ./multi_file
```

Output:

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: files=math.h,math.c,main.c -->
```
add(5, 3) = 8
multiply(4, 7) = 28
```

</details>

### Example 2: `static` inside a function (the value stays across calls)

**Predict**: Does `counter` grow as 1, 2, 3 on each call? Or is it 101 every time?

```c
// static_demo.c
#include <stdio.h>

int next_id(void)
{
  static int counter = 100;
  return ++counter;
}

int main()
{
  printf("next_id() = %d\n", next_id());
  printf("next_id() = %d\n", next_id());
  printf("next_id() = %d\n", next_id());
  return 0;
}
```

Compile and run:

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic -c static_demo.c -o static_demo.o && gcc static_demo.o -o static_demo && ./static_demo
```

Output:

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: -->
```
next_id() = 101
next_id() = 102
next_id() = 103
```

</details>

The value of `counter` is kept across calls.

### Example 3: File-scope `static` (same-named variables in different files)

**Predict**: Are the `counter` in `file1` and the `counter` in `file2` independent? Or are they shared?

```c
// file1.c
#include <stdio.h>

static int counter = 0;

int get_count_from_file1(void)
{
  return ++counter;
}
```

```c
// file2.c
#include <stdio.h>

static int counter = 1000;

int get_count_from_file2(void)
{
  return ++counter;
}
```

```c
// main_static.c
#include <stdio.h>

int get_count_from_file1(void);
int get_count_from_file2(void);

int main()
{
  printf("file1: %d\n", get_count_from_file1());
  printf("file1: %d\n", get_count_from_file1());
  printf("file2: %d\n", get_count_from_file2());
  printf("file2: %d\n", get_count_from_file2());
  return 0;
}
```

Compile and run:

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic main_static.c file1.c file2.c -o static_files && ./static_files
```

Output:

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: files=main_static.c,file1.c,file2.c -->
```
file1: 1
file1: 2
file2: 1001
file2: 1002
```

</details>

The two `counter` variables are completely independent. Thanks to `static`, the name collision was avoided.

## Common pitfalls

**`error: conflicting types for 'add'`**
The same function has different declarations (a different number or type of parameters).
Make the declaration in the header match the definition in the `.c` file.

**`error: undefined reference to 'add'`**
The definition of the function `add` cannot be found.
Link the `.o` files when you compile.
(Example: run `gcc main.c math.c -o ...` and include `math.c`)

**`error: multiple definition of 'counter'`**
The global variable `counter` is defined in several files.
Add `static` to one of them, or delete one.

**`error: 'math.h' file not found`**
The location of `#include "math.h"` cannot be found.
Check that you are in the same directory as the file, or fix the path.

**The `#define` name of a header guard collides**
Take care that header guard names do not overlap.
Do not create two or more of the same `#define HEADER_H` in one project.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c01_split_compile` — separate compilation and header guards

```bash
./drill run c01
```

If you get stuck, use `./drill hint c01`. From the exercise side, `./drill read c01` brings you back to this chapter.

## References

- C99 standard, [Preprocessing directives](https://www.open-std.org/JTC1/SC22/WG14/www/docs/n1570.pdf) (PDF)
- [ROS 2 coding conventions](../ros2_coding_conventions.md) — how to use header guards and `static`

---

Next chapter → [2. Fixed-width integers and implicit conversions](02_fixed_width_integers_and_implicit_conversions.md)
