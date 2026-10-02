# 14. Error handling

> **Goal of this chapter**: If you grep all 15 exercises of the drill, `throw` / `try` / `catch` do not appear. Zero times.
> So why does this chapter exist? Because "why C++ provides exceptions" and
> "why ROS 2 code avoids exceptions" are deeply connected.
> If you understand how exceptions work (stack unwinding, RAII),
> you can see why the drill values return codes (`ParameterNotDeclaredException` for an undeclared parameter) and
> timeout mechanisms.

## 14.1 Four ways to report errors

There are four ways to report an error in C++.

| Way | Example | When to use |
| --- | --- | --- |
| **Return code** | `int status = func(); if (status != 0) { ... }` | Crossing language or process boundaries (DDS, pipes, files) |
| **`std::optional<T>`** | `auto r = func(); if (!r) { ... }` | A two-valued difference: there is a value or not |
| **Exception** | `throw std::runtime_error(...);` | Propagating an error up the call stack. Inside one language. C++ only |
| **abort/assert** | `assert(x > 0);` | A violated assumption. For debugging. Removed in release builds |

The reason ROS 2 code uses return codes a lot is that **Python and C++ are mixed over DDS.**
A Python exception cannot be sent over DDS. Responses of a Service/Action are returned as return codes.

## 14.2 Exceptions — `throw`, `try`, `catch`

```cpp
#include <iostream>
#include <stdexcept>

int divide(int a, int b)
{
  if (b == 0) {
    throw std::invalid_argument("Cannot divide by 0");
  }
  return a / b;
}

int main()
{
  try {
    std::cout << "10 / 2 = " << divide(10, 2) << "\n";
    std::cout << "Trying 10 / 0...\n";
    divide(10, 0);
    std::cout << "Never reached\n";
  } catch (const std::invalid_argument & e) {
    std::cout << "Caught exception: " << e.what() << "\n";
  }

  std::cout << "Program continues\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/TEj1YKTb6)

```
10 / 2 = 5
Trying 10 / 0...
Caught exception: Cannot divide by 0
Program continues
```

`throw std::invalid_argument(...)` raises an exception.
`try { ... } catch (...) { ... }` catches it.

**Important: always catch with `const &`.** If you catch by value, slicing happens.

```cpp
catch (std::exception & e) { }          // reference ← do this
catch (std::exception e) { }             // value ← do not do this
```

### Slicing — a derived class shrinks to its base

What we did in chapter 3 applies to exceptions too.

```cpp
#include <iostream>
#include <stdexcept>
#include <string>

class MyError : public std::runtime_error
{
public:
  explicit MyError(const std::string & msg) : std::runtime_error(msg), code_(99) {}
  int code() const { return code_; }

private:
  int code_;
};

int main()
{
  try {
    throw MyError("Custom error");
  } catch (const std::exception & e) {
    std::cout << "Caught as base: " << e.what() << "\n";
    // e is a std::exception. It is not a MyError
    auto pe = dynamic_cast<const MyError *>(&e);
    if (pe) {
      std::cout << "Restored as MyError: " << pe->code() << "\n";
    }
  }

  std::cout << "\nCatching by value causes slicing:\n";
  try {
    throw MyError("Custom error");
  } catch (std::exception e) {  // ← catch by value
    std::cout << "Slicing: " << e.what() << "\n";
    // e is a copy of std::exception. The MyError part has been removed
  }
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/9E6rsxKxo)

```
Caught as base: Custom error
Restored as MyError: 99

Catching by value causes slicing:
Slicing: std::exception
```

In the second `try` block, `e` is a copy of `std::exception`.
The `code_` member of the derived class is cut off.

The compiler gives a warning: `warning: catching polymorphic type by value`.
When you see this warning, change it to `const &`.

## 14.3 Stack unwinding and RAII

The powerful point of C++ exceptions is that **the stack unwinds automatically and destructors are called.**

```cpp
#include <iostream>
#include <stdexcept>

class Resource
{
public:
  explicit Resource(const std::string & name) : name_(name)
  {
    std::cout << "  [" << name_ << "] acquired\n";
  }
  ~Resource() { std::cout << "  [" << name_ << "] released\n"; }

private:
  std::string name_;
};

void may_throw()
{
  std::cout << "  may_throw() running\n";
  throw std::runtime_error("error");
}

int main()
{
  try {
    Resource r1("Resource A");
    Resource r2("Resource B");
    may_throw();
    std::cout << "Never reached\n";
  } catch (const std::exception & e) {
    std::cout << "Caught exception: " << e.what() << "\n";
  }

  std::cout << "Resources were released for sure\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/GjjjEjjTY)

```
  [Resource A] acquired
  [Resource B] acquired
  may_throw() running
  [Resource B] released
  [Resource A] released
Caught exception: error
Resources were released for sure
```

When an exception is thrown in `may_throw()`, the scopes are left in reverse order.
The destructors are called in the order `r2`, then `r1`.

**This is why you can use exceptions safely in C++.** Combined with the RAII from chapter 2,
even if an exception is thrown, the guarantee "acquired resources are always released" holds.

## 14.4 Never throw an exception from a destructor

When a destructor is being called, an exception may already be propagating.
If another exception is thrown at that time, **`std::terminate` is called and the program is forcibly stopped.**

```cpp
#include <iostream>
#include <stdexcept>

class BadDtor
{
public:
  ~BadDtor() noexcept(false)
  {
    throw std::runtime_error("Exception from destructor");
  }
};

int main()
{
  try {
    BadDtor b;
    throw std::runtime_error("Another exception");
  } catch (const std::exception & e) {
    std::cout << "Caught: " << e.what() << "\n";
  }

  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/za8aaxzrn)

```
terminate called after throwing an instance of 'std::runtime_error'
  what():  Exception from destructor
```

The destructor of `BadDtor` tries to throw an exception, but a `std::runtime_error` is already propagating.
Throwing a second exception at that point is UB (undefined behavior), and `std::terminate` is called.

**Rule: put `noexcept` on destructors, and never throw an exception from them.**

```cpp
class Good
{
public:
  ~Good() noexcept  // ← promise not to throw an exception
  {
    // cleanup logic
    // do not throw an exception, or catch it and handle it
  }
};
```

## 14.5 `noexcept` — a promise about exceptions

`noexcept` is a promise that "this function does not throw an exception".
If you break it, `std::terminate` is called.

```cpp
#include <iostream>
#include <stdexcept>

void safe_operation() noexcept
{
  std::cout << "Safe operation\n";
}

void unsafe_operation() noexcept
{
  throw std::runtime_error("Throwing an exception");  // noexcept violation
}

int main()
{
  try {
    safe_operation();
    unsafe_operation();
  } catch (const std::exception & e) {
    std::cout << "Caught: " << e.what() << "\n";
  }

  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/d53Ev7z3Y)

```
Safe operation
terminate called after throwing an instance of 'std::runtime_error'
  what():  Throwing an exception
```

The compiler gives a warning.

```
warning: 'throw' will always call 'terminate' [-Wterminate]
```

`noexcept` is not just documentation. It is also a **basis for compiler optimization.**
Since it is guaranteed that "no exception is thrown", the cost of exception handling can be cut.

This is the reason chapter 5 says "a move constructor should be `noexcept`".
`std::vector` uses a `noexcept` move constructor when it reallocates, and copies otherwise.

## 14.6 The ROS 2 way of handling errors

The official [coding conventions](../ros2_coding_conventions.md) say this:

> Exceptions: allowed. Idiomatic in user-facing APIs. But **avoid them in destructors**.
> Consider avoiding them in APIs that wrap C.

In other words:

- **User-facing APIs (the main part of C++)**: exceptions are OK
- **Destructors**: exceptions are NG (`noexcept` is required)
- **APIs that throw toward the C side**: exceptions are NG (C cannot understand exceptions)

## 14.7 Exceptions you actually step on in the drill

I said that `throw` does not appear in the drill, but there are chances to `catch`.

### `rclcpp::exceptions::ParameterNotDeclaredException`

It appears in exercise 06.

```cpp
// Bad example
auto value = this->get_parameter("speed");  // throws if not declared
```

The correct way is to declare first and then use:

```cpp
this->declare_parameter("speed", 1.0);
auto value = this->get_parameter("speed");
```

This design follows the philosophy **"fail loudly rather than fail quietly".**
It prevents the trap where you think it runs on the default value, but in fact it was never set.

### `std::bad_weak_ptr` — a review of chapter 6

It is thrown when you call `shared_from_this()` on an object that was not created as a `shared_ptr`.

```cpp
MinimalPublisher node;  // on the stack
node.some_method_calling_shared_from_this();  // std::bad_weak_ptr
```

### `rclcpp::exceptions::InvalidNamespaceError`

It is thrown when the node name contains invalid characters.

## 14.8 Why ROS 2 avoids exceptions

In one sentence: **exceptions exist only inside one language. They cannot cross DDS.**

Responses of Services and Actions use return codes.

```cpp
// You can write this in C++
throw std::runtime_error("error");

// But the Result of an Action is a language-independent type
example_interfaces::action::Fibonacci_Result result;
result.sequence = {1, 1, 2, 3, 5};  // ← the return code goes here
```

The moment you talk to a Python node, **you cannot use exceptions.**
That is why ROS 2 has a strong return-code culture.

## 14.9 Other ways to handle errors

### `std::optional<T>` — there or not

```cpp
#include <iostream>
#include <optional>
#include <string>

std::optional<int> parse_int(const std::string & s)
{
  try {
    return std::stoi(s);
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

int main()
{
  auto a = parse_int("42");
  if (a) {
    std::cout << "Success: " << *a << "\n";
  } else {
    std::cout << "Failure\n";
  }

  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/z1crrhG1P)

```
Success: 42
```

`std::optional` has two values: "there is a value / there is not".
It cannot tell the details of the error (for what reason it failed).

It does not appear in the drill, so it is enough to keep it as knowledge.

### `assert()` and `static_assert`

```cpp
#include <iostream>
#include <cassert>

static_assert(sizeof(int) >= 4);  // compile-time check

void check_range(int x)
{
  assert(x >= 0 && x < 100);  // runtime check
  std::cout << "OK: " << x << "\n";
}

int main()
{
  check_range(50);
  // check_range(-1);  // if you remove the comment, assert is triggered
  return 0;
}
```

```
OK: 50
```

| Kind | When | Release build | Use |
| --- | --- | --- | --- |
| `static_assert` | Compile time | Stays | Checking template requirements |
| `assert()` | Runtime | Removed (`NDEBUG`) | Checking assumptions during development |

**`assert` is removed completely in release builds.** For checks that are required in production, do not use `assert`.
Use an exception or a return code.

## Try it yourself

**Implement all 4 patterns of error handling.**

```cpp
// error_handling.cpp
#include <cassert>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

// Pattern 1: return code
int divide_rc(int a, int b, int & result)
{
  if (b == 0) {
    return -1;  // error code
  }
  result = a / b;
  return 0;
}

// Pattern 2: std::optional
std::optional<int> divide_opt(int a, int b)
{
  if (b == 0) {
    return std::nullopt;
  }
  return a / b;
}

// Pattern 3: exception
int divide_exc(int a, int b)
{
  if (b == 0) {
    throw std::invalid_argument("divide by zero");
  }
  return a / b;
}

// Pattern 4: assert
int divide_assert(int a, int b)
{
  assert(b != 0);
  return a / b;
}

int main()
{
  std::cout << "-- 1. Return code\n";
  {
    int result = 0;
    if (divide_rc(10, 2, result) == 0) {
      std::cout << "Success: " << result << "\n";
    } else {
      std::cout << "Error\n";
    }
  }

  std::cout << "\n-- 2. std::optional\n";
  {
    auto r = divide_opt(10, 2);
    if (r) {
      std::cout << "Success: " << *r << "\n";
    } else {
      std::cout << "Error\n";
    }
  }

  std::cout << "\n-- 3. Exception\n";
  {
    try {
      int r = divide_exc(10, 2);
      std::cout << "Success: " << r << "\n";
    } catch (const std::invalid_argument & e) {
      std::cout << "Error: " << e.what() << "\n";
    }
  }

  std::cout << "\n-- 4. assert\n";
  {
    int r = divide_assert(10, 2);
    std::cout << "Success: " << r << "\n";
  }

  std::cout << "\n-- RAII: released even on exception\n";
  {
    struct Resource {
      explicit Resource(const char * n) : name(n) {
        std::cout << "  [" << n << "] acquired\n";
      }
      ~Resource() { std::cout << "  [" << name << "] released\n"; }
      const char * name;
    };

    try {
      Resource r1("A");
      Resource r2("B");
      std::cout << "  Throwing an error\n";
      throw std::runtime_error("problem");
    } catch (const std::exception & e) {
      std::cout << "Caught exception: " << e.what() << "\n";
    }
  }

  std::cout << "Program finished\n";
  return 0;
}
```

**Predict: Which message is printed in each pattern? In particular:**

1. Does the return-code version print "Error" when you call `divide_rc(10, 0, result)`?
2. Are both resources released for sure in the RAII block (in reverse order)?

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic error_handling.cpp -o error_handling && ./error_handling
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/hrYfWG3Kd)

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/nYec3YW9e)

After you check it, do these 2 experiments.

**① Catch an exception by value and see slicing:**

```cpp
class MyError : public std::runtime_error {
public:
  explicit MyError(int code) : std::runtime_error("my error"), code_(code) {}
  int code() const { return code_; }
private:
  int code_;
};

try {
  throw MyError(42);
} catch (std::exception e) {  // ← catch by value
  std::cout << "Slicing\n";
}
```

See the difference from catching by reference.

**② Call the error case of `divide_exc`:**

```cpp
divide_exc(10, 0);
```

(Run it in the state where it is commented out.)

## Common pitfalls

**`error: exception specification of overriding function is more lax`**

You are trying to override a destructor with `noexcept`. It does not pass unless the base is also `noexcept`.
In that case, make it explicit, as in `~Derived() noexcept = default;`.

**`error: catch (...) must be the last handler`**

There is another catch after `catch (...)`. `catch (...)` "catches everything",
so put it last.

**`exception specification of [...]  differs from [...]`**

The `noexcept` specification of a virtual function is different from the base.
A function that you `override` must follow the `noexcept` specification of the base.

**There is a memory leak (in Valgrind)**

Did you forget to `delete` memory that you `new`ed in a `catch` block?
Either `delete` it before you throw the exception, or use a smart pointer.

**`std::bad_weak_ptr` (a review of chapter 6)**

You are calling `shared_from_this()` on an object that was not created as a `shared_ptr`.
Always create the node with `std::make_shared<MinimalPublisher>()`.

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 06 | If you forget `declare_parameter()` before calling `get_parameter()`, you get `ParameterNotDeclaredException` |
| 07 | Declaring the same name twice with `declare_parameter()` is an error. Check first with `has_parameter()` |
| 09 | The `ResultCode` at action completion is a return code (not an exception) |
| Tests | When there is no service response, the timeout is handled with `future.wait_for()` (return code pattern) |

In exercise 06 you actually step on `ParameterNotDeclaredException`.
To catch both "forgot to set the parameter" and "typo in the name",
use `try { ... } catch (const rclcpp::exceptions::ParameterNotDeclaredException & e) { ... }`.

The test code of the drill contains exception handling.
But the main part that you implement basically does not.

## References

- [ROS 2 coding conventions](../ros2_coding_conventions.md) — when to use which for exceptions
- `cppreference`: [std::exception](https://en.cppreference.com/w/cpp/error/exception), [std::optional](https://en.cppreference.com/w/cpp/utility/optional), [assert](https://en.cppreference.com/w/c/error/assert)
- Herb Sutter and Andrei Alexandrescu, *C++ Coding Standards* — details of exception safety
- Details of `noexcept`: [cppreference noexcept](https://en.cppreference.com/w/cpp/language/noexcept_spec)

---

Previous → [13. Minimal concurrency](13_minimal_concurrency.md)
Next → [15. Checklist](15_checklist.md)
