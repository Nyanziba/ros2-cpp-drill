# 3. References

> **Goal of this chapter**: When you see the declaration `int & x`, you wonder "what is this? How is it different from a pointer?"
> To begin with, the symbol `&` is used with completely different meanings when it qualifies a type and when it acts inside an expression.
> This chapter clears up this confusion, where the meaning changes with the position.
> References are the point of comparison in chapters 4 to 5, 6, and 8, so do not leave them vague here.

## 3.1 A reference is an alias

`int & ref = x;` means "ref is an alias of x". Think of it as having two names for the same object.

```cpp
int x = 10;
int & ref = x;

ref = 20;
std::cout << x;  // 20 (we changed it through ref, but x changed too)
```

If you change the value through a reference, the original object changes. This is because it is **an alias, not a copy.**

## 3.2 Important: the two uses of `&`

**The `&` in `int & x` (a declaration) and the `&` in `&x` (an expression) are the same symbol, but they mean completely different things.**

| Context | Meaning | Example |
| --- | --- | --- |
| Type qualifier (declaration) | Reference (alias) | `int & ref = x;` |
| Operator in an expression | Get the address (make a pointer) | `int * p = &x;` |

**You tell them apart by position.** If it qualifies a type, it is a reference. If it is inside an expression, it takes an address.

## 3.3 A reference must be initialized

A reference must be initialized when it is declared. An uninitialized reference cannot exist.

```cpp
int & r;        // Error: no initializer
int & r = x;    // OK
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// basics03_uninit.cpp
#include <iostream>

int main()
{
  int x = 10;
  std::cout << x << "\n";

  int & r;        // Error: no initializer
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic basics03_uninit.cpp -o basics03_uninit
```

</details>

Compiler message:
```
error: ‘r’ declared as reference but not initialized
```

## 3.4 A reference cannot be reseated

After you create a reference, you cannot change it to refer to another object.

```cpp
int x = 1, y = 2;
int & ref = x;
ref = y;  // does ref now refer to y?
```

**No. `ref = y;` does not mean "reseat ref to y". It means "assign the value of y to x through ref".**

```cpp
std::cout << x;    // 2 (the value of y is stored in it)
std::cout << ref;  // 2 (ref is still an alias of x)
```

ref is always an alias of x.

## 3.5 A reference cannot be null

A reference must always refer to some object. It cannot be null.

```cpp
int & ref = nullptr;  // Error
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// basics03_null.cpp
int main()
{
  int & ref = nullptr;  // Error
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic basics03_null.cpp -o basics03_null
```

</details>

Compiler message:
```
error: invalid initialization of non-const reference of type ‘int&’ from an rvalue of type ‘std::nullptr_t’
```

If you want to express the state "nothing exists", use a pointer (chapter 4).

## 3.6 Pass by reference

If you pass to a function by reference, the function can change the caller's variable directly.

```cpp
void modify(int & x) {
  x = 99;
}

int a = 1;
modify(a);
std::cout << a;  // 99 (a was changed inside modify)
```

This is the big difference from **pass by value** (a copy).

| Pass by value | Pass by reference |
| --- | --- |
| `void f(int x)` | `void f(int & x)` |
| Receives a copy | Receives the original object |
| Changes inside the function do not affect the caller | Changes inside the function affect the caller |
| The caller writes `f(a)` | The caller writes `f(a)` (the same) |

## 3.7 const reference (a read-only alias)

`const int & ref` is a reference through which you cannot change the target.

```cpp
int x = 100;
const int & const_ref = x;

std::cout << const_ref;  // You can read it
const_ref = 200;         // Error: it is const, so you cannot write
```

You can also receive a temporary value (a literal) with a `const` reference.

```cpp
const int & temp = 42;
std::cout << temp;  // 42
```

A non-const reference cannot receive a temporary value.

```cpp
int & non_const_ref = 42;  // Error
```

### Why can only a const reference receive a temporary value?

A reference is "an alias of an object that already exists". A temporary value such as the literal `42` has no place in memory, so there is nothing to give a name to.

A `const` reference is still allowed because the compiler creates a temporary object behind the scenes and binds to it.

```cpp
// An image of what the compiler does internally for const int & temp = 42;
int hidden_temporary = 42;             // create a temporary object
const int & temp = hidden_temporary;   // bind the reference to it
// The lifetime of the temporary object is extended to the lifetime of temp (lifetime extension rule)
```

It is const, so nobody rewrites it, and it works without contradiction as a "read-only alias".

A non-const reference, on the other hand, means "rewrite the target". If binding to a temporary value were allowed, it would breed bugs like this.

```cpp
void increment(int & value) { value++; }

double distance = 3.14;
increment(distance);  // If this were allowed: a temporary int is made from distance,
                      // and only that "copy" is incremented.
                      // distance does not change → this differs from what the caller intended
```

The caller expects "`distance` increases", but in fact only a discarded temporary object changes. To prevent this accident, C++ forbids a non-const reference from binding to a temporary value.

Thanks to this rule, if you make a parameter `const T &`, you can pass both variables and literals.

```cpp
void print(const std::string & message);

std::string greeting = "hello";
print(greeting);   // OK: a variable
print("world");    // OK: a temporary std::string is made from the literal and bound
```

"Avoid copies, and accept both variables and temporary values" — this is why making a parameter `const T &` is the standard in C++.

| | Can it bind a temporary value? | Reason |
| --- | --- | --- |
| `T &` | ✗ | Even if you rewrite it, it is just discarded, which breeds bugs |
| `const T &` | ✓ | If you only read it, it is safe to create a temporary object and extend its lifetime |

!!! note "Supplement for C++11 and later"
    There is a dedicated reference for "receiving a temporary value and taking its contents": the rvalue reference `T &&` (the basis of move semantics). It is covered in a later chapter.

## 3.8 Dangling references

If you return a reference from a function, the target may be destroyed after the function exits.

```cpp
const int & bad_function(int x) {
  int local = x + 1;
  return local;  // local is destroyed when the function exits
}

const int & ref = bad_function(5);  // ref is invalid (a dangling reference)
std::cout << ref;                   // undefined behavior
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// basics03_dangling.cpp
#include <iostream>

const int & bad_function(int x) {
  int local = x + 1;
  return local;  // local is destroyed when the function exits
}

int main()
{
  const int & ref = bad_function(5);  // ref is invalid (a dangling reference)
  std::cout << ref;                   // undefined behavior
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -c basics03_dangling.cpp
```

</details>

The compiler should give a warning:

```
warning: reference to local variable ‘local’ returned [-Wreturn-local-addr]
```

If you return a reference, return a reference that was passed in, or make sure the object still exists after the function exits.

## Try it yourself

**We check how references behave with nine patterns.**

```cpp
// basics03_practice.cpp
#include <iostream>

int main()
{
  std::cout << "=== References ===\n\n";

  std::cout << "--- 1. A reference is an alias ---\n";
  int x = 10;
  int & ref = x;
  std::cout << "int x = 10;\n";
  std::cout << "int & ref = x;\n";
  std::cout << "x = " << x << "\n";
  std::cout << "ref = " << ref << "\n";
  std::cout << "&x = " << &x << "\n";
  std::cout << "&ref = " << &ref << " (the same address!)\n\n";

  std::cout << "--- 2. Change through a reference ---\n";
  ref = 20;
  std::cout << "After ref = 20; x also changed\n";
  std::cout << "x = " << x << "\n";
  std::cout << "ref = " << ref << "\n\n";

  std::cout << "--- 3. A reference must be initialized ---\n";
  std::cout << "Test: int & r; (no initializer) is an error\n";
  std::cout << "error: ‘r’ declared as reference but not initialized\n";
  std::cout << "(please try it yourself)\n\n";

  std::cout << "--- 4. A reference cannot be reseated ---\n";
  int y = 30;
  int & ref2 = x;
  std::cout << "int y = 30;\n";
  std::cout << "int & ref2 = x;  // ref2 refers to x\n";
  std::cout << "ref2 = y;  // this assigns the 'value' of y to x!\n";
  ref2 = y;
  std::cout << "Result: x = " << x << " (the value 30 of y was stored)\n";
  std::cout << "        y = " << y << " (unchanged)\n";
  std::cout << "ref2 is still an alias of x\n\n";

  std::cout << "--- 5. A reference cannot be null ---\n";
  std::cout << "A reference always refers to some object\n";
  std::cout << "int & null_ref = nullptr;  // this is an error\n";
  std::cout << "error: invalid initialization of non-const reference of type ‘int&’ from an rvalue of type ‘std::nullptr_t’\n\n";

  std::cout << "--- 6. The difference between pass by value and pass by reference ---\n";
  auto modify_by_value = [](int n) {
    n = 999;
  };
  auto modify_by_ref = [](int & n) {
    n = 999;
  };

  int a = 1, b = 1;
  std::cout << "before modify_by_value: a = " << a << "\n";
  modify_by_value(a);
  std::cout << "after modify_by_value:  a = " << a << " (unchanged)\n\n";

  std::cout << "before modify_by_ref:  b = " << b << "\n";
  modify_by_ref(b);
  std::cout << "after modify_by_ref:   b = " << b << " (changed!)\n\n";

  std::cout << "--- 7. const reference (a read-only alias) ---\n";
  int original = 100;
  const int & const_ref = original;
  std::cout << "const int & const_ref = original;\n";
  std::cout << "const_ref = " << const_ref << "\n";
  std::cout << "const_ref = 999;  // error (const)\n";
  std::cout << "original = 200;  // OK (directly, not through const_ref)\n";
  original = 200;
  std::cout << "const_ref = " << const_ref << " (it shows the change of original)\n\n";

  std::cout << "--- 8. const reference to a temporary value ---\n";
  const int & temp_ref = 42;
  std::cout << "const int & temp_ref = 42;\n";
  std::cout << "temp_ref = " << temp_ref << "\n";
  std::cout << "The temporary value stays alive with a const reference\n";
  std::cout << "(with a non-const reference it is an error)\n\n";

  std::cout << "--- 9. Returning a reference from a function (the danger of dangling) ---\n";
  std::cout << "Test: the following code creates a dangling reference\n";
  std::cout << "const int & dangerous_ref = [](const int & x) -> const int & {\n";
  std::cout << "  int local = x + 1;\n";
  std::cout << "  return local;  // local is destroyed when the function exits!\n";
  std::cout << "} (original);\n";
  std::cout << "The compiler should warn you\n";
  std::cout << "warning: reference to local variable ‘local’ returned\n";

  return 0;
}
```

**Predict: check whether the reference and the original object have the same memory address, and whether a change of value shows up on both.**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic basics03_practice.cpp -o basics03_practice && ./basics03_practice
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/h4xKTf973)

Output:

<details markdown="1"><summary>Answer (actual output)</summary>

```
=== References ===

--- 1. A reference is an alias ---
int x = 10;
int & ref = x;
x = 10
ref = 10
&x = 0x7ffffffc5a10
&ref = 0x7ffffffc5a10 (the same address!)

--- 2. Change through a reference ---
After ref = 20; x also changed
x = 20
ref = 20

--- 3. A reference must be initialized ---
Test: int & r; (no initializer) is an error
error: ‘r’ declared as reference but not initialized
(please try it yourself)

--- 4. A reference cannot be reseated ---
int y = 30;
int & ref2 = x;  // ref2 refers to x
ref2 = y;  // this assigns the 'value' of y to x!
Result: x = 30 (the value 30 of y was stored)
        y = 30 (unchanged)
ref2 is still an alias of x

--- 5. A reference cannot be null ---
A reference always refers to some object
int & null_ref = nullptr;  // this is an error
error: invalid initialization of non-const reference of type ‘int&’ from an rvalue of type ‘std::nullptr_t’

--- 6. The difference between pass by value and pass by reference ---
before modify_by_value: a = 1
after modify_by_value:  a = 1 (unchanged)

before modify_by_ref:  b = 1
after modify_by_ref:   b = 999 (changed!)

--- 7. const reference (a read-only alias) ---
const int & const_ref = original;
const_ref = 100
const_ref = 999;  // error (const)
original = 200;  // OK (directly, not through const_ref)
const_ref = 200 (it shows the change of original)

--- 8. const reference to a temporary value ---
const int & temp_ref = 42;
temp_ref = 42
The temporary value stays alive with a const reference
(with a non-const reference it is an error)

--- 9. Returning a reference from a function (the danger of dangling) ---
Test: the following code creates a dangling reference
const int & dangerous_ref = [](const int & x) -> const int & {
  int local = x + 1;
  return local;  // local is destroyed when the function exits!
} (original);
The compiler should warn you
warning: reference to local variable ‘local’ returned
```

</details>

Check these three points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. **Aliasing**: do `&x` and `&ref` show the same address?
2. **The reseating misunderstanding**: does the output show that `ref = y;` does not make ref refer to y, but assigns the value to x?
3. **Pass by value vs pass by reference**: is `a` unchanged with `modify_by_value`, and is `b` changed with `modify_by_ref`?

</details>

## Common pitfalls

**"I do not see the difference between a reference and pass by value"**

Pass by reference hands the caller's variable itself to the function. A change inside the function shows up in the original variable.
Pass by value hands over a copy, so a change inside the function does not show up in the original variable.
The output of number 6 makes it clear at a glance.

**The explanation "a reference cannot be reseated" feels vague**

`int & ref = x; ref = y;` is split into two steps of meaning: not "reseat ref to y", but "assign the value of y to x through ref".
`ref` is always an alias of `x`. You **cannot** reseat it.

**"What is a const reference for?"**

It is a read-only borrow. It is used very often for function parameters.
It lets you say "borrow a large object read-only, without copying it".
This is a preview of the detailed explanation in chapter 8.

## Matching exercise

After you read this chapter, work on the matching exercise in the drill.

- `cppb03_reference` — Receive by reference

```bash
./drill run cppb03
```

If you get stuck, use `./drill hint cppb03`. From the exercise side, `./drill read cppb03` brings you back to this chapter.

## References

- [C++ chapter 4. References and const](../cpp/04_references_and_const.md) — a more detailed explanation, and a comparison with pointers
- [C++ chapter 5. Move semantics](../cpp/05_move_and_ownership.md) — rvalue references (`&&`)

---

Previous chapter ← [2. Scope and lifetime](02_scope_and_lifetime.md)
Next chapter → [4. Pointers 1](04_pointers_1.md)
