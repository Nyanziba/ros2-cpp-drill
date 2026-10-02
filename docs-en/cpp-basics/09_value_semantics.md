# 9. Value semantics

> **Goal of this chapter**: **In C++, `b = a;` copies the value.** In Python and Java, assignment copies a reference, so if you change `a` later, `b` changes too. C++ is very different here. Once you understand this "value semantics", the behavior of C++ code becomes predictable.

## 9.1 Assignment is a copy — changing `a` after `b = a;` does not change `b`

```cpp
// value_assignment.cpp
#include <iostream>

class Value {
public:
  Value(int x) : x_(x) {}
  
  int x_;
};

int main() {
  Value a(10);
  Value b = a;     // copy
  
  a.x_ = 99;       // change a
  
  std::cout << "a.x_ = " << a.x_ << "\n";  // 99
  std::cout << "b.x_ = " << b.x_ << "\n";  // 10 (unchanged)
  
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/PKxnf3cMo)

<!-- measure: cmd="g++ -std=c++17 value_assignment.cpp -o value_assignment && ./value_assignment" -->
```
a.x_ = 99
b.x_ = 10
```

`a` and `b` are **two independent objects.** Changing `a` does not affect `b`.

## 9.2 Difference from Python / Java

In Python or Java:

```python
a = Value(10)
b = a            # assign a reference
a.x = 99
print(b.x)       # 99 (changed)
```

In C++:

```cpp
Value a(10);
Value b = a;     // copy the value
a.x_ = 99;
std::cout << b.x_ << "\n";  // 10 (unchanged)
```

**This difference is the biggest source of confusion.** People with Python/Java experience wonder, "why is the reference not passed?"

**In C++, assignment, function arguments, and return values all copy the value.** If you want a reference, use `&` explicitly.

## 9.3 Copy constructor and assignment operator

**There are two kinds of assignment:**

1. **`Value b = a;`**: create a new object `b` and initialize it
   → calls the **copy constructor**

2. **`b = a;`** (when `b` already exists): overwrite the existing object `b` with the value
   → calls the **copy assignment operator**

```cpp
// value_copy_constructor.cpp
#include <iostream>

class Value {
public:
  Value(int x) : x_(x) {
    std::cout << "Value(" << x << ")\n";
  }
  
  Value(const Value & other) : x_(other.x_) {
    std::cout << "Value copy constructor, x=" << x_ << "\n";
  }
  
  Value & operator=(const Value & other) {
    std::cout << "operator=, x=" << other.x_ << "\n";
    if (this != &other) {
      x_ = other.x_;
    }
    return *this;
  }
  
private:
  int x_;
};

int main() {
  std::cout << "=== Copy constructor: Value b = a; ===\n";
  Value a(10);
  Value b = a;   // copy constructor
  
  std::cout << "\n=== Copy assignment: b = a; ===\n";
  Value c(20);
  Value d(99);
  d = c;         // copy assignment
  
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/jx3eqPecj)

<!-- measure: cmd="g++ -std=c++17 value_copy_constructor.cpp -o value_copy_constructor && ./value_copy_constructor" -->
```
=== Copy constructor: Value b = a; ===
Value(10)
Value copy constructor, x=10

=== Copy assignment: b = a; ===
Value(20)
Value(99)
operator=, x=20
```

**The copy constructor is called only once.** The assignment operator, on the other hand, overwrites an existing object. So it cleans up the old members first and then assigns the new values (omitted in the example above).

## 9.4 Passing arguments — pass by value and pass by reference

**When you pass a value to a function, whether a copy happens depends on how you pass it.**

```cpp
void process_by_value(Data d) {
  // d is a copy
}

void process_by_const_ref(const Data & d) {
  // d is not copied
}
```

```cpp
// value_pass_by_argument.cpp
#include <iostream>

class Data {
public:
  Data(int x) : x_(x) {
    std::cout << "Data(" << x << ")\n";
  }
  
  Data(const Data & other) : x_(other.x_) {
    std::cout << "Data copy constructor\n";
  }
  
  ~Data() {
    std::cout << "~Data(" << x_ << ")\n";
  }
  
private:
  int x_;
};

void process_by_value(Data d) {
  // d was copied
}

void process_by_const_ref(const Data & d) {
  // d was NOT copied
}

int main() {
  std::cout << "=== pass by value ===\n";
  {
    Data a(10);
    process_by_value(a);
  }
  
  std::cout << "\n=== pass by const& ===\n";
  {
    Data b(20);
    process_by_const_ref(b);
  }
  
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/d659eTjea)

<!-- measure: cmd="g++ -std=c++17 value_pass_by_argument.cpp -o value_pass_by_argument && ./value_pass_by_argument" -->
```
=== pass by value ===
Data(10)
Data copy constructor
~Data(10)
~Data(10)

=== pass by const& ===
Data(20)
~Data(20)
```

**With pass by value, the copy constructor is called.** With `const Data & d`, no copy happens.

**If performance matters, use `const Data &`. For small data, pass by value is also fine.** The recent trend is: "the compiler optimizes, so choose whichever is easier to read."

## 9.5 Return value optimization — guaranteed in C++17

**When a function returns a value, C++17 constructs the return value directly.**

```cpp
// value_return.cpp
#include <iostream>

class Data {
public:
  Data(int x) : x_(x) {
    std::cout << "Data(" << x << ")\n";
  }
  
  Data(const Data & other) : x_(other.x_) {
    std::cout << "copy constructor\n";
  }
  
  Data(Data && other) noexcept : x_(other.x_) {
    std::cout << "move constructor\n";
  }
  
  ~Data() {
    std::cout << "~Data(" << x_ << ")\n";
  }
  
private:
  int x_;
};

Data create() {
  return Data(40);   // temporary object
}

int main() {
  std::cout << "=== Returning by value in C++17 ===\n";
  Data d = create();
  std::cout << "=== Done ===\n";
  return 0;
}
```

<!-- measure: cmd="g++ -std=c++17 value_return.cpp -o value_return && ./value_return" -->
```
=== Returning by value in C++17 ===
Data(40)
=== Done ===
~Data(40)
```

**The constructor runs only once.** No copy and no move happen. This is called **guaranteed return value optimization (guaranteed RVO)**.

Before C++17, copies or moves could happen. Since C++17, this is guaranteed.

## 9.6 Value semantics — summary table

| Operation | Does a copy happen? | Example |
| --- | --- | --- |
| `b = a;` | Yes | Pass by value; a new copy is made |
| Calling `void f(Data d)` | Yes | `d` inside the function is a separate copy |
| Calling `void f(const Data & d)` | No | `d` is a reference to the original object |
| Calling `void f(Data && d)` | No | `d` is a reference to a temporary object |
| `return a;` | No, since C++17 | Constructed directly by return value optimization |
| `push_back(d)` on a container | Yes | A copy is made inside the container |

## Try it yourself

This program checks the copy constructor, assignment, pass by value and by reference, and return value optimization in one place.

```cpp
// try.cpp
#include <iostream>

class Data {
public:
  Data(int x) : x_(x) {
    std::cout << "[constructor] x=" << x << "\n";
  }
  
  Data(const Data & other) : x_(other.x_) {
    std::cout << "[copy constructor] x=" << x_ << "\n";
  }
  
  Data & operator=(const Data & other) {
    std::cout << "[copy assignment] x=" << other.x_ << "\n";
    if (this != &other) {
      x_ = other.x_;
    }
    return *this;
  }
  
  Data(Data && other) noexcept : x_(other.x_) {
    std::cout << "[move constructor] x=" << x_ << "\n";
  }
  
  void print() const {
    std::cout << "value=" << x_ << "\n";
  }
  
private:
  int x_;
};

void show_copy_behavior() {
  std::cout << "== COPY SEMANTICS ==\n";
  std::cout << "Creating a(10):\n";
  Data a(10);
  
  std::cout << "\nCopy: Data b = a;\n";
  Data b = a;
  
  std::cout << "\nNow a and b are independent:\n";
  std::cout << "a: "; a.print();
  std::cout << "b: "; b.print();
}

void pass_by_value(Data d) {
  // d was copied when passed
}

void pass_by_const_ref(const Data & d) {
  // d was NOT copied
}

void show_passing() {
  std::cout << "\n== PASSING BY VALUE vs CONST& ==\n";
  
  std::cout << "Pass by value - creates copy:\n";
  Data a(20);
  pass_by_value(a);
  
  std::cout << "\nPass by const& - no copy:\n";
  Data b(30);
  pass_by_const_ref(b);
}

Data create_data() {
  return Data(40);
}

void show_returning() {
  std::cout << "\n== RETURNING BY VALUE (C++17) ==\n";
  std::cout << "Call create_data() and assign:\n";
  Data d = create_data();
  std::cout << "Only constructor, no copy or move!\n";
}

int main() {
  show_copy_behavior();
  show_passing();
  show_returning();
  
  return 0;
}
```

**Predict: Is the copy constructor called several times? Is the number of copies different between pass by value and pass by reference? Does a copy happen on return?**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/f6n1qxYr7)

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/4YfhvdhfK)

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: -->
```
try.cpp: In function ‘void pass_by_value(Data)’:
try.cpp:47:25: warning: unused parameter ‘d’ [-Wunused-parameter]
   47 | void pass_by_value(Data d) {
      |                    ~~~~~^
try.cpp: In function ‘void pass_by_const_ref(const Data&)’:
try.cpp:51:37: warning: unused parameter ‘d’ [-Wunused-parameter]
   51 | void pass_by_const_ref(const Data & d) {
      |                        ~~~~~~~~~~~~~^
try.cpp: In function ‘void show_returning()’:
try.cpp:74:8: warning: variable ‘d’ set but not used [-Wunused-but-set-variable]
   74 |   Data d = create_data();
      |        ^
== COPY SEMANTICS ==
Creating a(10):
[constructor] x=10

Copy: Data b = a;
[copy constructor] x=10

Now a and b are independent:
a: value=10
b: value=10

== PASSING BY VALUE vs CONST& ==
Pass by value - creates copy:
[constructor] x=20
[copy constructor] x=20

Pass by const& - no copy:
[constructor] x=30

== RETURNING BY VALUE (C++17) ==
Call create_data() and assign:
[constructor] x=40
Only constructor, no copy or move!
```

</details>

## Common pitfalls

**"I thought a reference was passed, but the value changed"**
In C++, assignment copies. Unless you use a reference (`&`), the value is copied. If you know Python, be careful about this difference.

**Copy constructor or assignment operator: which one is called**
- `Value b = a;` → copy constructor
- `b = a;` (`b` already exists) → assignment operator

It is the same `=`, but the meaning is different.

**You do not need move semantics yet**
Since C++17, return value optimization is guaranteed, so there are fewer cases where you need to use a move explicitly.
For details, see the [C++ track](../cpp/README.md).

**Pass big objects with `const &`**
Pass big data such as containers by `const Data &` to avoid copies.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `cppb09_value_semantics` — count the copies

```bash
./drill run cppb09
```

If you get stuck, run `./drill hint cppb09`. From the exercise side, `./drill read cppb09` brings you back to this chapter.

## References

- `cppreference`: [Copy constructor](https://en.cppreference.com/w/cpp/language/copy_constructor) and [Copy assignment operator](https://en.cppreference.com/w/cpp/language/copy_assignment)
- [Guaranteed Copy Elision](https://en.cppreference.com/w/cpp/language/copy_elision) (C++17)
- In ROS 2 materials, value semantics appears in places such as [Value and Reference](https://docs.ros.org/en/rolling/Concepts/Basic/About-Different-DDS-QoS-Policies.html)

---

Previous chapter → [8. Other qualifiers](08_other_qualifiers.md)
Next chapter → [10. Headers and project layout](10_headers_and_project_layout.md)
