# 3. Pointers 1 — addresses and dereferencing

> **Goal of this chapter**: A pointer is "a variable that stores a memory address". With two operators, `&` (address-of) and `*` (dereference), you can read and write the address of a variable, and change a value through that address. With pass by value, the caller does not change. But if you pass a pointer, a function can rewrite the caller's variable. When you understand this mechanism, the flow of control in C becomes much clearer.

## 3.1 Address and reference — `&` and `*`

**`&` — the address-of operator**

It gets the memory address of a variable.

```c
int x = 42;
int *p = &x;  // p holds an address
```

`&x` is "the memory address where x is stored".

**`*` — the dereference operator**

It reads and writes the value at the address the pointer points to.

```c
int *p = &x;
printf("%d\n", *p);  // the same value as x
*p = 100;            // x also changes to 100
```

`*p` is "the value in the memory that p points to".

## 3.2 Declaring a pointer variable

**Put `*` after the type.**

```c
int *p;       // pointer to int
double *q;    // pointer to double
int **pp;     // pointer to a pointer to int (double pointer)
```

The position of `*` means "this variable is a pointer".

## 3.3 NULL — an address that points to nothing

**`NULL` is a special value meaning "points to nothing"**

```c
int *p = NULL;
if (p == NULL) {
  printf("pointer is NULL\n");
}
```

When a returned pointer from a function means "failure", the function may return `NULL`.

```c
int *allocate(int size)
{
  if (size <= 0) {
    return NULL;  // failure
  }
  // ...
}
```

The caller must always check it.

```c
int *p = allocate(10);
if (p == NULL) {
  printf("allocation failed\n");
  return;
}
```

If you do `*p` on a pointer that is `NULL`, you get a **segmentation fault**.

## 3.4 Pass by value — the caller does not change

When you pass a value to a function, **a copy is passed to the function**.

```c
void increment(int x)
{
  x++;  // changes the local variable x
}

int main()
{
  int n = 10;
  increment(n);
  printf("%d\n", n);  // still 10 (unchanged)
}
```

Even if you change `x` inside the function, `n` in the caller does not change.

## 3.5 Pass by pointer — rewriting the caller

If you pass a pointer to a function, **it rewrites the original variable through the address**.

```c
void increment(int *p)
{
  (*p)++;  // changes the variable p points to
}

int main()
{
  int n = 10;
  increment(&n);
  printf("%d\n", n);  // 11 (changed)
}
```

You pass the address of `n` with `increment(&n)`, and when the function changes `*p`, `n` changes.

**Watch the parentheses:**

```c
(*p)++;   // adds 1 to the value p points to (correct)
*p++;     // advances p by 1 (the pointer moves)
```

The latter means "advance the pointer to the next address".

## 3.6 `const` and pointers

**`const int *p` — the pointed-to value is `const`**

```c
const int *p = &x;
*p = 100;  // error (cannot rewrite the pointed-to value)
p = &y;    // OK (the pointer can move)
```

Read it as "pointer to a `const int`".

**`int * const p` — the pointer is `const`**

```c
int * const p = &x;
*p = 100;  // OK (can rewrite the pointed-to value)
p = &y;    // error (the pointer cannot move)
```

Read it as "a `const` pointer to `int`".

**How to remember: read the left side of `const`.**

- `const int *` — the left of `const` is `int` → the `int` is `const`
- `int * const` — the left of `const` is `*` → the pointer is `const`

## Try it yourself

Check the basics of pointers, the difference between pass by value and pass by pointer, and `const` pointers.

### Example 1: Address and dereference

**Predict**: Does `p` point to the same value as `x`? Does `x` also change with `*p = 100`?

```c
// ch03_ptr_handson.c
#include <stdio.h>

void increment_by_value(int x)
{
  x++;
}

void increment_by_pointer(int *p)
{
  (*p)++;
}

int main()
{
  printf("== Passing by value ==\n");
  int x = 10;
  printf("Before: x = %d\n", x);
  increment_by_value(x);
  printf("After increment_by_value(x): x = %d\n", x);

  printf("\n== Passing by pointer ==\n");
  int y = 10;
  printf("Before: y = %d\n", y);
  increment_by_pointer(&y);
  printf("After increment_by_pointer(&y): y = %d\n", y);

  printf("\n== Pointer to pointer ==\n");
  int value = 42;
  int *p = &value;
  int **pp = &p;

  printf("value = %d\n", value);
  printf("*p = %d\n", *p);
  printf("**pp = %d\n", **pp);

  printf("\n== const pointer ==\n");
  int a = 100;
  int b = 200;

  const int *cp = &a;  // pointer to const int
  printf("const int *cp = &a: %d\n", *cp);
  cp = &b;  // OK - can move pointer
  printf("After cp = &b: %d\n", *cp);

  int * const p_const = &a;  // const pointer to int
  printf("int * const p_const = &a: %d\n", *p_const);
  *p_const = 150;  // OK - can modify pointee
  printf("After *p_const = 150: a = %d\n", a);

  return 0;
}
```

Compile and run:

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic ch03_ptr_handson.c -o ch03_ptr_handson && ./ch03_ptr_handson
```

Output:

<details markdown="1"><summary>Answer (actual output)</summary>

```
== Passing by value ==
Before: x = 10
After increment_by_value(x): x = 10

== Passing by pointer ==
Before: y = 10
After increment_by_pointer(&y): y = 11

== Pointer to pointer ==
value = 42
*p = 42
**pp = 42

== const pointer ==
const int *cp = &a: 100
After cp = &b: 200
int * const p_const = &a: 100
After *p_const = 150: a = 100
```

</details>

Check these 3 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. **Pass by value** — after returning from the function, `x` is still 10 (because a copy was passed)
2. **Pass by pointer** — after returning from the function, `y` has changed to 11 (because it was rewritten through the address)
3. **`const` pointers** — with `const int *cp` you can move the pointer (because the left side is `const`), and with `int * const p_const` you can rewrite the pointed-to value (because the right side is `const`)

</details>

### Example 2: The relation between an address and a value

Let us look at the actual value of an address, and at pointer arithmetic.

```c
#include <stdio.h>

int main()
{
  int x = 42;
  int *p = &x;

  printf("Variable x: %d\n", x);
  printf("Address of x (&x): %p\n", (void *)&x);
  printf("Pointer p: %p\n", (void *)p);
  printf("Value pointed to (*p): %d\n", *p);

  printf("\n== Modifying through pointer ==\n");
  *p = 100;
  printf("After *p = 100:\n");
  printf("x = %d\n", x);
  printf("*p = %d\n", *p);

  return 0;
}
```

Example output:

```
Variable x: 42
Address of x (&x): 0x7ffc2d5d030c
Pointer p: 0x7ffc2d5d030c
Value pointed to (*p): 42

== Modifying through pointer ==
After *p = 100:
x = 100
*p = 100
```

The address value of `p` changes on every run, but it matches the address of `x`.

## Common pitfalls

**Compile error with `int * p; printf("%d", p);`**
`p` is an uninitialized pointer (garbage value).
Initialize it with `int *p = NULL;`, or always assign a valid address.

**The value of `*p` is strange (segmentation fault)**
`p` points to `NULL` or an invalid address.
Check `p == NULL`, or check that `p` points to a valid address.

**You passed a pointer but the caller did not change**
Did you forget the `&`?
In the function definition, `int x` is pass by value and `int *p` is pass by pointer.

**`error: invalid type argument of unary '*' (have 'int')`**
You applied `*` to a variable. `*p` is OK, but `*x` (when `x` is an `int`) is an error.

**You cannot tell `const int *p` from `int * const p`**
Read the `const` from its left side.
- `const int *p` — the left side is `int`, so "the `int` is `const`"
- `int * const p` — the left side is `*`, so "the `*` (pointer) is `const`"

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c03_pointer` — pass an address with a pointer

```bash
./drill run c03
```

If you get stuck, use `./drill hint c03`. From the exercise side, `./drill read c03` brings you back to this chapter.

## References

- C99 standard, [Declarators](https://www.open-std.org/JTC1/SC22/WG14/www/docs/n1570.pdf) (PDF, see 5.7)
- [cppreference: pointers](https://en.cppreference.com/w/c/language/pointer)
- [ROS 2 coding conventions](../ros2_coding_conventions.md) — how to use pointers

---

Previous chapter → [2. Fixed-width integers and implicit conversions](02_fixed_width_integers_and_implicit_conversions.md)

Next chapter → [4. Pointers 2 — arrays and pointer arithmetic](04_pointers_2.md)
