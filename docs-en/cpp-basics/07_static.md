# 7. `static`

> **Goal of this chapter**: Like pointers, **the same keyword `static` is used with 3 completely different meanings.** The meaning changes with the place where `static` appears. If you can tell the 3 apart ("not visible outside the scope", "visible only inside the file", "shared between instances of the class"), `static` becomes predictable.

## 7.1 `static` in a function — initialized once, lives across calls

**A `static` variable in a function is initialized only on the first call, and after that it lives on and keeps its value.**

```cpp
#include <iostream>

int get_request_id()
{
  static int counter = 1000;
  return ++counter;
}

int main()
{
  std::cout << get_request_id() << "\n";  // 1001
  std::cout << get_request_id() << "\n";  // 1002
  std::cout << get_request_id() << "\n";  // 1003
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/oqaK3d54x)

```
1001
1002
1003
```

**Initialization happens only the first time.** On later calls the initialization is skipped, and the function starts from the previous value.

This is used to keep **function-local state**: counters, ID generation, caches, and so on.
A `static` function-local variable has a **narrower scope and is safer** than a global variable.

**Since C++11, the initialization is thread-safe.**
Even if several threads call the same function, the initialization happens only once.

## 7.2 `static` in a file — turn external linkage into internal linkage

**A variable declared `static` at the top level (global scope) of a file is visible only inside that translation unit (`.cpp`).**

```cpp
// file1.cpp
static int counter = 0;

int get_counter_1()
{
  return ++counter;
}
```

```cpp
// file2.cpp
static int counter = 0;  // a different variable from the counter in file1

int get_counter_2()
{
  return ++counter;
}
```

If you write a variable with the same name in several files without `static`, you get a **link error**.

```cpp
// file1.cpp
int counter = 0;  // external linkage
```

```cpp
// file2.cpp
int counter = 0;  // duplicates file1 → multiple definition error
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// file1.cpp
int counter = 0;  // external linkage
```

```cpp
// file2.cpp
int counter = 0;  // duplicates file1 → multiple definition error
```

```cpp
// main.cpp
int main()
{
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -c file1.cpp file2.cpp main.cpp && g++ file1.o file2.o main.o -o app
```

</details>

```
/usr/bin/ld: file2.o:(.bss+0x0): multiple definition of `counter'; file1.o:(.bss+0x0): first defined here
collect2: error: ld returned 1 exit status
```

**If you declare it `static` ("used only in this file"), the names do not collide at link time.**

You can use the same name in several files. However, each file has its own independent copy.
If you want to synchronize the values, you must communicate through functions.

## 7.3 `static` in a class — state shared between instances

**A `static` member is a single object shared by all instances of the class.**

```cpp
#include <iostream>

class Counter
{
public:
  Counter() { ++count_; }
  
  static int get_total() { return count_; }
  
private:
  static int count_;
};

int Counter::count_ = 0;  // A definition is required

int main()
{
  std::cout << Counter::get_total() << "\n";  // 0
  
  Counter c1;
  std::cout << Counter::get_total() << "\n";  // 1
  
  Counter c2;
  std::cout << Counter::get_total() << "\n";  // 2
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/xqe4aseeK)

```
0
1
2
```

**A `static` member needs a definition.** A declaration alone cannot be found at link time.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// static_undefined.cpp
#include <iostream>

class Counter
{
public:
  Counter() { ++count_; }
  
  static int get_total() { return count_; }
  
private:
  static int count_;
};

// int Counter::count_ = 0;  // A definition is required  <- leave this line out

int main()
{
  std::cout << Counter::get_total() << "\n";  // 0
  
  Counter c1;
  std::cout << Counter::get_total() << "\n";  // 1
  
  Counter c2;
  std::cout << Counter::get_total() << "\n";  // 2
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -c static_undefined.cpp && g++ static_undefined.o -o static_undefined
```

</details>

```
/usr/bin/ld: static_undefined.o: warning: relocation against `_ZN7Counter6count_E' in read-only section `.text._ZN7Counter9get_totalEv[_ZN7Counter9get_totalEv]'
/usr/bin/ld: static_undefined.o: in function `Counter::Counter()':
static_undefined.cpp:(.text._ZN7CounterC2Ev[_ZN7CounterC5Ev]+0xe): undefined reference to `Counter::count_'
/usr/bin/ld: static_undefined.cpp:(.text._ZN7CounterC2Ev[_ZN7CounterC5Ev]+0x17): undefined reference to `Counter::count_'
/usr/bin/ld: static_undefined.o: in function `Counter::get_total()':
static_undefined.cpp:(.text._ZN7Counter9get_totalEv[_ZN7Counter9get_totalEv]+0xa): undefined reference to `Counter::count_'
/usr/bin/ld: warning: creating DT_TEXTREL in a PIE
collect2: error: ld returned 1 exit status
```

**Since C++17, `inline static` removes the need for a separate definition.**

```cpp
class Counter
{
private:
  inline static int count_ = 0;  // This is complete
};
```

### `static` member functions — there is no `this`

When you add `static` to a member function, it no longer receives `this`.

```cpp
class Logger
{
public:
  static void print_stats()  // there is no this
  {
    std::cout << "instances: " << instance_count_ << "\n";
  }
  
private:
  inline static int instance_count_ = 0;
};

// Call it with the class name
Logger::print_stats();
```

**A `static` member function can access only `static` members.**
`instance_count_` is `static`, so it is OK. It cannot use members that belong to a particular instance.

## 7.4 Summary of the 3 meanings of `static`

| Position | Example | Lifetime | Scope | Initialization |
| --- | --- | --- | --- | --- |
| In a function | `static int x = 0;` | The whole program | Only inside that function | Once, at the first call |
| File top level | `static int x = 0;` | The whole program | Only inside that file | At program start |
| Class member | `static int x_;` | The whole program | Everywhere, as `ClassName::x_` | At program start |

**Tips for telling which `static` it is:**

- Is it defined outside or inside `{}`? → If outside, it is a "file static" or a "class static"
- Is it inside a `class`? → If inside, it is a "class static"
- Is it inside a function that is outside a `class`? → If inside a function, it is a "function-local static"

## Try it yourself

Check the 3 kinds of `static`: in a function, in a file, and in a class.

```cpp
// static_all.cpp
#include <iostream>

class Logger
{
public:
  Logger(const char * name) : name_(name) { ++instance_count_; }
  
  void log(const char * msg) const
  {
    std::cout << name_ << ": " << msg << "\n";
  }
  
  static int instance_count() { return instance_count_; }
  
  static void print_stats()
  {
    std::cout << "Total instances created: " << instance_count_ << "\n";
  }
  
private:
  const char * name_;
  inline static int instance_count_ = 0;
};

int get_request_id()
{
  static int id = 1000;
  return ++id;
}

int main()
{
  std::cout << "== static in function ==\n";
  std::cout << "request 1: " << get_request_id() << "\n";
  std::cout << "request 2: " << get_request_id() << "\n";
  std::cout << "request 3: " << get_request_id() << "\n";
  
  std::cout << "\n== static in class ==\n";
  Logger log1("app");
  std::cout << "instances: " << Logger::instance_count() << "\n";
  
  Logger log2("system");
  std::cout << "instances: " << Logger::instance_count() << "\n";
  
  Logger::print_stats();
  
  log1.log("hello");
  log2.log("world");
  
  return 0;
}
```

**Predict: will `request_id` be 1001, 1002, 1003? Will `instance_count` grow step by step?**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic static_all.cpp -o static_all && ./static_all
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/dPocTz5e1)

<details markdown="1"><summary>Answer (actual output)</summary>

```
== static in function ==
request 1: 1001
request 2: 1002
request 3: 1003

== static in class ==
instances: 1
instances: 2
Total instances created: 2
app: hello
system: world
```

</details>

Check these three points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. `id` increases on every call of `get_request_id()` (initialization happens only once)
2. `instance_count_` increases each time a `Logger` instance is created
3. `Logger::print_stats()` can be called with the class name, and it can access the `static` member

</details>

Next, try the following.

- Change `++instance_count_;` in the `Logger` constructor to a non-static member and look at the compile error
- Remove `inline static` and try defining it in a `.cpp` file

## Common pitfalls

**`error: undefined reference to 'Counter::count_'`**
You forgot to write the definition of the `static` member. Define it outside the class.
If you use C++17, you can also use `inline static` so that no definition is needed.

**`error: multiple definition of 'counter'`**
You defined a global variable with the same name in several `.cpp` files.
Keep one definition and either add `static` to the others, or move them to another file.

**I cannot initialize a `static` member from a function**
Initialize it in an initializer list or inside the class definition.

**A `static` member of a class can be accessed from an instance**
Technically it works, but you should call it as `ClassName::member`. It makes the intent clear.

**I want a function-local `static` to be thread-safe**
With C++11 or later you are fine. The initialization happens exactly once (Magic Statics).

## Matching exercise

After reading this chapter, practice with the matching drill.

- `cppb07_static`: the 3 meanings of static

```bash
./drill run cppb07
```

If you get stuck, run `./drill hint cppb07`. From the exercise side, `./drill read cppb07` brings you back to this chapter.

## References

- `cppreference`: [Static storage duration](https://en.cppreference.com/w/cpp/language/storage_duration) and [Static members](https://en.cppreference.com/w/cpp/language/static)
- [ROS 2 coding conventions](../ros2_coding_conventions.md): how to use global state

---

Previous → [6. `const`](06_const.md)
Next → [8. Other Qualifiers](08_other_qualifiers.md)
