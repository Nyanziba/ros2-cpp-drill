# 2. Adapter

> **Corresponds to Yuki's book, Chapter 2.** Please open the two `Banner` / `PrintBanner` samples
> (the one that uses inheritance and the one that uses delegation) next to you.
>
> **Goal of this chapter**: Yuki's book writes Adapter in **two ways: an inheritance version and a delegation version**.
> In Java, you can almost say "both are fine, choose by taste".
> **In C++ it is different.** If you choose the inheritance version, diamond inheritance, name collisions, and slicing all fall on you at once.
> In this chapter, we check with real compile errors **why the delegation version is the only choice in C++**.
> We also see that **`std::stack` and `std::queue` are themselves delegation-version Adapters**.

## 2.1 If you write the Java version in C++ as is

Yuki's book has three characters.

| Role | Meaning | Subject of this chapter |
| --- | --- | --- |
| Target | The form the user wants to see | `MotorActuator` (units are rad/s, rad) |
| Adaptee | Something that already exists and cannot be changed | `LegacyMotorDriver` (units are pulses, encoder counts) |
| Adapter | The one who fills the gap | What we write from now on |

The Java delegation Adapter is written like this (it corresponds to `PrintBanner` in Yuki's book).

```java
public class PrintBanner extends Print {
    private Banner banner;
    public PrintBanner(String string) {
        this.banner = new Banner(string);
    }
    public void printWeak() { banner.showWithParen(); }
}
```

If you move it to C++ in the plain way, it looks like this.

```cpp
class DelegatingMotorAdapter : public MotorActuator
{
public:
  explicit DelegatingMotorAdapter(LegacyMotorDriver driver)
  : driver_(std::move(driver))
  {
  }

  void set_velocity(double rad_per_sec) override;
  void stop() override;
  double position_rad() const override;

private:
  LegacyMotorDriver driver_;
};
```

There are four changes from the Java version. **If you do not make them, you get bugs, or the code gets slow.**

### Change 1: Made Target a pure virtual class instead of an `interface`

In Yuki's book, `Print` appears in both an abstract class version and an `interface` version.
C++ has no `interface` keyword. You use a class that has only pure virtual functions instead.

```cpp
class MotorActuator
{
public:
  virtual ~MotorActuator() = default;
  virtual void set_velocity(double rad_per_sec) = 0;
  virtual void stop() = 0;
  virtual double position_rad() const = 0;
};
```

**A Java `interface` and a C++ pure virtual class are not the same thing.**

| | Java `interface` | C++ pure virtual class |
| --- | --- | --- |
| Data members | Cannot have (constants only) | **Can have** |
| Implemented methods | Only `default` methods | You can write as many as you like |
| Implementing many | You can do as many as you like | It becomes multiple inheritance. The diamond problem appears |
| Destructor | No such concept | **You must write `virtual` explicitly** |

"Can have" is the troublemaker. In C++ you can carelessly add a data member to Target,
and the moment you add it, the weight of multiple inheritance changes. Decide: **keep Target empty**.

### Change 2: Added `virtual ~MotorActuator() = default;`

This is the same story as chapter 1. If you forget it, the derived class is not destroyed when you delete through a base pointer.
The compiler warns you.

```cpp
struct MotorActuator { virtual void stop() = 0; };   // forgot to write a virtual destructor
struct Adapter : MotorActuator { void stop() override {} };

MotorActuator * m = new Adapter{};
delete m;
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// delete_warning.cpp
struct MotorActuator { virtual void stop() = 0; };   // forgot to write a virtual destructor
struct Adapter : MotorActuator { void stop() override {} };

int main()
{
  MotorActuator * m = new Adapter{};
  delete m;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic delete_warning.cpp -o delete_warning
```

</details>

```
delete_warning.cpp: In function ‘int main()’:
delete_warning.cpp:8:3: warning: deleting object of abstract class type ‘MotorActuator’ which has non-virtual destructor will cause undefined behavior [-Wdelete-non-virtual-dtor]
    8 |   delete m;
      |   ^~~~~~~~
```

**It is a warning, not an error.** In an environment where `-Wall` is off, it passes silently.
Every time, do this: if you write even one pure virtual function, write a virtual destructor.

### Change 3: Held the Adaptee **by value**

The Java version has `private Banner banner;`. It holds a reference.
If you write in the same mood in C++, you tend to write this.

```cpp
LegacyMotorDriver & driver_;      // reference. Does not own
LegacyMotorDriver * driver_;      // raw pointer. Unclear who deletes it
LegacyMotorDriver driver_;        // value. The Adapter owns it ← this exercise uses this
```

**All three can be right. What decides it is "whose is the Adaptee?"**

| How to hold it | Meaning | When |
| --- | --- | --- |
| Value | The Adapter owns the Adaptee | The Adaptee is dedicated to this Adapter. **This is the default** |
| Reference | Borrow an Adaptee that lives outside | Something there is only one of in the world, such as a peripheral |
| `unique_ptr` | Owns it, but you need polymorphism / want to hide the type | The Adaptee side also has inheritance |
| `shared_ptr` | Several Adapters share the same Adaptee | Only when you really need sharing |

**Java does not have this choice** (it is always a reference). When you write an Adapter in C++,
the first thing you decide is not the shape of the pattern but **this line**.

### Change 4: Added `const` to `position_rad()`

A function that only reads is made a `const` member function. Java has no such distinction.
When you receive it as `const MotorActuator &`, you cannot call a function that has no `const`.

## 2.2 Who owns it?

The upper-level code is written like this.

```cpp
std::vector<std::unique_ptr<MotorActuator>> motors;
motors.push_back(std::make_unique<DelegatingMotorAdapter>(LegacyMotorDriver{}));
```

`MotorActuator` is an abstract class, so you **cannot hold it by value**. Since you need polymorphism,
it becomes a `vector` of `unique_ptr`. This is the same conclusion as chapter 1.

The lifetime of the Adaptee is already solved when you choose "value" in the table above.
**When the Adapter dies, the Adaptee dies too.** There is nothing more to think about.

Only when you hold it by reference does the same lifetime problem as chapter 1 remain.

```cpp
std::unique_ptr<MotorActuator> make_motor()
{
  LegacyMotorDriver driver;                     // local variable
  return std::make_unique<BorrowingAdapter>(driver);   // driver dies here
}
```

In Java, the GC keeps `driver` alive. In C++, it dies.
**Once you decide to hold it by reference, your job is to write in a comment: "the Adaptee must outlive the Adapter".**

## 2.3 What happens if you write the inheritance version in C++

The inheritance version in Yuki's book looks like this.

```java
public class PrintBanner extends Banner implements Print {
    public void printWeak() { showWithParen(); }
}
```

`extends` (inheritance of implementation) and `implements` (implementing an interface)
**are distinguished by keywords**. C++ has no such distinction. Both are `:`.

```cpp
class InheritingMotorAdapter : public MotorActuator, private LegacyMotorDriver
```

This is **multiple inheritance**. Java's `extends Banner implements Print` is
"one class + an interface", so it is not multiple inheritance, but **in C++ it becomes multiple inheritance.**
Four problems come from this.

### Problem 1: Diamond inheritance

It happens the moment the interface on the Target side becomes two.

```cpp
struct Device { virtual ~Device() = default; virtual void reset() = 0; };
struct Readable : Device { virtual double read() const = 0; };
struct Writable : Device { virtual void write(double v) = 0; };

struct Adapter : Readable, Writable
{
  void reset() override {}
  double read() const override { return 0.0; }
  void write(double) override {}
};

Adapter a;
Device * d = &a;
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// diamond.cpp
struct Device { virtual ~Device() = default; virtual void reset() = 0; };
struct Readable : Device { virtual double read() const = 0; };
struct Writable : Device { virtual void write(double v) = 0; };

struct Adapter : Readable, Writable
{
  void reset() override {}
  double read() const override { return 0.0; }
  void write(double) override {}
};

int main()
{
  Adapter a;
  Device * d = &a;
  (void)d;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic diamond.cpp -o diamond
```

</details>

```
diamond.cpp: In function ‘int main()’:
diamond.cpp:16:16: error: ‘Device’ is an ambiguous base of ‘Adapter’
   16 |   Device * d = &a;
      |                ^~
```

`Adapter` contains **two** `Device`s. You cannot convert to a base pointer.
To fix it, make them `Readable : virtual Device` and `Writable : virtual Device`
(**virtual inheritance**). Then this follows:

- The `Adapter` object gets an extra **virtual base pointer** (the size grows)
- Access to the base takes **one more level of indirection**
- An initialization rule appears: the most derived class calls the constructor of the virtual base directly

**Java has none of these problems.** An interface has no implementation,
so no data is duplicated however many you stack.
This is a typical case where "it works in Java, so it should work in C++" does not hold.

### Problem 2: Name collisions

If Target and Adaptee have the same name, **you get an error the moment you call it**.

```cpp
struct MotorActuator { virtual ~MotorActuator() = default;
                       virtual void set_velocity(double) = 0; void reset() {} };
struct LegacyDriver { void reset() {} };
struct Adapter : MotorActuator, private LegacyDriver
{
  void set_velocity(double) override {}
};

Adapter a;
a.reset();
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// name_clash.cpp
struct MotorActuator { virtual ~MotorActuator() = default;
                       virtual void set_velocity(double) = 0; void reset() {} };
struct LegacyDriver { void reset() {} };
struct Adapter : MotorActuator, private LegacyDriver
{
  void set_velocity(double) override {}
};

int main()
{
  Adapter a;
  a.reset();
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic name_clash.cpp -o name_clash
```

</details>

```
name_clash.cpp: In function ‘int main()’:
name_clash.cpp:13:5: error: request for member ‘reset’ is ambiguous
   13 |   a.reset();
      |     ^~~~~
name_clash.cpp:4:28: note: candidates are: ‘void LegacyDriver::reset()’
    4 | struct LegacyDriver { void reset() {} };
      |                            ^~~~~
name_clash.cpp:3:68: note:                 ‘void MotorActuator::reset()’
    3 |                        virtual void set_velocity(double) = 0; void reset() {} };
      |                                                                    ^~~~~
```

The Adaptee is assumed to be **unchangeable**. If the names collide, the only way is to write
something like `using LegacyDriver::reset;` on the Adapter side to remove the ambiguity.
**Every time the Adaptee gains a member, our side may break.** This is the maintenance cost of the inheritance version.

Worse, there are **collisions that do not produce an error**.

```cpp
struct MotorActuator { virtual void reset() = 0; /* ... */ };
struct LegacyDriver { void reset() {} };

struct Adapter : MotorActuator, private LegacyDriver
{
  void reset() override { reset(); }   // I meant to call the Adaptee, but it calls itself
};
```

An unqualified `reset()` resolves to **itself**. It is infinite recursion. It compiles.
The correct way is to write `LegacyDriver::reset();`. With the delegation version, you write `driver_.reset();`,
so **you cannot make this mistake in the first place.**

### Problem 3: With `public` inheritance, slicing goes through

If you think "`private` inheritance is a hassle, so I'll use `public`", this happens.

```cpp
struct LegacyDriver { int pulse = 0; void setPulse(int p) { pulse = p; } };
struct PublicAdapter : public LegacyDriver { double gain = 2.0; };
struct PrivateAdapter : private LegacyDriver { double gain = 2.0; };

void tune(LegacyDriver driver) { driver.setPulse(0); }   // pass by value

PublicAdapter pub;
tune(pub);            // compiles. gain is sliced off

PrivateAdapter priv;
tune(priv);           // does not compile
```

Only the `private` version gives an error.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// slicing.cpp
struct LegacyDriver { int pulse = 0; void setPulse(int p) { pulse = p; } };
struct PublicAdapter : public LegacyDriver { double gain = 2.0; };
struct PrivateAdapter : private LegacyDriver { double gain = 2.0; };

void tune(LegacyDriver driver) { driver.setPulse(0); }   // pass by value

int main()
{
  PublicAdapter pub;
  tune(pub);            // compiles. gain is sliced off

  PrivateAdapter priv;
  tune(priv);           // does not compile
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic slicing.cpp -o slicing
```

</details>

```
slicing.cpp: In function ‘int main()’:
slicing.cpp:14:7: error: ‘LegacyDriver’ is an inaccessible base of ‘PrivateAdapter’
   14 |   tune(priv);           // does not compile
      |   ~~~~^~~~~~
slicing.cpp:6:24: note:   initializing argument 1 of ‘void tune(LegacyDriver)’
    6 | void tune(LegacyDriver driver) { driver.setPulse(0); }   // pass by value
      |           ~~~~~~~~~~~~~^~~~~~
```

**The `public` version passes silently. There is not even a warning.**
A copy with `gain` sliced off is made, and later changes do not reach the original.
Java has no value copy, so **you can never notice slicing just by reading the Java version**. It is a real danger.

And the `public` inheritance version has a more direct problem.

```cpp
MotorActuator * m = get_motor();
static_cast<PublicAdapter *>(m)->setPulse(999);   // you can hit the raw units directly
```

**The purpose of Adapter, hiding the units, is broken by public inheritance.**

### Problem 4: The Adaptee has no virtual destructor

`LegacyMotorDriver` is three-year-old code. It has no virtual destructor.
If you inherit publicly, you get a type that "can be assigned to `LegacyMotorDriver *`, but breaks if you delete through it".
With `private` inheritance, you cannot assign it in the first place, so no accident happens.

### Conclusion: use only the delegation version

| | Inheritance version (private) | Delegation version |
| --- | --- | --- |
| Diamond inheritance | Happens when Target becomes two | Does not happen |
| Name collisions | May break when the Adaptee changes | Does not happen |
| Calling the Adaptee | The accident of calling yourself when unqualified | Explicit with `driver_.` |
| Swapping the Adaptee at run time | Not possible | Possible by swapping the member |
| Combining two Adaptees in one Adapter | Not possible (inheriting both is hell) | Just have two members |
| Using the Adaptee's protected members | **You can use them** | You cannot use them |
| Object size | Empty base optimization may apply | The size of the Adaptee as is |

**The inheritance version wins only in the last two rows.** And
"I want to use the Adaptee's `protected`" **rarely happens with someone else's library that you cannot change**.

> **When in doubt, delegate.** Choose inheritance only when you find that you need a `protected` member.

### `private` inheritance, a choice specific to C++

Java has no `private extends`. `private` inheritance in C++ expresses
**not "is-a" but "is-implemented-in-terms-of"**.

```cpp
class InheritingMotorAdapter : public MotorActuator, private LegacyMotorDriver
```

Read this as: "it **is a** `MotorActuator`, and it **is implemented using** `LegacyMotorDriver`".
`private` inheritance has one small trick that delegation does not have: **selective re-exposure**.

```cpp
using LegacyMotorDriver::getPulse;    // expose only this
```

The exercise uses this to peek inside from the tests.
Still, with the delegation version you can do the same by "writing a function that forwards that one line".
**`private` inheritance has hardly any value beyond "slightly less to write".**

## 2.4 Does the standard library already have the same thing?

Yes. And it is **something you use every day**.

```cpp
std::stack<int>    // the default underlying container is std::deque<int>
std::queue<int>    // the default underlying container is std::deque<int>
```

`std::stack` does not hold data by itself. It **holds a `std::deque` as a member**,
and just exposes `push_back` / `pop_back` / `back` renamed as `push` / `pop` / `top`.
The standard also calls it a **container adaptor**.

| Role in Adapter | In `std::stack` |
| --- | --- |
| Target | The "stack" interface (`push` / `pop` / `top`) |
| Adaptee | `std::deque` (default) |
| Adapter | `std::stack` itself |

**And `std::stack` is the delegation version.** It does not inherit from `std::deque`.
The reason the standard library chose delegation is the same as what we have seen in this chapter so far.

With the second template argument of `std::stack`, you can swap the whole Adaptee.

```cpp
std::stack<int, std::vector<int>> vector_stack;   // use a vector inside
```

**You can swap the Adaptee because it is delegation.** If it inherited, the type would change.

And one more point. An Adapter is also a tool that **narrows the interface**.

```cpp
std::deque<int> raw;
raw.push_front(0);          // deque has it

std::stack<int> s;
// s.push_front(0);         // stack does not have it
// for (int v : s) { }      // you cannot traverse it either
```

`std::stack` **can do less** than `std::deque`. That is its job.
`MotorActuator` in the exercise is the same: it **deliberately removes** the interface "hit it with pulses".

## 2.5 Try it yourself

Before you solve the exercise, compile this one file and **predict the output** before you run it.

```cpp
#include <deque>
#include <iostream>
#include <stack>
#include <vector>

int main()
{
  // std::stack is a delegation-version Adapter that only "holds one container and narrows the interface".
  // The default underlying container is std::deque.
  std::stack<int> default_stack;

  // Even if you swap the underlying container, the interface seen from outside is exactly the same.
  std::stack<int, std::vector<int>> vector_stack;

  for (int i = 1; i <= 3; ++i) {
    default_stack.push(i);
    vector_stack.push(i);
  }

  std::cout << default_stack.top() << " " << vector_stack.top() << "\n";
  std::cout << default_stack.size() << " " << vector_stack.size() << "\n";

  // If you use the Adaptee as it is, the interface is not narrowed.
  std::deque<int> raw;
  raw.push_back(1);
  raw.push_front(0);              // an operation that stack does not have
  std::cout << raw.front() << " " << raw.back() << "\n";

  // What std::stack cannot do: traversal
  // for (int v : default_stack) { (void)v; }   // ← if you uncomment this, you get an error
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// try_range.cpp
#include <deque>
#include <iostream>
#include <stack>
#include <vector>

int main()
{
  // std::stack is a delegation-version Adapter that only "holds one container and narrows the interface".
  // The default underlying container is std::deque.
  std::stack<int> default_stack;

  // Even if you swap the underlying container, the interface seen from outside is exactly the same.
  std::stack<int, std::vector<int>> vector_stack;

  for (int i = 1; i <= 3; ++i) {
    default_stack.push(i);
    vector_stack.push(i);
  }

  std::cout << default_stack.top() << " " << vector_stack.top() << "\n";
  std::cout << default_stack.size() << " " << vector_stack.size() << "\n";

  // If you use the Adaptee as it is, the interface is not narrowed.
  std::deque<int> raw;
  raw.push_back(1);
  raw.push_front(0);              // an operation that stack does not have
  std::cout << raw.front() << " " << raw.back() << "\n";

  // What std::stack cannot do: traversal
  for (int v : default_stack) { (void)v; }   // ← if you uncomment this, you get an error
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try_range.cpp -o try_range
```

</details>

<details>
<summary>Predict: what are the three lines of output? And what happens if you uncomment the last line?</summary>

```
3 3
3 3
0 1
```

The point is that the first and second lines are **the same**. Whether the underlying container is `std::deque` or
`std::vector`, **the interface seen from outside does not change by a single character**.
Even if you swap the Adaptee, Target does not change. This is what Adapter is.

If you uncomment the last line, this happens.

```
try_range.cpp: In function ‘int main()’:
try_range.cpp:31:16: error: no matching function for call to ‘begin(std::stack<int>&)’
   31 |   for (int v : default_stack) { (void)v; }   // ← if you uncomment this, you get an error
      |                ^~~~~~~~~~~~~
...
```

`std::deque` has `begin()`. `std::stack` **does not expose it**.
You can see that an Adapter is a tool that widens the interface, and also a tool that **narrows** it.
</details>

## 2.6 Conclusion for microcontrollers

**The inheritance version is out of the question, and the delegation version is not used as it is, either.** There are three reasons.

1. If you make `MotorActuator` with virtual functions, **the vtable goes into ROM and a vtable pointer goes into each object**
2. If you hold it with `std::unique_ptr<MotorActuator>`, you need a **heap allocation**
3. The calls are not inlined

You can measure the cost of the vtable pointer. On a local arm64 machine,

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// sizeof_adapter.cpp
#include <cstdint>
#include <cstdio>

// Existing raw driver (cannot be changed)
class LegacyMotorDriver
{
public:
  void setPulse(int pulse) { pulse_ = pulse; }
  int getPulse() const { return pulse_; }
  void stopAll() { pulse_ = 0; }
  std::int32_t readEncoderRaw() const { return encoder_raw_; }

private:
  int pulse_ = 0;
  std::int32_t encoder_raw_ = 0;
};

class MotorActuator
{
public:
  virtual ~MotorActuator() = default;
  virtual void set_velocity(double rad_per_sec) = 0;
  virtual void stop() = 0;
  virtual double position_rad() const = 0;
};

// Delegation Adapter with virtual functions. It holds the raw driver by value
class DelegatingMotorAdapter : public MotorActuator
{
public:
  void set_velocity(double rad_per_sec) override
  {
    driver_.setPulse(static_cast<int>(rad_per_sec * 100.0));
  }
  void stop() override { driver_.stopAll(); }
  double position_rad() const override { return driver_.readEncoderRaw() / 4096.0; }

private:
  LegacyMotorDriver driver_;
};

// Delegation-version Adapter with no virtual functions, no inheritance, and no allocation.
// Align not by "the type MotorActuator" but by "the name set_velocity".
template <typename Driver>
class MotorAdapter
{
public:
  explicit MotorAdapter(Driver & driver) : driver_(driver) {}

  void set_velocity(std::int32_t milli_rad_per_sec)
  {
    driver_.setPulse(static_cast<int>(milli_rad_per_sec / 10));
  }

  void stop() { driver_.stopAll(); }

private:
  Driver & driver_;              // does not own. The caller guarantees the lifetime
};

int main()
{
  std::printf("LegacyMotorDriver                  : %zu\n", sizeof(LegacyMotorDriver));
  std::printf("DelegatingMotorAdapter (virtual)   : %zu\n", sizeof(DelegatingMotorAdapter));
  std::printf("MotorAdapter<LegacyMotorDriver>    : %zu\n", sizeof(MotorAdapter<LegacyMotorDriver>));
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic sizeof_adapter.cpp -o sizeof_adapter && ./sizeof_adapter
```

</details>

| Type | `sizeof` |
| --- | --- |
| `LegacyMotorDriver` (`int` + `int32_t`) | 8 |
| Delegation Adapter with virtual functions | **16** |
| Template version of the delegation Adapter | 8 |

It adds **8 bytes per object**. With 4 motors, that is 32 bytes.
On a microcontroller with 20 KB of RAM, doing this in 20 places starts to matter.

Instead, **align by name, not by type**. This is a template delegation Adapter.

```cpp
#include <cstdint>

// Existing raw driver (cannot be changed)
class LegacyMotorDriver
{
public:
  void setPulse(int pulse) { pulse_ = pulse; }
  int getPulse() const { return pulse_; }
  void stopAll() { pulse_ = 0; }
  std::int32_t readEncoderRaw() const { return encoder_raw_; }

private:
  int pulse_ = 0;
  std::int32_t encoder_raw_ = 0;
};

// Delegation-version Adapter with no virtual functions, no inheritance, and no allocation.
// Align not by "the type MotorActuator" but by "the name set_velocity".
template <typename Driver>
class MotorAdapter
{
public:
  explicit MotorAdapter(Driver & driver) : driver_(driver) {}

  void set_velocity(std::int32_t milli_rad_per_sec)
  {
    driver_.setPulse(static_cast<int>(milli_rad_per_sec / 10));
  }

  void stop() { driver_.stopAll(); }

private:
  Driver & driver_;              // does not own. The caller guarantees the lifetime
};

// The user side is also a template. All calls are inlined
template <typename Motor>
void drive_forward(Motor & motor)
{
  motor.set_velocity(1000);
}

LegacyMotorDriver g_driver;        // static storage. Constructed only once at startup

int main()
{
  MotorAdapter<LegacyMotorDriver> motor{g_driver};
  drive_forward(motor);
  motor.stop();
  return g_driver.getPulse();
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti micro.cpp -o micro
```

Against the microcontroller constraints, this is how it works out.

| Constraint | What happens with this way of writing |
| --- | --- |
| Dynamic allocation forbidden as a rule | There is no `new` and no `make_unique`. The Adaptee is in static storage and the Adapter is on the stack |
| `-fno-exceptions` | Nothing is `throw`n. Errors are returned as return values |
| `-fno-rtti` | There is no `dynamic_cast` and no `typeid`. There is no inheritance in the first place |
| vtable cost | Zero virtual functions. No vtable and no vtable pointer appear |

**The only price is that "you cannot swap the Adaptee at run time"**.
On a microcontroller, the type of the connected motor driver does not change at run time.
**What is decided at compile time, decide at compile time.**

There is one more thing to note about unit conversion. The code above does not use `double`
and takes `milli_rad_per_sec` (`std::int32_t`).
On a microcontroller without an FPU, `double` arithmetic is software emulation,
and it takes **hundreds of cycles inside an interrupt handler**.
**An Adapter is the place where units change, so this tends to be the entrance of floating point.**
Take fixed-point numbers, or at least use `float`.

If you really need to swap at run time (for example, to replace it with a dummy driver for tests),
use the virtual function version, and make it **point to an object in static storage** instead of using `unique_ptr`.

## 2.7 Conclusion for ROS 2 (supplement)

On the ROS 2 side, you may write the delegation version from 2.1 to 2.4 as it is.
There are almost no situations where allocation or virtual functions matter as a cost.

Where Adapter really works around rclcpp is **the boundary between the hardware layer and the message layer**.

```cpp
// The raw driver knows pulses. The node only needs to know geometry_msgs
class MotorAdapter : public MotorActuator { /* ... */ };
```

If the node callback touches `LegacyMotorDriver` directly,
**on the day you swap the driver, all the node tests have to be rewritten**.
If you put one Adapter in between, swapping only affects the Adapter.

Note that `rclcpp::TypeAdapter` (Humble and later) has Adapter in its name, but
it is a mechanism that "registers conversion between your own type and a ROS message type by template specialization",
and **it is a different thing from the GoF Adapter**. Do not confuse them.

## 2.8 Common pitfalls

| Symptom | Cause |
| --- | --- |
| `error: 'Device' is an ambiguous base of 'Adapter'` | Diamond inheritance. The Target-side interface has become two. Change to delegation |
| `error: request for member 'reset' is ambiguous` | Name collision between Target and Adaptee. It does not happen with delegation |
| In the inheritance version, a call I meant for the Adaptee recurses infinitely | The unqualified call resolves to itself. Write `LegacyDriver::reset()` |
| Members of the Adapter disappeared after passing it to a function | Slicing with public inheritance + pass by value. Use `private` inheritance or delegation |
| `warning: deleting object of abstract class type ... which has non-virtual destructor` | Target has no virtual destructor |
| It crashes when I use the return value of a function that returns an Adapter | The Adaptee is held by reference. Hold it by value, or promise the lifetime |
| `error: 'LegacyDriver' is an inaccessible base of 'PrivateAdapter'` | You cannot convert to the base from outside with `private` inheritance. **This is correct behavior** |
| The unit conversion gives a different value for each call | The conversion factor is scattered outside the Adapter. Put it in one place as a `constexpr` on the Target side |

## 2.9 Matching exercise

```bash
./drill run dp02
```

In `exercises/dp02_adapter/src/motor_adapter.cpp`, you implement two classes:

1. `DelegatingMotorAdapter`, the delegation version that **holds the raw driver as a member**
2. `InheritingMotorAdapter`, the inheritance version that **inherits the raw driver with `private` inheritance**

The tests check that **both behave exactly the same**, and that
you can mix them in a `vector` of `std::unique_ptr<MotorActuator>` and treat them polymorphically.

After you finish, change `private` in `InheritingMotorAdapter` to `public` and check
what happens (and what does **not** happen). **That is the answer check for 2.3.**

## 2.10 Summary of this chapter

- Java's `extends` + `implements` become **multiple inheritance in C++**
- The inheritance version brings diamond inheritance, name collisions, slicing, and a missing virtual destructor all at once
- **Use only the delegation version.** Inheritance wins only when you need the Adaptee's `protected`
- `private` inheritance is "is-implemented-in-terms-of". It is a **choice Java does not have**, but delegation is enough
- A Java `interface` and a C++ pure virtual class are different things. **Keep Target empty**
- Before you write an Adapter, what you decide is not the shape but **whether to hold the Adaptee by value / reference / `unique_ptr`**
- **`std::stack` and `std::queue` are delegation-version Adapters**. You can swap the underlying container because it is delegation
- An Adapter not only widens the interface but also **narrows** it. `std::stack` has no `begin()`
- On microcontrollers, drop virtual functions and use a template delegation. You save **an 8-byte vtable pointer per object**

---

Previous: [1. Iterator](01_Iterator.md) / Next: [3. Template Method](03_TemplateMethod.md)
