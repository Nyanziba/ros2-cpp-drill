# 3. Template Method

> **Corresponds to Yuki's book, Chapter 3.** Please open `AbstractDisplay` and `CharDisplay` / `StringDisplay` next to you.
>
> **Goal of this chapter**: `AbstractDisplay` in Yuki's book has a structure where
> **`public final void display()` calls the `protected abstract` methods `open` / `print` / `close`**.
> When you move it to C++, three differences appear in this one line at the same time.
> `final` becomes "do not write `virtual`", `protected abstract` becomes "a `private` pure virtual function",
> and a **virtual destructor** becomes newly necessary.
> C++ also has one pitfall that Java does not have.
> **If you call a virtual function from a constructor, the derived implementation is not called.**
> This form is called the **NVI (Non-Virtual Interface) idiom**, and it is
> the standard way to write Template Method in C++.

## 3.1 If you write the Java version in C++ as is

`AbstractDisplay` in Yuki's book looks like this.

```java
public abstract class AbstractDisplay {
    public abstract void open();
    public abstract void print();
    public abstract void close();
    public final void display() {
        open();
        for (int i = 0; i < 5; i++) {
            print();
        }
        close();
    }
}
```

If you move it to C++ in the plain way, it looks like this.

```cpp
class AbstractDisplay
{
public:
  virtual ~AbstractDisplay() = default;

  void display()                 // ← the skeleton. No virtual
  {
    open();
    for (int i = 0; i < 5; ++i) {
      print();
    }
    close();
  }

private:                         // ← Java uses protected. In C++, private is fine
  virtual void open() = 0;
  virtual void print() = 0;
  virtual void close() = 0;
};
```

There are three changes. **All of them have a meaning.**

### Change 1: Added `virtual ~AbstractDisplay() = default;`

This is the same as chapter 1. **If you destroy through a base class pointer, the destructor must be virtual.**
Template Method is a pattern that "holds by the base type and calls the derived contents",
so destruction through a base pointer always happens. If you forget it, it leaks silently.

For details, see [C++ 3.5 Virtual destructors](../cpp/03_inheritance.md).

### Change 2: `final void display()` → `void display()` (do not write `virtual`)

A Java method is **virtual even if you do not write anything**. So when you do not want it replaced, you add `final`.
C++ is the opposite: **if you do not write `virtual`, it is not virtual from the start.**

| | Can be replaced | Cannot be replaced |
| --- | --- | --- |
| Java | `void f()` (default) | `final void f()` |
| C++ | `virtual void f()` | `void f()` (default) |

**The default is reversed.** This is the first place where people coming from Java have an accident, and there are two ways to have it.

```cpp
// Accident 1: you make even the skeleton virtual
virtual void display() { ... }   // the derived class can replace all of it = you cannot protect the skeleton

// Accident 2: you forget to write virtual on a part you want replaced
void open();                     // even if the derived class writes open(), it is not called. The base runs silently
```

Accident 2 **compiles**. If you call `display()` through `AbstractDisplay *`,
the base `open()` runs silently. This does not happen in Java.

If you want to state "do not replace" for `display()`, you can only **write it in a comment**.
The absence of `virtual` is itself the declaration of "do not replace".

### Change 3: `protected abstract` → `private virtual`

This is the part that people from Java find hardest to believe.

> **In C++, a derived class can override a `private` virtual function.**

**The access specifier and whether you can override are independent.** What `private` forbids is
only "calling that name", not "replacing it".

```cpp
class Sensor
{
private:
  virtual void setup() { }       // private
};

class Imu : public Sensor
{
private:
  void setup() override { }      // OK. You can override even if it is private
};
```

A Java `private` method cannot be overridden (even if it looks like you did, it becomes a different method),
so Yuki's book uses `protected abstract`. In C++ you can make it `private`, and that is better.
The reason is that **you do not give the derived class the right to "call"**.

```cpp
class Imu : public Sensor
{
private:
  void setup() override
  {
    Sensor::setup();     // error because it is private. The path from the derived class to the base version is closed
  }
};
```

This is the actual error.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// private_call.cpp
class Sensor
{
private:
  virtual void setup() { }       // private
};

class Imu : public Sensor
{
private:
  void setup() override
  {
    Sensor::setup();     // error because it is private. The path from the derived class to the base version is closed
  }
};

int main()
{
  Imu imu;
  (void)imu;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic private_call.cpp -o private_call
```

</details>

```
private_call.cpp: In member function ‘virtual void Imu::setup()’:
private_call.cpp:13:18: error: ‘virtual void Sensor::setup()’ is private within this context
   13 |     Sensor::setup();     // error because it is private. The path from the derived class to the base version is closed
      |     ~~~~~~~~~~~~~^~
private_call.cpp:5:16: note: declared private here
    5 |   virtual void setup() { }       // private
      |                ^~~~~
```

All the derived class can do is "fill in the contents".
**Calling the steps is monopolized by the skeleton.** This is the NVI (Non-Virtual Interface) idiom.

| | Access specifier | Meaning |
| --- | --- | --- |
| Skeleton `display()` | `public` non-virtual | Can be called from outside. Cannot be replaced |
| Each step such as `open()` | `private virtual` | Cannot be called from outside or from derived classes. Can only be replaced |
| A step where the derived class wants to call the base version | `protected virtual` | After replacing it, you can still call the base version |

Make **only the third row `protected`**. A hook that "adds a condition to the base check" has this form.
`validate()` in the exercise is an example: the derived class calls `SensorReader::validate()` and then
adds its own range check. **If you do not need to call it, leave it `private`.**

## 3.2 Always write `override`

In the structure of chapter 3, whether you write `override` decides between life and death.

```cpp
class Sensor
{
private:
  virtual bool check(double value) const { return value == value; }
};

class Imu : public Sensor
{
private:
  bool check(double value) { return value > 0.0; }   // dropped const
};
```

`check(double) const` and `check(double)` are **different signatures**.
`Imu::check` is not an override, it becomes **a completely new function**.
What the skeleton calls is still the base `check`. **It compiles, and there is no warning.**

If you write `override`, it becomes an error on the spot.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// override_const.cpp
class Sensor
{
private:
  virtual bool check(double value) const { return value == value; }
};

class Imu : public Sensor
{
private:
  bool check(double value) override { return value > 0.0; }   // dropped const. Added override
};

int main()
{
  Imu imu;
  (void)imu;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic override_const.cpp -o override_const
```

</details>

```
override_const.cpp:11:8: error: ‘bool Imu::check(double)’ marked ‘override’, but does not override
   11 |   bool check(double value) override { return value > 0.0; }   // dropped const. Added override
      |        ^~~~~
override_const.cpp:5:16: warning: ‘virtual bool Sensor::check(double) const’ was hidden [-Woverloaded-virtual=]
    5 |   virtual bool check(double value) const { return value == value; }
      |                ^~~~~
override_const.cpp:11:8: note:   by ‘bool Imu::check(double)’
   11 |   bool check(double value) override { return value > 0.0; }   // dropped const. Added override
      |        ^~~~~
```

`override` is a declaration that "I intend to replace a virtual function of the base", and
if that is not true, the compiler stops. In Template Method, the worst kind of breakage is that **the skeleton keeps calling the base version silently**,
so write it without exception.

Conversely, if you put `override` on a non-virtual skeleton, this happens.

```cpp
class Imu : public Sensor
{
public:
  void boot() override { }     // Sensor::boot() is non-virtual
};
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// override_boot.cpp
class Sensor
{
public:
  void boot() { }     // non-virtual skeleton
};

class Imu : public Sensor
{
public:
  void boot() override { }     // Sensor::boot() is non-virtual
};

int main()
{
  Imu imu;
  imu.boot();
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic override_boot.cpp -o override_boot
```

</details>

```
override_boot.cpp:11:8: error: ‘void Imu::boot()’ marked ‘override’, but does not override
   11 |   void boot() override { }     // Sensor::boot() is non-virtual
      |        ^~~~
```

**This is a good mistake.** It lets you notice that "I am trying to replace the skeleton".

## 3.3 Stop replacement with `final` (C++11)

After you implement one step, you sometimes do not want classes derived further to replace it.

```cpp
class Sensor
{
private:
  virtual void setup() final { }    // no replacement from here on
};
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// final_setup.cpp
class Sensor
{
private:
  virtual void setup() final { }    // no replacement from here on
};

class Imu : public Sensor
{
private:
  void setup() override { }         // tried to replace a final function
};

int main()
{
  Imu imu;
  (void)imu;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic final_setup.cpp -o final_setup
```

</details>

```
final_setup.cpp:11:8: error: virtual function ‘virtual void Imu::setup()’ overriding final function
   11 |   void setup() override { }         // tried to replace a final function
      |        ^~~~~
final_setup.cpp:5:16: note: overridden function is ‘virtual void Sensor::setup()’
    5 |   virtual void setup() final { }    // no replacement from here on
      |                ^~~~~
```

It has the same effect as a Java `final` method, but in C++ you use it to **cut the chain of `virtual` in the middle**.
If you put it on the class itself, inheritance itself stops.

```cpp
class EncoderReader final : public SensorReader { };   // no more derived classes
```

`EncoderReader` / `ThermistorReader` in the exercise have this form.
**"Put `final` on leaf classes"** is more than a statement of intent in C++:
the compiler can replace a virtual call with a direct call (devirtualization).

## 3.4 A danger specific to C++: calling a virtual function from a constructor

**This is where the difference from Java is clearest.**

```cpp
class Sensor
{
public:
  Sensor()
  {
    setup();          // dangerous
  }
private:
  virtual void setup() { /* base implementation */ }
};
```

In C++, **while the base class constructor is running, the object is still the base class.**
The vptr points to the base vtable. So `setup()` calls **the base implementation**.
The derived override is not called.

Java is the opposite: even from a constructor, **the derived override is called**
(it runs with fields uninitialized, which is a famous source of bugs in its own way).

The same thing happens in destructors. The base destructor runs after the derived destruction is finished,
so a virtual function called from there is the base implementation.

If it was a pure virtual function, it is even worse: **it crashes at run time**.

```cpp
struct B { B() { setup(); } virtual ~B() = default; virtual void setup() = 0; };
struct D : B { void setup() override { std::printf("D\n"); } };
int main() { D d; (void)d; }
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// pure_virtual.cpp
#include <cstdio>

struct B { B() { setup(); } virtual ~B() = default; virtual void setup() = 0; };
struct D : B { void setup() override { std::printf("D\n"); } };
int main() { D d; (void)d; }
```

```bash
clang++ -std=c++17 -Wall -Wextra -Wpedantic pure_virtual.cpp -o pure_virtual && ./pure_virtual
```

</details>

```
pure_virtual.cpp:4:18: warning: call to pure virtual member function 'setup' has undefined behavior; overrides of 'setup' in subclasses are not available in the constructor of 'B' [-Wcall-to-pure-virtual-from-ctor-dtor]
    4 | struct B { B() { setup(); } virtual ~B() = default; virtual void setup() = 0; };
      |                  ^
pure_virtual.cpp:4:53: note: 'setup' declared here
    4 | struct B { B() { setup(); } virtual ~B() = default; virtual void setup() = 0; };
      |                                                     ^
1 warning generated.
```

```
libc++abi: Pure virtual function called!
```

(This is the run result with Apple clang 21 / libc++. If you build the same program with g++ 13.3 / libstdc++ on Linux,
the warning is `pure virtual ‘virtual void B::setup()’ called from constructor`,
and linking fails with `undefined reference to 'B::setup()'`, so no executable is produced.)

**In this case a warning is shown**, but when the base has an implementation, as at the start of 3.4,
the base is called silently without even a warning. That case is more troublesome.

**Rule**: Do not call virtual functions from constructors or destructors.
If part of the initialization should be left to the derived class, **do it inside the skeleton method, not in the constructor**.
This is why `read_once()` in the exercise has the form "call `initialize()` only the first time".

```cpp
std::optional<double> SensorReader::read_once()
{
  if (!is_initialized_) {
    initialize();            // from here, the derived implementation is called
    is_initialized_ = true;
  }
  // ...
}
```

## 3.5 Does the standard library or a language feature already have the same thing?

There is no language feature that corresponds to Template Method. **In this chapter, writing it yourself is the right answer.**
However, the structure "fix the skeleton, replace only a part" also appears in the standard library.

| Standard library | Skeleton | Part to replace |
| --- | --- | --- |
| `std::sort` | The whole sort algorithm | Comparison (comparator) |
| `std::basic_streambuf` | The I/O procedure of the stream | `overflow` / `underflow` (**protected virtual**) |

`std::basic_streambuf` is one of the few NVI examples inside the standard library.
`pubsync()` (public non-virtual) calls `sync()` (protected virtual).
Because they form **a pair of a public function with `pub` and a virtual function without it**,
you can see the structure directly if you look at the header.

On the other hand, `std::sort` **uses no virtual functions at all**. It receives the replaceable part as a template argument.
The decision is "if you do not need to replace at run time, do not use virtual functions",
and this leads to CRTP in 3.7. The choice of the replacement method itself is
the main topic of chapter 10 (Strategy).

## 3.6 Try it yourself

Before you solve the exercise, compile this one file and **predict the output** before you run it.

```cpp
#include <iostream>

class Sensor
{
public:
  Sensor()
  {
    std::cout << "Calling setup() from Sensor()\n";
    setup();                       // dangerous. The derived class does not exist yet
  }
  virtual ~Sensor() = default;

  // The skeleton. No virtual = do not allow replacement
  void boot()
  {
    std::cout << "Calling setup() from boot()\n";
    setup();
  }

private:
  // Even though it is private, the derived class can override it
  virtual void setup() { std::cout << "  Sensor::setup\n"; }
};

class Imu : public Sensor
{
private:
  void setup() override { std::cout << "  Imu::setup\n"; }
};

int main()
{
  Imu imu;
  imu.boot();
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// try_boot.cpp
#include <iostream>

class Sensor
{
public:
  Sensor()
  {
    std::cout << "Calling setup() from Sensor()\n";
    setup();                       // dangerous. The derived class does not exist yet
  }
  virtual ~Sensor() = default;

  // The skeleton. No virtual = do not allow replacement
  void boot()
  {
    std::cout << "Calling setup() from boot()\n";
    setup();
  }

private:
  // Even though it is private, the derived class can override it
  virtual void setup() { std::cout << "  Sensor::setup\n"; }
};

class Imu : public Sensor
{
public:
  void boot() override { }      // tried to replace the non-virtual skeleton

private:
  void setup() override { std::cout << "  Imu::setup\n"; }
};

int main()
{
  Imu imu;
  imu.boot();
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try_boot.cpp -o try_boot
```

</details>

<details>
<summary>Predict: <code>setup()</code> is called twice. Is it <code>Imu::setup</code> both times?</summary>

**No. The first time it is `Sensor::setup`.**

```
Calling setup() from Sensor()
  Sensor::setup
Calling setup() from boot()
  Imu::setup
```

While the constructor is running, the object is still a `Sensor`.
**In Java, `Imu::setup` is called both times.** This is the biggest difference.

You can also confirm two more things.

- `Imu` can `override` the `private virtual` `setup()`
- `boot()` is non-virtual, so `Imu` has no way to replace it

Try adding `void boot() override { }` to `Imu`.

```
try_boot.cpp:29:8: error: ‘void Imu::boot()’ marked ‘override’, but does not override
   29 |   void boot() override { }      // tried to replace the non-virtual skeleton
      |        ^~~~
```

**The skeleton is protected.**
</details>

## 3.7 Conclusion for microcontrollers

**You can write it with virtual functions, too.** Unlike Iterator, Template Method
**does not cause dynamic allocation inside a loop**. You do not even need to hold it by a base class pointer.
You can place the object statically, as in `EncoderReader encoder{...};`.

```cpp
static EncoderReader encoder{...};      // global or static. Zero allocation
encoder.read_once();                    // 3 to 4 virtual calls
```

It does not conflict with `-fno-exceptions` / `-fno-rtti`. Virtual functions need neither exceptions nor RTTI.
You may keep the return value as `std::optional<double>` (`optional` does not allocate).
Just keep this rule: **do not report failure with `throw`**.

Two costs remain.

1. A vptr of 8 bytes per object (4 bytes on a 32-bit microcontroller)
2. A vtable per class goes into ROM
3. 3 to 4 virtual calls per read. They are not inlined

**With 3 sensors at 1 kHz, this is within the noise. Write it.**
It becomes a problem when "there are dozens of derived classes" or "the read is inside an interrupt handler".

### The same structure with zero vtables: CRTP

If the replacement is **decided at compile time**, you do not need virtual functions.
You make the base class a template on "the type of the derived class".
This form is called **CRTP (Curiously Recurring Template Pattern)**.

```cpp
#include <cstdint>
#include <cstdio>

// CRTP version of Template Method. No vtable, no dynamic allocation, no exceptions
template <typename Derived>
class SensorReaderBase
{
public:
  // The skeleton. There is not a single virtual
  bool read_once(std::int32_t * out)
  {
    Derived & self = static_cast<Derived &>(*this);
    if (!is_initialized_) {
      self.initialize();
      is_initialized_ = true;
    }
    const std::int32_t raw = self.fetch_raw();
    const std::int32_t value = self.convert(raw);
    if (!self.validate(value)) {
      return false;
    }
    *out = value;
    return true;
  }

protected:
  // Default hook. If the derived class defines the same name, that one is chosen
  bool validate(std::int32_t) const { return true; }

private:
  bool is_initialized_ = false;
};

class AdcThermistor : public SensorReaderBase<AdcThermistor>
{
public:
  void initialize() { index_ = 0; }
  std::int32_t fetch_raw() { return samples_[index_++ % 3]; }
  std::int32_t convert(std::int32_t raw) const { return raw / 10 - 20; }
  bool validate(std::int32_t celsius) const { return celsius >= -10 && celsius <= 120; }

private:
  std::int32_t samples_[3] = {200, 400, 2000};
  unsigned index_ = 0;
};

int main()
{
  AdcThermistor sensor;
  for (int i = 0; i < 3; ++i) {
    std::int32_t value = 0;
    if (sensor.read_once(&value)) {
      std::printf("ok %d\n", value);
    } else {
      std::printf("rejected\n");
    }
  }
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti crtp.cpp -o crtp && ./crtp
```

```
ok 0
ok 20
rejected
```

The goal of Template Method, **the skeleton is in one place**, is fully achieved,
and there is not a single `virtual`. With `static_cast<Derived &>(*this)`, you only tell the compiler
"I am actually the derived class".

The size also gets smaller. Compare classes that have the same members (one `bool` and one `unsigned`).

```cpp
#include <cstdio>

template <typename Derived>
struct CrtpBase { bool is_initialized_ = false; };
struct CrtpSensor : CrtpBase<CrtpSensor> { unsigned index_ = 0; };

struct VirtualBase { virtual ~VirtualBase() = default; bool is_initialized_ = false; };
struct VirtualSensor : VirtualBase { unsigned index_ = 0; };

int main()
{
  std::printf("CRTP    : %zu\n", sizeof(CrtpSensor));
  std::printf("virtual : %zu\n", sizeof(VirtualSensor));
  return 0;
}
```

```
CRTP    : 8
virtual : 16
```

**The 8 bytes of the vptr, and the alignment padding that comes with it, disappear entirely.** (Measured on arm64 / Apple clang 17)
All calls also become candidates for inlining.

CRTP has two costs.

| What you lose | Details |
| --- | --- |
| Run-time polymorphism | You cannot mix them in `std::vector<SensorReaderBase *>`. Each type gets a separate list |
| Error messages | If you forget to implement a step, the error appears inside the template. It is hard to read |

**The decision is as follows.**

| Situation | What to choose |
| --- | --- |
| The sensor types are decided at compile time (embedded robot code is almost always like this) | **CRTP** |
| You choose the type at run time / you want to loop over all sensors in one list | Virtual functions + NVI |
| You are unsure | **Start with virtual functions + NVI.** Measure, then move to CRTP |

**The exercise is written with the virtual function version.** If you start from CRTP, it is hard to see "what is happening",
and you also lose the correspondence with the Java version.

## 3.8 Conclusion for ROS 2 (supplement)

On the ROS 2 side, use the virtual function version in the plain way. There is no situation where the vtable cost matters.

An NVI-like example around rclcpp is `rclcpp_lifecycle::LifecycleNode`.
When you call `configure()` / `activate()`, the skeleton of the transition (state check, state update, notification)
stays with the lifecycle implementation, and only the **replaceable callbacks** `on_configure()` / `on_activate()`
are called.
`on_configure()` is `virtual`, but `configure()` is not a target for replacement.
This is exactly the structure of chapter 3.

`on_init()` / `read()` / `write()` of `hardware_interface::SystemInterface` can also be read as
steps called from the skeleton of the control loop that `ros2_control` owns.

## 3.9 Common pitfalls

| Symptom | Cause |
| --- | --- |
| The derived implementation is not called, and the base implementation runs | The base function has no `virtual`. In C++ it is not virtual unless you write it |
| The derived implementation is not called (I did add `virtual`) | The signature is off and it became a different function. If you add `override`, it becomes an error |
| `error: 'void Imu::boot()' marked 'override', but does not override` | You are trying to replace a non-virtual skeleton. Review the design |
| `error: 'virtual void Sensor::setup()' is private within this context` | You are trying to **call** a `private virtual` from a derived class. If you need to call it, make it `protected` |
| `error: virtual function 'virtual void Imu::setup()' overriding final function` | `final` is on the base. Replacement is stopped on purpose |
| Behavior differs only inside the constructor | 3.4. A virtual call inside a constructor / destructor runs the base implementation |
| It crashes at run time with `pure virtual method called` | You call a **pure** virtual function from a constructor / destructor |
| It leaked when destroyed through a base pointer | There is no `virtual ~SensorReader()` |
| The derived class rewrites the skeleton on its own | You put `virtual` on the skeleton |
| The tests pass but the order of the steps is different | The order of calls inside the skeleton. The exercise tests check this |

## 3.10 Matching exercise

```bash
./drill run dp03
```

In `exercises/dp03_template_method/src/sensor_reader.cpp`, you implement

1. `SensorReader::read_once()`, the template method (initialize → fetch → convert → validate)
2. `SensorReader::validate()`, the default validation (the hook method)
3. `EncoderReader`, count value → angle [deg]
4. `ThermistorReader`, AD value → temperature [degC], rejects values out of range (replaces `validate()`)

You do not edit the header `include/drill/sensor_reader.hpp`.
**The skeleton records the order in which it called each step, and the tests check that order.**
Even if the values are right, the test fails if the procedure is different.

Only `ThermistorReader::validate()` needs to call the base version,
so it is placed as `protected` on the header side. **Why are the other three `private`?**
Check this against change 3 in 3.1.

## 3.11 Summary of this chapter

- Java is **virtual by default**, and C++ is **not virtual unless you write it**. The default is reversed
- Java's `final` method ↔ C++'s "do not write `virtual`". **Do not put `virtual` on the skeleton**
- **In C++, you can override a `private` virtual function.** The access specifier and the ability to override are independent
- Using this, "a public non-virtual skeleton → private virtual steps" is the **NVI idiom**
- Make `protected virtual` only the step where the derived class calls the base version. If it does not call it, leave it `private`
- Write `override` without exception. If you do not, **a signature mismatch silently becomes a different function**
- To stop replacement midway, use `final`. Also put `final` on leaf classes
- **A virtual destructor is required**
- **Do not call virtual functions from constructors / destructors.** The base implementation is called. The opposite of Java
- You can write it with virtual functions on microcontrollers too (no allocation happens). But the cost of the vptr and the vtable is added
- If the replacement is decided at compile time, **CRTP** gives zero vtables. You lose run-time polymorphism

---

Previous: [2. Adapter](02_Adapter.md) / Next: [4. Factory Method](04_FactoryMethod.md)
