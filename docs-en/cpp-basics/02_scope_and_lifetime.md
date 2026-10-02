# 2. Scope and lifetime

> **Goal of this chapter**: When you leave a block that starts with `{`, the variables declared there disappear. If you do not know when they disappear and in what order, then from chapter 3 on you will hit the problem of "why can I no longer use this reference or pointer?"
> This chapter is the basis for chapters 3 to 5. Do not read it lightly. Check the constructor and destructor output carefully.

## 2.1 Block scope

In C++, everything from `{` to `}` is one block. A variable declared inside a block **no longer exists** once you leave the block.

```cpp
{
  int x = 10;
  std::cout << x << "\n";  // OK
}
std::cout << x << "\n";    // Error: x does not exist
```

This is called **block scope.**

## 2.2 Automatic storage duration

A variable declared inside a block is managed automatically.

- **Creation**: at the place where the variable is declared (the constructor is called)
- **Destruction**: when you leave the block (the destructor is called)

This mechanism is **automatic storage.**

## 2.3 Destruction happens in reverse order

If there are several variables in a block, **they are destroyed in reverse order.**

```cpp
{
  Logger a("a");     // constructed 1st
  Logger b("b");     // constructed 2nd
  Logger c("c");     // constructed 3rd
}
// destroyed: c → b → a (reverse order)
```

This rule comes from the nature of a stack (last in, first out).

## 2.4 Nested blocks

A variable declared in an inner block is destroyed when the inner block closes.

```cpp
{
  Logger outer("outer");    // constructed 1st
  {
    Logger inner("inner");  // constructed 2nd
  }
  // inner is destroyed
  // outer is still alive
}
// outer is destroyed
```

## 2.5 Destruction by return

When you `return` from a function, the variables are destroyed just before leaving the scope.

```cpp
void example() {
  Logger local("local");
  // ... work ...
  return;  // local is destroyed
}
```

Variables are destroyed on every exit path (`return`, an exception, or the end of the scope).

## 2.6 Shadowing

If you declare a variable with the same name as an outer one in an inner scope, the inner one hides the outer one.

```cpp
{
  int x = 1;
  {
    int x = 2;  // the inner x hides the outer x
    std::cout << x;  // 2
  }
  std::cout << x;  // 1
}
```

**The compiler warning `-Wshadow` can detect this.**

## 2.7 The basics of RAII

In C++, you manage resources (memory, files, locks) automatically by using the creation and destruction of objects.
This is the principle of **RAII (Resource Acquisition Is Initialization).**

The automatic destruction you learned in this chapter is what supports RAII.

## Try it yourself

**We make block scope and the order of destruction visible with constructors and destructors.**

```cpp
// basics02_practice.cpp
#include <iostream>

struct Logger
{
  const char * name;

  Logger(const char * n) : name(n)
  {
    std::cout << "[ctor] " << name << " created\n";
  }

  ~Logger()
  {
    std::cout << "[dtor] " << name << " destroyed\n";
  }
};

int main()
{
  std::cout << "=== Scope and lifetime ===\n\n";

  std::cout << "--- 1. Block scope ---\n";
  {
    Logger a("a");
    std::cout << "a is alive in inner block\n";
  }
  std::cout << "a is now dead (exited block)\n\n";

  std::cout << "--- 2. Order of creation and destruction (reverse) ---\n";
  {
    Logger x("x");
    {
      Logger y("y");
      {
        Logger z("z");
        std::cout << "x, y, z all alive here\n";
      }
      std::cout << "z was destroyed (reverse order: z -> y -> x)\n";
    }
    std::cout << "y was destroyed\n";
  }
  std::cout << "x was destroyed\n\n";

  std::cout << "--- 3. Destroyed by return ---\n";
  {
    Logger before("before_return");
    std::cout << "about to return...\n";
  }
  std::cout << "before was destroyed when block ended\n\n";

  std::cout << "--- 4. Shadowing (the inner variable hides the outer one) ---\n";
  {
    int x = 1;
    std::cout << "outer x = " << x << "\n";
    {
      int x = 2;
      std::cout << "inner x = " << x << "\n";
    }
    std::cout << "back to outer x = " << x << "\n";
  }
  std::cout << "\n";

  std::cout << "--- 5. Reverse-order destruction of several variables ---\n";
  {
    Logger first("first");
    Logger second("second");
    Logger third("third");
    std::cout << "all three alive here\n";
  }
  std::cout << "destroyed in reverse: third, second, first\n\n";

  std::cout << "--- 6. Function scope ---\n";
  auto test_function = []() {
    Logger func_local("func_local");
    std::cout << "inside function\n";
  };
  std::cout << "calling function:\n";
  test_function();
  std::cout << "function returned, func_local was destroyed\n";

  return 0;
}
```

**Predict: follow the order of the constructor and destructor calls, and check in what order the objects are created and then destroyed in reverse.**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic basics02_practice.cpp -o basics02_practice && ./basics02_practice
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/K8haY9x7E)

Output:

<details markdown="1"><summary>Answer (actual output)</summary>

```
=== Scope and lifetime ===

--- 1. Block scope ---
[ctor] a created
a is alive in inner block
[dtor] a destroyed
a is now dead (exited block)

--- 2. Order of creation and destruction (reverse) ---
[ctor] x created
[ctor] y created
[ctor] z created
x, y, z all alive here
[dtor] z destroyed
z was destroyed (reverse order: z -> y -> x)
[dtor] y destroyed
y was destroyed
[dtor] x destroyed
x was destroyed

--- 3. Destroyed by return ---
[ctor] before_return created
about to return...
[dtor] before_return destroyed
before was destroyed when block ended

--- 4. Shadowing (the inner variable hides the outer one) ---
outer x = 1
inner x = 2
back to outer x = 1

--- 5. Reverse-order destruction of several variables ---
[ctor] first created
[ctor] second created
[ctor] third created
all three alive here
[dtor] third destroyed
[dtor] second destroyed
[dtor] first destroyed
destroyed in reverse: third, second, first

--- 6. Function scope ---
calling function:
[ctor] func_local created
inside function
[dtor] func_local destroyed
function returned, func_local was destroyed
```

</details>

Check these three points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. Is the **destruction in reverse order**? Do the `[ctor]` and `[dtor]` timings form pairs?
2. In the **nested blocks**, are the objects destroyed in the order inner → outer?
3. With **shadowing**, do the inner `x = 2` and the outer `x = 1` both work correctly?

</details>

Run this code yourself, then go on to chapters 3, 4, and 5.
The mystery of "why can I no longer use this reference or pointer?" is solved here.

## Common pitfalls

**"Variables disappear when you leave the block" does not click yet**

"Disappear" means the memory area of that variable becomes reusable.
The destructor is called, and the cleanup is done.
This is an important idea that leads to the "dangling pointer" in chapter 5.

**Why is the destruction order reversed?**

Variables in a block are placed on the stack. A stack is last in, first out, so the item added last is taken out first.
This is the foundation of memory management in C and C++.

**How to avoid shadowing**

Use different names, or have the compiler warn you with the `-Wshadow` flag.
The tests in the drill have `-Wall -Wextra` enabled, so get into the habit of avoiding shadowing.

## Matching exercise

After you read this chapter, work on the matching exercise in the drill.

- `cppb02_scope` — Scope and lifetime

```bash
./drill run cppb02
```

If you get stuck, use `./drill hint cppb02`. From the exercise side, `./drill read cppb02` brings you back to this chapter.

## References

- RAII pattern — Wikipedia
- [C++ chapter 2. Constructors and destructors](../cpp/02_classes_and_initialization.md)

---

Previous chapter ← [1. Reading declarations](01_reading_declarations.md)
Next chapter → [3. References](03_references.md)
