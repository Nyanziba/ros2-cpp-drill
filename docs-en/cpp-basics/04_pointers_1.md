# 4. Pointers 1 (Addresses and Dereferencing)

> **Goal of this chapter**: References seem to be enough, so why do we need pointers? There are three reasons.
> A pointer **can be null** (it can express "optional"), it **can be reseated** (switched to another object), and it **supports pointer arithmetic** (walking through an array).
> This chapter shows concretely why these matter in real code.

## 4.1 Why do we need pointers?

References (chapter 3) are simple and safe, but they have limits in three areas.

| Requirement | Reference | Pointer | Example use |
| --- | --- | --- | --- |
| Express null | ✗ | ✓ | Express "not found" as a search result |
| Point the same variable at different objects | ✗ | ✓ | Move to the next node while walking a list |
| Do arithmetic on addresses | ✗ | ✓ | Process array elements in order |

Whenever one of these three requirements, which references cannot meet, appears, pointers do the job.

### Example: a search function

```cpp
// Cannot be written with a reference (cannot express "not found")
int & search(int * arr, int size, int target);

// Possible with a pointer (nullptr expresses "not found")
int * search(int * arr, int size, int target) {
  for (int i = 0; i < size; ++i) {
    if (arr[i] == target) return &arr[i];
  }
  return nullptr;  // Not found
}
```

## 4.2 Getting an address (`&`)

`&x` gives you the address of the variable `x` (its location in memory).

```cpp
int x = 42;
std::cout << &x;  // 0x7fff5fbff8ac (the address differs on every run)
```

A pointer is a variable that stores this address.

## 4.3 Pointer variables (`int *`)

A pointer is a variable that stores an "address".

```cpp
int x = 42;
int * p = &x;      // p stores the address of x

std::cout << p;    // 0x... (the address)
std::cout << *p;   // 42 (the value it points to)
```

- `p`: the pointer itself (the address)
- `*p`: the value the pointer points to (dereference)

## 4.4 Changing a value by dereferencing

You can change the value a pointer points to through the pointer.

```cpp
int x = 10;
int * p = &x;
*p = 100;
std::cout << x;  // 100 (x changed through p)
```

This has the same effect as a reference. The difference is that a pointer can be reseated.

## 4.5 nullptr: the null pointer

A pointer uses `nullptr` (the null pointer) to express "points to nothing".

```cpp
int * p = nullptr;

if (p) {
  std::cout << *p;  // Safe to dereference when it is not null
} else {
  std::cout << "pointer is null";  // Skip the work when it is null
}
```

This **null check** lets you express an optional value.

## 4.6 Dereferencing a null pointer crashes

If you dereference a null pointer, the program crashes (segmentation fault).

```cpp
// segv.cpp
#include <iostream>

int main()
{
  int * bad_ptr = nullptr;
  std::cout << *bad_ptr << "\n";   // Segmentation fault
  return 0;
}
```

**Crash it for real, just once.** If you do not know the symptom, you will not recognize it when it happens on a real robot.

```bash
$ g++ -std=c++17 segv.cpp -o segv
$ ./segv
Segmentation fault (core dumped)
$ echo $?
139
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/b8v4hMooz)

**It compiles. There is no warning either.** It fails at run time.
Exit code 139 means `128 + 11` (SIGSEGV).
If you see this number in a shell script or in CI, suspect a null dereference or an out-of-range access.

**Always check for null before you dereference a pointer.**
References (chapter 3) do not need this extra work. This is the price of "can be null".

## 4.7 Reseating a pointer

Unlike a reference, a pointer can be changed to point at another object.

```cpp
int x = 10, y = 20;
int * ptr = &x;
std::cout << *ptr;  // 10

ptr = &y;           // ptr now points to y
std::cout << *ptr;  // 20
```

This is essential for moving to the next node in a linked list and similar work.

## 4.8 Accessing struct members

There are two ways to access a struct member through a pointer.

```cpp
struct Point { int x, y; };
Point p{10, 20};
Point * pp = &p;

std::cout << (*pp).x;  // 10 (dereference first, then access the member)
std::cout << pp->x;    // 10 (pointer member access operator)
```

The two mean the same thing. **`->` is shorthand for `(*p).`**.

## 4.9 Comparing value, reference, and pointer

| Property | Value | Reference | Pointer |
| --- | --- | --- | --- |
| Is a copy made? | ✓ Yes | ✗ No | ✗ No |
| Can it be null? | N/A (it is the value itself) | ✗ No | ✓ Yes |
| Can it be reseated? | N/A (assigning gives a different value) | ✗ No | ✓ Yes |
| How the caller writes it | `f(a)` | `f(a)` | `f(p)` or `f(&a)` |

## Try it yourself

**Check the basics of pointers with 9 patterns.**

```cpp
// basics04_practice.cpp
#include <iostream>

int main()
{
  std::cout << "=== Pointers 1 (addresses and dereferencing) ===\n\n";

  std::cout << "--- 1. Why do we need pointers? ---\n";
  std::cout << "Are references enough? ... No. Reasons:\n";
  std::cout << "  1. A pointer can be null (it can express an uninitialized state)\n";
  std::cout << "  2. A pointer can be reseated (switched to another object)\n";
  std::cout << "  3. A pointer can walk an array (pointer arithmetic)\n\n";

  std::cout << "--- 2. Taking an address (&) ---\n";
  int x = 42;
  std::cout << "int x = 42;\n";
  std::cout << "address of x: &x = " << &x << "\n";
  std::cout << "size of int: sizeof(int) = " << sizeof(int) << " bytes\n\n";

  std::cout << "--- 3. Pointer variable (int *) ---\n";
  int * p = &x;
  std::cout << "int * p = &x;\n";
  std::cout << "p (address) = " << p << "\n";
  std::cout << "*p (dereference: the value pointed to) = " << *p << "\n";
  std::cout << "sizeof(int*) = " << sizeof(int*) << " bytes (depends on the architecture)\n\n";

  std::cout << "--- 4. Writing through a dereference ---\n";
  *p = 100;
  std::cout << "*p = 100;\n";
  std::cout << "x = " << x << " (x changed through p)\n\n";

  std::cout << "--- 5. nullptr check ---\n";
  int * nullable = nullptr;
  std::cout << "int * nullable = nullptr;\n";
  std::cout << "nullable = " << nullable << " (null pointer)\n";
  std::cout << "You can check it with if (nullable) { ... }\n";
  if (nullable) {
    std::cout << "pointer is valid\n";
  } else {
    std::cout << "pointer is null\n";
  }
  std::cout << "\n";

  std::cout << "--- 6. Dereferencing a null pointer crashes ---\n";
  std::cout << "Warning: the next code is not executed. We only explain it\n";
  std::cout << "int * bad_ptr = nullptr;\n";
  std::cout << "std::cout << *bad_ptr;  // Segmentation fault!\n";
  std::cout << "If you really ran it, this program would crash\n\n";

  std::cout << "--- 7. Reseating a pointer ---\n";
  int y = 99;
  int * ptr = &x;
  std::cout << "int * ptr = &x;  // ptr points to x\n";
  std::cout << "*ptr = " << *ptr << "\n";
  ptr = &y;
  std::cout << "ptr = &y;  // ptr now points to y\n";
  std::cout << "*ptr = " << *ptr << "\n";
  std::cout << "x = " << x << " (unchanged)\n\n";

  std::cout << "--- 8. Accessing struct members ---\n";
  struct Point {
    int x, y;
  };
  Point pt{10, 20};
  Point * pp = &pt;
  std::cout << "struct Point { int x, y; };\n";
  std::cout << "Point pt{10, 20};\n";
  std::cout << "Point * pp = &pt;\n";
  std::cout << "(*pp).x = " << (*pp).x << "\n";
  std::cout << "pp->x = " << pp->x << " (same)\n";
  std::cout << "pp->y = " << pp->y << "\n\n";

  std::cout << "--- 9. Comparing value, reference, and pointer ---\n";
  std::cout << "+----------+----------+----------+----------+\n";
  std::cout << "| Kind     | nullable | reseat   | Meaning  |\n";
  std::cout << "+----------+----------+----------+----------+\n";
  std::cout << "| Value    | no       | N/A      | Copy     |\n";
  std::cout << "| Ref      | no       | no       | Alias    |\n";
  std::cout << "| Pointer  | yes      | yes      | Address  |\n";
  std::cout << "+----------+----------+----------+----------+\n";

  return 0;
}
```

**Predict: check whether the `nullptr` check works in 5, and whether the pointer switches to y in 7.**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic basics04_practice.cpp -o basics04_practice && ./basics04_practice
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/7Ys9ed7xo)

Output:

<details markdown="1"><summary>Answer (actual output)</summary>

```
=== Pointers 1 (addresses and dereferencing) ===

--- 1. Why do we need pointers? ---
Are references enough? ... No. Reasons:
  1. A pointer can be null (it can express an uninitialized state)
  2. A pointer can be reseated (switched to another object)
  3. A pointer can walk an array (pointer arithmetic)

--- 2. Taking an address (&) ---
int x = 42;
address of x: &x = 0x7ffffffc5908
size of int: sizeof(int) = 4 bytes

--- 3. Pointer variable (int *) ---
int * p = &x;
p (address) = 0x7ffffffc5908
*p (dereference: the value pointed to) = 42
sizeof(int*) = 8 bytes (depends on the architecture)

--- 4. Writing through a dereference ---
*p = 100;
x = 100 (x changed through p)

--- 5. nullptr check ---
int * nullable = nullptr;
nullable = 0 (null pointer)
You can check it with if (nullable) { ... }
pointer is null

--- 6. Dereferencing a null pointer crashes ---
Warning: the next code is not executed. We only explain it
int * bad_ptr = nullptr;
std::cout << *bad_ptr;  // Segmentation fault!
If you really ran it, this program would crash

--- 7. Reseating a pointer ---
int * ptr = &x;  // ptr points to x
*ptr = 100
ptr = &y;  // ptr now points to y
*ptr = 99
x = 100 (unchanged)

--- 8. Accessing struct members ---
struct Point { int x, y; };
Point pt{10, 20};
Point * pp = &pt;
(*pp).x = 10
pp->x = 10 (same)
pp->y = 20

--- 9. Comparing value, reference, and pointer ---
+----------+----------+----------+----------+
| Kind     | nullable | reseat   | Meaning  |
+----------+----------+----------+----------+
| Value    | no       | N/A      | Copy     |
| Ref      | no       | no       | Alias    |
| Pointer  | yes      | yes      | Address  |
+----------+----------+----------+----------+
```

</details>

Check these three points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. **Null check**: In 5, is `pointer is null` printed?
2. **Reseating**: In 7, does the pointer switch with `ptr = &y`?
3. **struct access**: In 8, do `(*pp).x` and `pp->x` give the same result?

</details>

## Common pitfalls

**"I am still unsure about the difference between a pointer and a reference"**

A reference is an "alias" and always refers to something. A pointer is a "variable that stores an address", and it can be null or be reseated.
Compare the output of 4 and 5 and the difference becomes clear.

**"I do not see the difference between -> and (*p)."**

`pp->x` is shorthand for `(*pp).x`. They mean the same thing.
In practice `pp->x` is used more often, but either is fine.

**"Why is an address a big hexadecimal number?"**

A memory address differs with the environment (OS, machine, timing of the run).
Writing it in hexadecimal is just a convention. The value itself is not important. It is enough to read it as "different addresses mean independent variables, the same address means the same variable".

## Matching exercise

After reading this chapter, practice with the matching drill.

- `cppb04_pointer`: use pointers

```bash
./drill run cppb04
```

If you get stuck, run `./drill hint cppb04`. From the exercise side, `./drill read cppb04` brings you back to this chapter.

## References

- [C++ 4. References and const](../cpp/04_references_and_const.md): a detailed comparison of pointers and references
- [C++ 6. Smart pointers](../cpp/06_smart_pointers.md): the modern C++ alternative to raw pointers

---

Previous → [3. References](03_references.md)
Next → [5. Pointers 2](05_pointers_2.md)
