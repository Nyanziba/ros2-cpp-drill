# 10. Headers and project layout

> **Goal of this chapter**: In C++, the reason to split code into `.h` files and `.cpp` files is a **division of work between the compiler and the linker**. Once you understand declaration vs definition, translation units, and how include works, you can see what the project layout and the error messages mean.

## 10.1 Declaration and definition — the jobs of the compiler and the linker

**Declaration**: the information "a function/variable with this name and type exists". **The compiler needs it.**

**Definition**: the actual body, "what the implementation of that function/variable is". **Both the compiler and the linker need it.**

```cpp
// declaration only
int add(int a, int b);

// declaration + definition
int add(int a, int b) {
  return a + b;
}
```

**The compiler** is fine with only a declaration when you call `add(10, 20)`.
**The linker** looks for the implementation of that function in a definition.

### When a declaration is missing

```cpp
int main() {
  int x = add(10, 20);  // add() is not declared
  return 0;
}
```

[⚠ See this error in your browser (gcc 13.3)](https://godbolt.org/z/9Pnf7nx1z)

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// not_declared.cpp
int main() {
  int x = add(10, 20);  // add() is not declared
  return 0;
}
```

```bash
g++ -std=c++17 not_declared.cpp -o not_declared
```

</details>

```
error: ‘add’ was not declared in this scope
```

**This is a compile error.** The compiler cannot find a declaration of `add()`.

### When a definition is missing

```cpp
// declaration of add
int add(int a, int b);

int main() {
  int x = add(10, 20);  // compiles OK
  return 0;
}

// there is no definition of add
```

[⚠ See this error in your browser (gcc 13.3)](https://godbolt.org/z/aP6GPnfs7)

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// no_definition.cpp
// declaration of add
int add(int a, int b);

int main() {
  int x = add(10, 20);  // compiles OK
  return 0;
}

// there is no definition of add
```

```bash
g++ -std=c++17 -c no_definition.cpp -o no_definition.o && g++ no_definition.o -o no_definition
```

</details>

```
/usr/bin/ld: no_definition.o: in function `main':
no_definition.cpp:(.text+0x17): undefined reference to `add(int, int)'
collect2: error: ld returned 1 exit status
```

**This is a link error.** The compiler succeeded, but the linker cannot find the definition.

## 10.2 Translation units — each `.cpp` is compiled independently

**Each `.cpp` file is compiled independently and separately.** Each one becomes an `.o` (object) file, and at the end the linker combines them into one.

```bash
g++ -c file1.cpp -o file1.o   # compile file 1
g++ -c file2.cpp -o file2.o   # compile file 2
g++ file1.o file2.o main.o    # link the three
```

**Inside each translation unit, only the declarations written in it are visible.** To see declarations from another file, read its header with `#include`.

Example: if you write just `#include <string>` in a 2-line `.cpp`, the preprocessor **expands it to more than 25,000 lines.**

```bash
echo '#include <string>' > simple.cpp
echo 'int main() { return 0; }' >> simple.cpp
g++ -E simple.cpp | wc -l
```

```
25325
```

`#include` is **text replacement.** It pastes the contents of the header file as they are.

## 10.3 The multiple include problem and `#pragma once`

If you `#include` a header from several files, the same content is expanded several times.
If a struct or class definition is duplicated, you get a **redefinition error**.

```cpp
// no_guard.h
struct Point {
  int x;
  int y;
};
```

```cpp
// main.cpp
#include "no_guard.h"
#include "no_guard.h"  // second time
int main() { return 0; }
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// no_guard.h
struct Point {
  int x;
  int y;
};
```

```cpp
// main.cpp
#include "no_guard.h"
#include "no_guard.h"  // second time
int main() { return 0; }
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -c main.cpp
```

</details>

```
In file included from main.cpp:3:
no_guard.h:2:8: error: redefinition of ‘struct Point’
    2 | struct Point {
      |        ^~~~~
In file included from main.cpp:2:
no_guard.h:2:8: note: previous definition of ‘struct Point’
    2 | struct Point {
      |        ^~~~~
```

**The error points at line 2 of `no_guard.h`** (the `struct Point {` line), not at the `#include` line in
`main.cpp`. `#include` pastes text, so from the compiler's point of view the problem is in the pasted text.
If you read the `In file included from ...` chain from bottom to top, you can see how it got there.

**The fix is `#pragma once`.** Put it at the top of the header. It prevents the same file from being included a second time.

```cpp
// with_pragma_once.h
#pragma once

struct Point {
  int x;
  int y;
};
```

```cpp
// main.cpp
#include "with_pragma_once.h"
#include "with_pragma_once.h"  // OK even the second time
int main() {
  Point p = {1, 2};
  (void)p;            // standard idiom to avoid an unused-variable warning
  return 0;
}
```

It compiles. Without `(void)p;`, `-Wall` gives
`warning: unused variable ‘p’` (all exercises in the drill enable `-Wall -Wextra`). `#pragma once` tells the compiler "include this file only once".

**In the past, people used `#ifndef` guards. Today `#pragma once` is fine.**

## 10.4 How to read error messages — compile error vs link error

| Error message | Stage | Cause | Fix |
| --- | --- | --- | --- |
| `error: X was not declared in this scope` | Compile | No declaration | Add a declaration to the `.h`, or `#include` it |
| `error: expected ';' before X` | Compile | Syntax error | Check the grammar |
| `undefined reference to 'X'` | Link | No definition | Add a definition to a `.cpp`, or include it in the link |
| `multiple definition of 'X'` | Link | Duplicate definition | The definition is spread over several `.cpp` files. Keep only one, and make the others `static` or `inline` |
| `undefined reference to 'ClassName::member'` | Link | No definition of a static member (before C++17) | Define it outside the class, or use `inline static` |

## 10.5 A minimal CMake setup

Real projects use CMake.

```cmake
cmake_minimum_required(VERSION 3.10)
project(Demo)

set(CMAKE_CXX_STANDARD 17)

# define a library (from util.cpp)
add_library(util util.cpp)
target_include_directories(util PUBLIC .)

# define an executable (from main.cpp)
add_executable(demo main.cpp)

# link the executable with the library
target_link_libraries(demo PRIVATE util)
```

**Each command:**

- **`add_library(util util.cpp)`**: build a library named `util` from `util.cpp`
- **`target_include_directories(util PUBLIC .)`**: tell CMake that the headers of this library are found in `.`
- **`add_executable(demo main.cpp)`**: build an executable named `demo` from `main.cpp`
- **`target_link_libraries(demo PRIVATE util)`**: link `demo` with the `util` library

**What happens when something is missing:**

- Forget `add_library(util util.cpp)` → `target_link_libraries` cannot find `util`
- Forget `target_include_directories` → compile error "header not found"
- Forget `target_link_libraries` → link error "undefined reference"

## Try it yourself

This program puts together declaration and definition, include guards, and translation units.

```cpp
#include <iostream>

// === DECLARATION vs DEFINITION ===
// Declaration: tells compiler "this function exists"
// Definition: tells compiler AND linker where the code is

// This is a declaration:
int add(int a, int b);

// This is also a declaration + definition:
int subtract(int a, int b) {
  return a - b;
}

// This is a definition for the declared add():
int add(int a, int b) {
  return a + b;
}

// === HEADER FILE INCLUSION ===
// In a real project, declarations go in .h files and definitions in .cpp files.
// When you #include a .h file, the entire text is pasted at that location.

// === REDEFINITION PROBLEM ===
// If a .h file (without #pragma once) is included twice,
// you get redefinition errors. Solution: use #pragma once or include guards.

int main() {
  std::cout << "=== Declaration vs Definition ===\n";
  std::cout << "add(10, 20) = " << add(10, 20) << "\n";
  std::cout << "subtract(20, 10) = " << subtract(20, 10) << "\n";
  
  std::cout << "\n=== Translation Units ===\n";
  std::cout << "Each .cpp file is compiled independently.\n";
  std::cout << "The linker combines the object files.\n";
  std::cout << "Use 'g++ -c file.cpp' to compile to .o, then link with other .o files.\n";
  
  return 0;
}
```

**Predict: Do compiling and running succeed? Are the functions called correctly?**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/Mb6nP5hMh)

<details markdown="1"><summary>Answer (actual output)</summary>

```
=== Declaration vs Definition ===
add(10, 20) = 30
subtract(20, 10) = 10

=== Translation Units ===
Each .cpp file is compiled independently.
The linker combines the object files.
Use 'g++ -c file.cpp' to compile to .o, then link with other .o files.
```

</details>

## Common pitfalls

**`error: X was not declared in this scope`**
The declaration was not found. `#include` the header file.

**`undefined reference to 'X'`**
The definition was not found (a link error). Check the following:
- Is the definition written in a `.cpp` file?
- Is that `.cpp` file a compile target in CMakeLists.txt?
- Is it linked correctly with `target_link_libraries`?

**`error: redefinition of 'struct X'`**
The header contains a struct or class definition, and it is included several times.
Add `#pragma once` at the top of the header.

**CMake cannot find the library**
- Check that it is defined with `add_library(...)`
- Check that it is specified in `target_link_libraries(...)`
- Check for typos

**When you may write a function body in a header**
- Templates
- `constexpr` functions
- `inline` functions
- Member functions (defined inside the class)

Anything else becomes a multiple definition. Add `inline`, or move it to a `.cpp` file.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `cppb10_headers` — split into headers

```bash
./drill run cppb10
```

If you get stuck, run `./drill hint cppb10`. From the exercise side, `./drill read cppb10` brings you back to this chapter.

## References

- `cppreference`: [Translation unit](https://en.cppreference.com/w/cpp/language/translation_unit) and [#include directive](https://en.cppreference.com/w/cpp/preprocessor/include)
- [CMake documentation](https://cmake.org/cmake/help/latest/)
- Next step: [C++ track, 1. How build and link work](../cpp/01_how_build_and_link_work.md) (ODR, name mangling, the `nm` command, and more)

---

Previous chapter → [9. Value semantics](09_value_semantics.md)
