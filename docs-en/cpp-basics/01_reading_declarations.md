# 1. Reading declarations

> **Goal of this chapter**: A C++ variable declaration can have decorations before and after the type. When you see something like `const int * const p`, you may think "what is this?" and stop making progress in the drill.
> In this chapter, you learn **a procedure for reading from the variable name, right to left.** The procedure is not something to memorize. It is a mechanical process, so you can use it again and again.
> It is required for reading the code in exercise 01.

## 1.1 Reading declarations right to left

If you try to read a C++ declaration from left to right, it does not make sense. **Read it from right to left.**
This is not a matter of taste. It is because the C++ declaration syntax is built that way.

### Procedure

1. **Find the variable name**
2. **Read to the left, one token at a time**
3. **Replace each symbol with words as you go**

This is the whole replacement table.

| Symbol | Reading |
| --- | --- |
| `&` | reference to |
| `*` | pointer to |
| `const` | const (cannot be changed) |
| type name | that type |

### Examples

**Example 1: `int x`**

```
int   x
 ②    ①
```

① `x` is → ② `int`. **"x is an int"**

**Example 2: `int * p`**

```
int   *   p
 ③    ②   ①
```

① `p` is → ② a pointer → ③ to `int`. **"p is a pointer to int"**

**Example 3: `const int * q`**

```
const  int   *   q
  ④     ③    ②   ①
```

① `q` is → ② a pointer → ③④ to a "const int".
**"q is a pointer to const int"**

This is the key point. **`const` is at the left end, but it is attached to `int`, not to `q`.**
It is not "the pointer itself is const". It is "the thing the pointer points to is const".

**Example 4: `int * const r`**

```
int   *   const   r
 ④    ③     ②      ①
```

① `r` is → ② const → ③ a pointer → ④ to `int`.
**Because `const` is to the right of `*`, "the pointer itself is const".**
This means you cannot reseat it.

### The const rule

**`const` attaches to "the thing immediately to its left". If there is nothing on the left, it attaches to the thing on the right.**

| Pattern | What is on the left | Result |
| --- | --- | --- |
| `const int *` | Nothing is to the left of `const` | Attaches to the `int` on the right → the pointed-to value is const |
| `int const *` | `int` is to the left of `const` | Attaches to `int` → the pointed-to value is const (the same) |
| `int * const` | `*` is to the left of `const` | Attaches to `*` (the pointer) → the pointer is const |
| `const int * const` | The first `const` attaches to `int`, and the second to `*` | Both are const |

So `const int *` and `int const *` have **the same meaning.**

## 1.2 Table of all patterns

With the procedure so far, you can read every combination that appears.

| Declaration | Reading | What you can and cannot do |
| --- | --- | --- |
| `int x` | x is an int | You can read and write it |
| `const int x` | x is a const int | You can read it but not write it |
| `int & x` | x is a reference to int | You can read and write the original |
| `const int & x` | x is a reference to const int | You can read the original but not write it |
| `int * x` | x is a pointer to int | You can read and write through `*x`. You can replace `x` |
| `const int * x` | x is a pointer to const int | You can read through `*x` but not write. You can replace `x` |
| `int * const x` | x is a const "pointer to int" | You can write through `*x`. You cannot replace `x` |
| `const int * const x` | x is a const "pointer to const int" | Neither is allowed |

## 1.3 const on a member function (the exception to this procedure)

**Only the `const` at the end of a member function is outside this procedure.**

```cpp
void topic_callback(const std_msgs::msg::String & msg) const
//                                                     ^^^^^ this one
```

This is **not a variable declaration but a function declaration.**
It means "this function does not change the object", and it is covered in chapter 6.

It is easy to tell apart. **A `const` after `)` is always the member function `const`.**
Every other `const` can be read with the procedure above.

## 1.4 Practice

Read the following six, right to left, following the procedure. Say them aloud or write them on paper.

```cpp
double v
const double & v
char * argv[]
const rclcpp::NodeOptions & options
std::shared_ptr<Node> node
const std::shared_ptr<Request> request
```

The answers are, in order:

1. `v` is a double
2. `v` is a reference to const double
3. `argv` is an array of "pointer to char" (the argument of `main` in exercise 01)
4. `options` is a reference to const `rclcpp::NodeOptions`
5. `node` is a `std::shared_ptr<Node>` (everything inside `<>` counts as one type)
6. `request` is a const `std::shared_ptr<Request>`. It is a **value**, not a reference (there is no `&`)

**If you tripped on number 6, that is the right way to trip.**
Without `&`, it is not a reference. Having `const` does not mean it is a reference.
The two are independent qualifiers.

## Try it yourself

**Read some declarations, then run them to check.**

```cpp
// basics01_practice.cpp
#include <iostream>
#include <vector>

int main()
{
  std::cout << "=== Practice: reading declarations ===\n\n";

  // Pattern 1: int x
  int x = 10;
  std::cout << "1. int x\n";
  std::cout << "   Reading: x is an int\n";
  std::cout << "   x = " << x << "\n\n";

  // Pattern 2: int * p
  int * p = &x;
  std::cout << "2. int * p = &x\n";
  std::cout << "   Reading: p is a pointer to int\n";
  std::cout << "   p (address) = " << p << "\n";
  std::cout << "   *p (pointed-to value) = " << *p << "\n";
  *p = 20;
  std::cout << "   After *p = 20; x also changed: " << x << "\n\n";

  // Pattern 3: const int * q
  int const_val = 100;
  const int * q = &const_val;
  std::cout << "3. const int * q\n";
  std::cout << "   Reading: q is a pointer to (const int)\n";
  std::cout << "   q (address) = " << q << "\n";
  std::cout << "   *q (pointed-to value, read only) = " << *q << "\n";
  std::cout << "   Test: this would be an error, so the line is removed\n";
  std::cout << "   // *q = 99;   <- not allowed\n\n";

  // Pattern 4: int * const r
  int val = 50;
  int * const r = &val;
  std::cout << "4. int * const r = &val\n";
  std::cout << "   Reading: r is a const (pointer to int)\n";
  std::cout << "   r (address) = " << r << "\n";
  std::cout << "   *r (pointed-to value) = " << *r << "\n";
  *r = 60;
  std::cout << "   After *r = 60; val also changed: " << val << "\n";
  std::cout << "   Test: r itself cannot be reseated\n";
  std::cout << "   // r = &x;   <- not allowed\n\n";

  // Pattern 5: const int * const s
  int val2 = 200;
  const int * const s = &val2;
  std::cout << "5. const int * const s = &val2\n";
  std::cout << "   Reading: s is a const (pointer to const int)\n";
  std::cout << "   s (address) = " << s << "\n";
  std::cout << "   *s (pointed-to value, read only) = " << *s << "\n";
  std::cout << "   Test: neither is allowed\n";
  std::cout << "   // *s = 99;   <- not allowed (the pointed-to value is const)\n";
  std::cout << "   // s = &x;    <- not allowed (the pointer is const)\n\n";

  // Check the left-right rule of const
  std::cout << "=== The left-right rule of const ===\n\n";
  std::cout << "const int * and int const * have the same meaning\n";
  std::cout << "  const int * a = &const_val;    // the pointed-to value is const\n";
  std::cout << "  int const * b = &const_val;    // the pointed-to value is const\n";
  std::cout << "  Result: in both, the pointed-to value cannot be changed\n\n";

  std::cout << "int * const means the pointer itself is const\n";
  std::cout << "  int * const c = &val;\n";
  std::cout << "  Result: the pointer itself cannot be reseated\n\n";

  // Array of pointers
  std::cout << "=== Applied: an array of pointers ===\n\n";
  int v1 = 1, v2 = 2, v3 = 3;
  int * array[] = {&v1, &v2, &v3};
  std::cout << "int * array[] = {&v1, &v2, &v3};\n";
  std::cout << "Reading: array is an array of (pointer to int)\n";
  for (int i = 0; i < 3; ++i) {
    std::cout << "  array[" << i << "] = " << array[i] << " -> *array[" << i << "] = " << *array[i] << "\n";
  }

  return 0;
}
```

**Predict: what does this program print? In particular, do the "allowed" and "not allowed" claims in each pattern really hold?**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic basics01_practice.cpp -o basics01_practice && ./basics01_practice
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/519Pj6oGd)

Output:

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: -->
```
=== Practice: reading declarations ===

1. int x
   Reading: x is an int
   x = 10

2. int * p = &x
   Reading: p is a pointer to int
   p (address) = 0x7ffffffc5a40
   *p (pointed-to value) = 10
   After *p = 20; x also changed: 20

3. const int * q
   Reading: q is a pointer to (const int)
   q (address) = 0x7ffffffc5a44
   *q (pointed-to value, read only) = 100
   Test: this would be an error, so the line is removed
   // *q = 99;   <- not allowed

4. int * const r = &val
   Reading: r is a const (pointer to int)
   r (address) = 0x7ffffffc5a48
   *r (pointed-to value) = 50
   After *r = 60; val also changed: 60
   Test: r itself cannot be reseated
   // r = &x;   <- not allowed

5. const int * const s = &val2
   Reading: s is a const (pointer to const int)
   s (address) = 0x7ffffffc5a4c
   *s (pointed-to value, read only) = 200
   Test: neither is allowed
   // *s = 99;   <- not allowed (the pointed-to value is const)
   // s = &x;    <- not allowed (the pointer is const)

=== The left-right rule of const ===

const int * and int const * have the same meaning
  const int * a = &const_val;    // the pointed-to value is const
  int const * b = &const_val;    // the pointed-to value is const
  Result: in both, the pointed-to value cannot be changed

int * const means the pointer itself is const
  int * const c = &val;
  Result: the pointer itself cannot be reseated

=== Applied: an array of pointers ===

int * array[] = {&v1, &v2, &v3};
Reading: array is an array of (pointer to int)
  array[0] = 0x7ffffffc5a50 -> *array[0] = 1
  array[1] = 0x7ffffffc5a54 -> *array[1] = 2
  array[2] = 0x7ffffffc5a58 -> *array[2] = 3
```

</details>

Check these three points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. Is the difference between `int * p` and `const int * q` clear?
2. Do `const int * q` and `int * const r` mean different things?
3. In `const int * const s`, are both restricted?

</details>

## Common pitfalls

**You are unsure where `const` attaches**

If you follow the right-to-left procedure, it becomes clear automatically. If you are unsure, trace the procedure again.

**Are `const int *` and `int const *` really the same?**

Yes, they are exactly the same. By the `const` rule, "it attaches to the thing on its left, or to the thing on its right if nothing is on the left", both attach to `int`.

**You still confuse "pointer to int" and "reference"**

This is normal. Chapter 3 covers references and chapter 4 covers pointers in detail. For now, think "a `*` means a pointer" and "a `&` means a reference".

## Matching exercise

After you read this chapter, work on the matching exercise in the drill.

- `cppb01_declarations` — Reading declarations

```bash
./drill run cppb01
```

If you get stuck, use `./drill hint cppb01`. From the exercise side, `./drill read cppb01` brings you back to this chapter.

## References

- [C++ chapter 4. References and const](../cpp/04_references_and_const.md) — the same topic in more depth (advanced)

---

Next chapter → [2. Scope and lifetime](02_scope_and_lifetime.md)
