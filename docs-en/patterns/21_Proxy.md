# 21. Proxy

> **Matches chapter 21 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep `Printer` / `PrinterProxy` / `Printable` open next to you.
>
> **Goal of this chapter**: In the book, `PrinterProxy` implements (inherits) `Printable` to pose as the real object.
> You can write the same thing in C++, but **that is not the mainstream way to write a Proxy in C++.**
> C++ has `operator->`. Both `std::unique_ptr` and `std::shared_ptr` are
> **Proxies that do nothing but provide `operator->`.** In this chapter, you write `operator->` yourself.
> Then you use a rule that is unique to C++: **`operator->` is called again and again until a pointer is returned.**
> With this rule, you can put code before and after every access.

## 21.1 Porting the Java version to C++ as is

This is the structure in the book.

```java
public interface Printable {
    public abstract void setPrinterName(String name);
    public abstract String getPrinterName();
    public abstract void print(String string);
}

public class PrinterProxy implements Printable {
    private String name;
    private Printer real = null;      // the "real object"
    public synchronized void print(String string) {
        realize();
        real.print(string);
    }
    private synchronized void realize() {
        if (real == null) { real = new Printer(name); }
    }
}
```

If you port it to C++ in a straightforward way, you get this.

```cpp
class Printable
{
public:
  virtual ~Printable() = default;                       // change 1
  virtual void set_printer_name(std::string name) = 0;
  virtual const std::string & printer_name() const = 0;  // change 2
  virtual void print(const std::string & text) = 0;
};

class PrinterProxy : public Printable
{
public:
  void print(const std::string & text) override
  {
    realize();
    real_->print(text);
  }

private:
  void realize() const                                  // change 3
  {
    if (!real_) { real_ = std::make_unique<Printer>(name_); }
  }

  std::string name_;
  mutable std::unique_ptr<Printer> real_;               // change 4
  mutable std::mutex mutex_;                            // change 5
};
```

There are 5 changes.

### Change 1: `virtual ~Printable() = default;`

You hold `Printable` in a `unique_ptr<Printable>` and destroy it through that pointer, so you need a virtual destructor.
It is the same in every chapter since chapter 1.

### Change 2: `String getPrinterName()` became `const std::string &`

In Java, a reference is returned, so no copy happens each time. In C++, if you write `std::string printer_name()`,
**the string is copied every time you call it.** Add `const &`.
Making the member function `const` is also your job in C++ (Java has no such distinction).

### Change 3: `realize()` becomes a `const` member function

`printer_name()` is `const`. But the book's `getPrinterName()`
**does not call `realize()`** (the Proxy already knows the name).
`print()` is not `const`, so there is no problem... you might think. In real projects, this breaks soon.

This is because there will always be a `const` query that cannot be answered without looking at the real object.
For example, `entry_count() const`. If `realize()` is not `const`, you are stuck.
**Make it `const` from the start.**

### Change 4: Add `mutable` to `real_`

You assign to `real_` from a `const` member function, so you need `mutable`.
If you remove it, you get this (measured with Apple clang).

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// printer_proxy.cpp
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

class Printer
{
public:
  explicit Printer(std::string name) : name_(std::move(name)) {}

  void print(const std::string & text) { std::cout << "[" << name_ << "] " << text << "\n"; }

private:
  std::string name_;
};

class Printable
{
public:
  virtual ~Printable() = default;                       // change 1
  virtual void set_printer_name(std::string name) = 0;
  virtual const std::string & printer_name() const = 0;  // change 2
  virtual void print(const std::string & text) = 0;
};

class PrinterProxy : public Printable
{
public:
  void set_printer_name(std::string name) override { name_ = std::move(name); }
  const std::string & printer_name() const override { return name_; }

  void print(const std::string & text) override
  {
    realize();
    real_->print(text);
  }

private:
  void realize() const                                  // change 3
  {
    if (!real_) { real_ = std::make_unique<Printer>(name_); }
  }

  std::string name_;
  std::unique_ptr<Printer> real_;                       // change 4 (mutable removed)
  mutable std::mutex mutex_;                            // change 5
};

int main()
{
  PrinterProxy proxy;
  proxy.set_printer_name("lp0");
  proxy.print("hello");
  return 0;
}
```

```bash
clang++ -std=c++17 -Wall -Wextra -Wpedantic printer_proxy.cpp -o printer_proxy
```

</details>

<!-- measure: env=clang filter="head -n 6; echo ..." -->
```
printer_proxy.cpp:43:25: error: no viable overloaded '='
   43 |     if (!real_) { real_ = std::make_unique<Printer>(name_); }
      |                   ~~~~~ ^ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1/__memory/unique_ptr.h:227:67: note: candidate function not viable: 'this' argument has type 'const std::unique_ptr<Printer>', but method is not marked const
  227 |   _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 unique_ptr& operator=(unique_ptr&& __u) _NOEXCEPT {
      |                                                                   ^
...
```

`mutable` is a tool for "logically `const`, but physically modified".
**Lazy creation is a typical example**, and a cache is the same. Do not overuse it, but this is a legitimate use.

### Change 5: `synchronized` becomes `std::mutex`

In Java, you only need to put `synchronized` on the method. C++ has no such thing, so
you hold a `mutable std::mutex mutex_;` and write a `std::lock_guard`.
`mutex_` is also `mutable`, because you lock even in a `const` member function.

**So far, this is the version that "moved the book's structure as is". In C++, there is a better way to write it.**

## 21.2 A C++ Proxy uses `operator->`, not inheritance

The book's `PrinterProxy` implements `Printable`.
In other words, it poses as the real object by "having the same interface as the real object".

In C++, there is a way to pose as the real object **without sharing any interface.**

```cpp
class CalibrationProxy
{
public:
  const CalibrationTable * operator->() const;   // this is all
};
```

Now you can write `proxy->entry(3)`. Even if `CalibrationTable` has 100 methods,
you write none of them in the Proxy. **You do not need a base class like `Printable` either.**

Why does this work? The compiler rewrites `proxy->entry(3)` to

```
proxy.operator->()->entry(3)
```

The `->` just continues on the pointer that the Proxy returned.

### You already use this every day

```cpp
std::unique_ptr<Motor> motor = std::make_unique<Motor>();
motor->set_duty(0.5);
```

`std::unique_ptr` does not inherit from `Motor`. Still, you can write `motor->set_duty(...)`.
This is because it has `operator->` and `operator*`.

> **Both `std::unique_ptr` and `std::shared_ptr` are implementations of the Proxy pattern.**
> `shared_ptr` is a Proxy that adds access control by reference counting,
> and `weak_ptr` is a Proxy that checks whether the object is alive before letting you through (only `lock()` is explicit).

In chapter 20, you used `shared_ptr` as a "tool for sharing". Now you look at the same thing
as "a place to put code in front of access".

### Inheritance version vs. `operator->` version

| | Inheritance version (the book's form) | `operator->` version |
| --- | --- | --- |
| Can it be passed as the same type as the real object? | Yes (as `Printable &`) | No |
| When the real object gets more methods | Add them to the Proxy too | **Do nothing** |
| Cost of virtual functions | Yes | Zero |
| Can you put different code on each method? | Yes | No (the same code for all) |
| Can you use it on a microcontroller? | Costs a vtable | Nothing that costs |

**It depends on "do you need to pass it as the real object?"** If you do not, use the `operator->` version.
And in team libraries, you usually do not need to.

## 21.3 The `operator->` chain (drill-down) — a rule unique to C++

This is the most interesting part of `operator->`.

> **If the return value of `operator->` is not a pointer, `operator->` is called again on that return value.
> This repeats until a pointer is returned.**

No other operator has this rule. `+` is not applied again to the return value of `operator+`.

What is good about it? **You can put code just before and just after the access to the real object.**

```cpp
class Guard
{
public:
  explicit Guard(Real * real) : real_(real) { /* entry code */ }
  ~Guard() { /* exit code */ }
  Real * operator->() const { return real_; }   // a pointer here. The chain stops
private:
  Real * real_;
};

class Proxy
{
public:
  Guard operator->() const { return Guard{real_}; }   // not a pointer. The chain continues
private:
  Real * real_;
};
```

`proxy->work()` is expanded like this.

1. `Proxy::operator->()` → `Guard` (a temporary object. The constructor runs here)
2. `Guard` is not a pointer, so `->` is applied again
3. `Guard::operator->()` → `Real *` (a pointer. It stops)
4. Call `Real::work()`
5. **The expression ends, so `Guard` is destroyed** (the destructor runs)

The lifetime of a temporary object lasts **until the end of the expression (the semicolon).**
So **the call to `work()` is always between the constructor and the destructor.**

## 21.4 Who owns the real object?

There are 2 kinds of Proxy, and their ownership is the opposite.

| Kind | Example | How it holds the real object |
| --- | --- | --- |
| **Virtual Proxy** (lazy creation) | A heavy calibration table | `mutable std::unique_ptr<Real>`. The Proxy owns it |
| **Protection Proxy** (check, record) | Register access | `Real &` or `Real *`. **The Proxy does not own it** |

A Virtual Proxy creates the object itself, so it owns it. Use `unique_ptr`.
A Protection Proxy only wraps something that already exists, so it holds a reference. **It does not own it.**

And the side that holds a reference has the same constraint we have repeated since chapter 1.

```cpp
SafeRegisterProxy make_proxy()
{
  RegisterFile file;              // local
  return SafeRegisterProxy{file}; // file dies here
}                                 // the returned Proxy points to a dead real object
```

**In Java, the GC keeps it alive. In C++, it crashes.**
When you write a function that returns a Proxy, always check the lifetime of the real object.

### Where you use `friend`

The exercise's `RegisterAccess` needs to reach the non-`const` real object held by `SafeRegisterProxy`.
The `file()` accessor returns `const RegisterFile &`, so you cannot write through it.

There are 3 choices.

| Method | Verdict |
| --- | --- |
| Use `const_cast` | **Do not.** Removing `const` is the last resort |
| Add a non-`const` `raw_file()` as public | Anyone can bypass the check. The Proxy loses its meaning |
| Make `RegisterAccess` a `friend` | **This one.** Only the temporary object that the Proxy itself creates can bypass it |

`friend` is often disliked because it "breaks encapsulation", but
**a Proxy and the helper object that the Proxy creates** are a legitimate use of `friend`.
The standard library does the same thing.

## 21.5 What is the difference between Proxy and Decorator (chapter 12)?

The structure is almost the same. Both wrap the real object and forward calls. **The difference is the purpose.**

| | Decorator (chapter 12) | Proxy (chapter 21) |
| --- | --- | --- |
| Purpose | **Add behavior** | Keep the behavior the same and **control access** |
| Result seen by the caller | Changes (more logs, encryption) | Does not change (it looks like it does not) |
| Creation of the real object | Passed in from outside | **The Proxy decides** (it can create it lazily) |
| Do you stack many of them? | Stacking is the premise | Usually 1 layer |

The Decorator in chapter 12 that adds a timestamp to `LogSink` **changes the output**.
The `CalibrationProxy` in this chapter does not change the return value of `entry(3)` by even a millimeter.
The only thing it changes is "when to create the real object".

When you are not sure, decide by "**does the result change from the caller's view?**"
If it changes, it is a Decorator. If not, it is a Proxy.

## 21.6 Is there the same thing in the standard library / language?

**There are plenty. This chapter is about "writing it yourself to understand what the standard already has".**

| Standard item | What kind of Proxy it is |
| --- | --- |
| `std::unique_ptr` / `std::shared_ptr` | The smallest Proxy that has `operator->` |
| `std::weak_ptr` | Checks whether the object is alive before letting you through. A Proxy where `lock()` is explicit |
| `std::vector<bool>::reference` | A Proxy that makes one bit look like a `bool`. **A famous trap** |
| `operator->` of `std::optional` | Lets you through only when it has a value |
| `std::reference_wrapper` | Carries a reference around like a value |
| `std::lock_guard` | A container for "lock only during the access". Exactly the `Guard` in 21.3 |

### About `std::vector<bool>`

```cpp
std::vector<bool> flags(4);
auto flag = flags[0];        // not a bool. A Proxy comes back
flag = true;                 // flags[0] is changed (even though you thought you copied it)
```

`std::vector<bool>` packs bits, so `operator[]` cannot
return `bool &`. It returns a Proxy instead.
When this is combined with `auto`, the behavior is against intuition.
It is famous as a **pitfall of designs that return a Proxy.**

When you write your own Proxy, **always check what happens when it is received with `auto`.**

### A practical idiom: `operator->` returns a temporary object

The structure in 21.3 becomes a thread-safe accessor as it is.

```cpp
template <typename T>
class Locked
{
public:
  class Handle
  {
  public:
    Handle(T & object, std::mutex & mutex) : object_(object), lock_(mutex) {}
    Handle(const Handle &) = delete;
    T * operator->() const { return &object_; }

  private:
    T & object_;
    std::lock_guard<std::mutex> lock_;
  };

  Handle operator->() { return Handle{object_, mutex_}; }

private:
  T object_;
  std::mutex mutex_;
};

Locked<SensorBuffer> buffer;
buffer->push(10);        // lock -> push -> unlock. You cannot forget it
```

Just by writing `buffer->push(10)`, the lock is held only during `push`.
**The syntax does not let you forget to take the lock.**
This is safer than holding a `std::mutex` directly and writing a `lock_guard` in every member function.

But there is one pitfall.

```cpp
if (buffer->size() > 0) {     // lock -> release here
  buffer->pop();              // another lock. Another thread can empty it in between
}
```

**The lock is released at every expression.** If you want to make several operations exclusive together,
receive the `Handle` explicitly in a variable.

```cpp
auto handle = buffer.operator->();   // the lock continues while handle is alive
```

## 21.7 Try it yourself

You see the `operator->` chain and the lifetime of the temporary object with your own eyes.
**Predict the order of the output** before you run it.

```cpp
// try.cpp
#include <iostream>
#include <memory>

class Real
{
public:
  void work() { std::cout << "  Real::work\n"; }
};

// The first-level Proxy. It returns a Guard, not a pointer.
class Guard
{
public:
  explicit Guard(Real * real) : real_(real) { std::cout << "  Guard created\n"; }
  ~Guard() { std::cout << "  Guard destroyed\n"; }
  Guard(const Guard &) = delete;
  Real * operator->() const { return real_; }

private:
  Real * real_;
};

class Proxy
{
public:
  explicit Proxy(Real * real) : real_(real) {}
  Guard operator->() const { return Guard{real_}; }

private:
  Real * real_;
};

int main()
{
  Real real;
  Proxy proxy{&real};

  std::cout << "before the expression\n";
  proxy->work();
  std::cout << "after the expression\n";

  // std::unique_ptr is also just a Proxy that has operator->
  std::unique_ptr<Real> owned = std::make_unique<Real>();
  owned->work();
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

The C++11 error at the end of the answer was measured with this minimal program.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// copy_elision.cpp
class A
{
public:
  A() = default;
  A(const A &) = delete;
};

A make() { return A{}; }

int main()
{
  A a = make();
  (void)a;
  return 0;
}
```

```bash
g++ -std=c++11 -Wall -Wextra -Wpedantic copy_elision.cpp -o copy_elision
```

</details>

<details>
<summary>Predict: in the single line <code>proxy-&gt;work()</code>, what is called, and how many times?</summary>

<!-- measure: files=try.cpp cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try" -->
```
before the expression
  Guard created
  Real::work
  Guard destroyed
after the expression
  Real::work
```

In this **one line**, `proxy->work()`, a `Guard` is created and destroyed.
The caller does not remember writing a `Guard`. The `operator->` chain does it behind the scenes.

Look at the order. `Guard created` → `Real::work` → `Guard destroyed`.
**The call to the real object is always sandwiched.** If you put `lock()` / `unlock()` there,
the lock is taken without the caller writing anything.

`Guard` can be neither copied nor moved, yet `operator->` can return it by value.
This is thanks to the **guaranteed copy elision** of C++17. If you compile a minimal program of the same shape with C++11, you get this.

<!-- measure: -->
```
copy_elision.cpp: In function ‘A make()’:
copy_elision.cpp:9:19: error: use of deleted function ‘A::A(const A&)’
    9 | A make() { return A{}; }
      |                   ^~~
copy_elision.cpp:6:3: note: declared here
    6 |   A(const A &) = delete;
      |   ^
copy_elision.cpp: In function ‘int main()’:
copy_elision.cpp:13:14: error: use of deleted function ‘A::A(const A&)’
   13 |   A a = make();
      |              ^
copy_elision.cpp:6:3: note: declared here
    6 |   A(const A &) = delete;
      |   ^
```
</details>

## 21.8 Conclusion for microcontrollers

**You cannot use a Virtual Proxy (lazy creation) on a microcontroller.** There is one reason.

```cpp
real_ = std::make_unique<CalibrationTable>(source_);   // heap allocation in the middle of the loop
```

Code that allocates at an unknown time cannot be written on bare metal.
And there is no way out when the allocation fails (because of `-fno-exceptions`).
You load the calibration table **into a fixed region at startup.** You do not delay it.

**What is practical on a microcontroller is a Proxy that wraps register access.**
You put a range check or a unit conversion on a read or write to a `volatile` address.

```cpp
// duty_register.cpp
#include <cstdint>
#include <cstdio>

// A fake peripheral so that this runs on a PC. On real hardware, it is the address in the datasheet.
volatile std::uint16_t g_pwm_duty = 0;

// A Proxy that wraps one register. No dynamic allocation, no exceptions, no vtable.
// Reads and writes look the same as a raw register. Only the range clamp is added.
class DutyRegister
{
public:
  static constexpr std::uint16_t kMax = 999;

  explicit DutyRegister(volatile std::uint16_t * reg) : reg_(reg) {}

  // Read: called when you use it like a value
  operator std::uint16_t() const { return *reg_; }

  // Write: called when you write reg = value. Out-of-range values are clamped to kMax
  DutyRegister & operator=(std::uint16_t value)
  {
    *reg_ = (value > kMax) ? kMax : value;
    return *this;
  }

  DutyRegister & operator=(const DutyRegister &) = delete;

private:
  volatile std::uint16_t * reg_;
};

int main()
{
  DutyRegister duty{&g_pwm_duty};

  duty = 500;
  std::printf("duty=%u raw=%u\n", static_cast<unsigned>(duty), static_cast<unsigned>(g_pwm_duty));

  duty = 5000;  // out of range. The Proxy clamps it
  std::printf("duty=%u raw=%u\n", static_cast<unsigned>(duty), static_cast<unsigned>(g_pwm_duty));

  std::printf("sizeof(DutyRegister)=%zu\n", sizeof(DutyRegister));
  return 0;
}
```

This is the result.

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic duty_register.cpp -o duty_register && ./duty_register" -->
```
duty=500 raw=500
duty=999 raw=999
sizeof(DutyRegister)=8
```

**`duty = 5000;` is silently clamped to 999.**
If you wrote directly to a raw register, the upper bits could have another meaning, and the hardware would run out of control.

This example does not use `operator->`. **A register is a single value, so
the combination of `operator=` and `operator T()` is natural.**
`operator->` is for wrapping a real object that has multiple members.

### Do not drop `volatile`

```cpp
std::uint16_t * reg_;             // forgot to write volatile
```

If you do this, the compiler decides that "reading the same address twice is pointless" and
merges the reads. The assumption that **hardware changes the value by itself** disappears.
Keeping `volatile` inside the Proxy also has the effect of **reducing the places where you can forget to write it to one.**

### If you make the address a template argument, it becomes 1 byte

The example above holds a `volatile std::uint16_t *`, so it is 8 bytes.
If the address is known at compile time, you can make it a template argument.

```cpp
template <std::uint32_t Address, std::uint16_t Mask = 0xffffu>
class RegisterProxy
{
  static volatile std::uint16_t * reg()
  {
    return reinterpret_cast<volatile std::uint16_t *>(static_cast<std::uintptr_t>(Address));
  }
  // ...
};
```

With this form, `sizeof` is **1** (the minimum size of an empty class). It uses no RAM at all.
I measured it.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// register_proxy_size.cpp
#include <cstdint>
#include <cstdio>

template <std::uint32_t Address, std::uint16_t Mask = 0xffffu>
class RegisterProxy
{
  static volatile std::uint16_t * reg()
  {
    return reinterpret_cast<volatile std::uint16_t *>(static_cast<std::uintptr_t>(Address));
  }
  // ...
};

int main()
{
  std::printf("sizeof(RegisterProxy) = %zu\n", sizeof(RegisterProxy<0x40012C34u>));
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic register_proxy_size.cpp -o register_proxy_size && ./register_proxy_size
```

</details>

<!-- measure: -->
```
sizeof(RegisterProxy) = 1
```

**When you write a Proxy for a microcontroller, consider this form first.**

## 21.9 Conclusion for ROS 2 (supplement)

rclcpp has no class named "Proxy", but there are many Proxies themselves.

- `rclcpp::Node::SharedPtr` — a `shared_ptr`, so it is a Proxy with `operator->`
- `rclcpp::Publisher<T>::SharedPtr` — the same. `pub->publish(msg)` is a call through a Proxy
- `rclcpp::LoanedMessage` — wraps memory that DDS lent out and returns it in the destructor.
  It has **the same structure as the `Guard` in 21.3** (we touched on this in the zero-copy exercise of chapter 14)

In ROS 2, you can use dynamic allocation and exceptions, so you can also choose a Virtual Proxy (lazy creation of a heavy resource).
For example, "load a large map only when the first query comes".

But a node's callbacks are not always single-threaded.
**Make a lazily creating Proxy thread-safe.** You need the `mutable std::mutex` in 21.1.
If 2 threads make the first access at the same time, the real object is created twice.

## 21.10 Common pitfalls

| Symptom | Cause |
| --- | --- |
| `error: no viable overloaded '='` (with `unique_ptr`) | `operator->` is `const`, but `real_` is not `mutable` |
| `proxy->method()` does not compile | The return value of `operator->` is neither a pointer nor a Proxy. It returns something like `int` |
| `operator->` is called forever and the compile never finishes | The end of the chain is not a pointer. It returns itself |
| It was created lazily, but it is rebuilt every time | The `if (!real_)` check is missing |
| The lock of the temporary object does not work | It is released at every expression (21.6). Receive the `Handle` in a variable |
| `auto x = proxy[0];` behaves strangely | You receive the Proxy with `auto`. The same trap as `std::vector<bool>` |
| A function that returns a Proxy crashes | A Protection Proxy outlives the real object (21.4) |
| Reads disappear when optimizing on a microcontroller | You dropped `volatile` |
| You start to want to add features to the Proxy | That is not a Proxy but a Decorator (21.5). Do not mix them |

## 21.11 Matching exercise

```bash
./drill run dp21
```

In `exercises/dp21_proxy/src/calibration_proxy.cpp`, you implement

1. **`CalibrationProxy::operator->()`** — lazy creation with `mutable std::unique_ptr`
2. **The constructor / destructor / `operator->` of `RegisterAccess`** — the second level of the chain
3. **`SafeRegisterProxy::read()` / `write()`** — range check and recording

The tests check

- The real object is not created until the first access (checked with a creation counter)
- It is not rebuilt on the second and later accesses (checked with the address and the creation counter)
- **A `const` Proxy can also create lazily** (checks `mutable`)
- An out-of-range access **does not reach the real object** (checked with the access count on the real object)
- The `operator->` chain happens (checks the return type with `static_assert`)
- The lifetime of the temporary object sandwiches the access to the real object (checked with the order of the records)

## 21.12 Summary of this chapter

- **`std::unique_ptr` / `std::shared_ptr` are Proxies themselves.** They only have `operator->`
- A C++ Proxy **writes `operator->`** instead of using inheritance. Even if the real object gets more methods, the Proxy does not change
- **`operator->` is called repeatedly until a pointer is returned** (drill-down). A rule unique to C++
- With that rule and the lifetime of temporary objects, you can **put code before and after the access to the real object** (lock, record)
- Lazy creation is `mutable std::unique_ptr` + a `const` `operator->`. A legitimate use of `mutable`
- A Virtual Proxy **owns** the real object. A Protection Proxy **does not own** it (watch the lifetime)
- **A Decorator adds behavior. A Proxy looks the same and controls access**
- On microcontrollers, you do not use lazy creation. A Proxy that **wraps a `volatile` register** is practical
- If you receive a Proxy with `auto`, the behavior changes. The same trap as `std::vector<bool>`

---

Previous: [20. Flyweight](20_Flyweight.md) / Next: [22. Command](22_Command.md)
