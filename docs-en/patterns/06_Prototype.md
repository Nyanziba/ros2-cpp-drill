# 6. Prototype

> **Corresponds to Chapter 6 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Open the `Product` interface, `Manager`, and `UnderlinePen` / `MessageBox` from the book.
>
> **Goal of this chapter**: Java's `clone()` and `Cloneable` were needed **because Java has no language feature for copying.**
> C++ has had the copy constructor from the start. So half of the answer to Chapter 6 is "**you do not need this pattern**".
> Only the other half, duplicating an object when you hold nothing but a `std::unique_ptr<Base>`, is the real Prototype.
> There you hit a wall that only C++ has: **covariant return types do not work with `unique_ptr`.**

## 6.1 First ask whether the copy constructor is enough

The `Product` in Hiroshi Yuki's book looks like this.

```java
public interface Product extends Cloneable {
    public abstract void use(String s);
    public abstract Product createCopy();
}
```

The body of `createCopy()` only calls `clone()`.
First, understand **why Java needs this**. Java only has

```java
UnderlinePen copy = pen;      // Only points to the same object. Not a copy
```

**All variables are references**, so there is no language feature to make a "copy of the contents".
So it is added later with the protected method `Object.clone()` and the `Cloneable` marker interface.

C++ is different.

```cpp
PulseTrain copy = original;   // It is copied. The contents are copied
```

**That is all.** The copy constructor is a language feature,
and the compiler makes it for you even if you do not write it.

So when you bring Chapter 6 to C++, the first decision is this.

> **Do you know the type of the object you want to copy at that place?**
> If you do, you do not need `clone()`. Use the copy constructor.

If you register `PulseTrain` in a `Manager` and take it out as a `PulseTrain`, Prototype is not needed.
Put it in a `std::map<std::string, PulseTrain>`, take it out, and copy it.

## 6.2 You need clone() only for polymorphism

You need it in this form.

```cpp
std::unique_ptr<Waveform> original = load_from_config();   // We do not know what it holds
std::unique_ptr<Waveform> copy = /* ??? */;
```

The caller does not know whether the real object of `original` is a `PulseTrain` or a `SineSweep`.
The copy constructor is **chosen by the static type**, so we cannot use it here.

```cpp
Waveform copy = *original;    // Waveform is an abstract class. You cannot even write this
```

What we need is to **let the object itself, which knows what it is, make the copy.**
That is `clone()` as a virtual function. **This is the whole essence of Prototype.**

This table is enough to decide.

| Situation | What to use |
| --- | --- |
| You know the type | The copy constructor. Do not write `clone()` |
| You only have a `unique_ptr<Base>` / only a reference to the base | A virtual `clone()` |
| The candidate types are finite and can be listed | `std::variant`. Copying is already polymorphic (6.6) |

## 6.3 Covariant return types do not work with `unique_ptr`

C++ has **covariant return types**.
If the base has `Base * f()`, the derived class can override it with `Derived * f()`.

```cpp
class Waveform
{
public:
  virtual Waveform * clone() const = 0;
};

class SineSweep : public Waveform
{
public:
  SineSweep * clone() const override;      // OK. It may return a type narrower than Waveform *
};
```

What if we use `std::unique_ptr` to write the ownership in the type? **It does not compile.**

```cpp
class Waveform
{
public:
  virtual std::unique_ptr<Waveform> clone() const = 0;
};

class SineSweep : public Waveform
{
public:
  std::unique_ptr<SineSweep> clone() const override    // here
  {
    return std::make_unique<SineSweep>(*this);
  }
};
```

If you really compile it, you get this.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// covariant_clone.cpp
#include <memory>

class Waveform
{
public:
  virtual std::unique_ptr<Waveform> clone() const = 0;
};

class SineSweep : public Waveform
{
public:
  std::unique_ptr<SineSweep> clone() const override    // here
  {
    return std::make_unique<SineSweep>(*this);
  }
};

int main()
{
  SineSweep sweep;
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic covariant_clone.cpp -o covariant_clone
```

</details>

```
covariant_clone.cpp:13:30: error: invalid covariant return type for ‘virtual std::unique_ptr<SineSweep> SineSweep::clone() const’
   13 |   std::unique_ptr<SineSweep> clone() const override    // here
      |                              ^~~~~
covariant_clone.cpp:7:37: note: overridden function is ‘virtual std::unique_ptr<Waveform> Waveform::clone() const’
    7 |   virtual std::unique_ptr<Waveform> clone() const = 0;
      |                                     ^~~~~
```

**Covariant return types are allowed only for pointers and references.**
This is because `std::unique_ptr<Derived>` is not a derived class of `std::unique_ptr<Base>`.
Even if the template arguments are related by inheritance, the instantiations of the template are unrelated.
It is the same as `std::vector<Derived>` not being derived from `std::vector<Base>`.

### Workaround: combine it with NVI

To get both "I want it covariant" and "I want to return `unique_ptr`", **split it into two steps.**
It is the same shape as NVI that we did in Chapter 3 (public non-virtual → private virtual).

```cpp
class Waveform
{
public:
  virtual ~Waveform() = default;

  // public. Non-virtual. The caller sees only this
  std::unique_ptr<Waveform> clone() const
  {
    return std::unique_ptr<Waveform>(do_clone());
  }

private:
  // private. Virtual. It returns a raw pointer, so it can be covariant
  virtual Waveform * do_clone() const = 0;
};

class SineSweep : public Waveform
{
private:
  SineSweep * do_clone() const override      // Covariant. It can return SineSweep *
  {
    return new SineSweep(*this);
  }
};
```

What you get:

- The caller receives a `std::unique_ptr`. **The type says who frees it**
- The derived class returns a raw pointer. **Covariance works**, so if you receive
  `SineSweep::do_clone()` as a `SineSweep *`, you can use it without a cast
- A bare `new` appears in only one line, in `do_clone()`. **`clone()` wraps it immediately**,
  so nothing leaks even if an exception is thrown

"Just write a virtual function that returns `unique_ptr`" also works (you only give up covariance).
If you do not need covariance, that is shorter and safer. **You need covariance when you call `clone()`
in a context where the derived type is known, and you want to receive the derived type without a cast.**
In this exercise, we write the NVI version.

## 6.4 Who owns it

The `createCopy()` in Hiroshi Yuki's book just returns a `Product`. The GC collects it.
If you write `Waveform * clone() const;` in C++, **the type does not say whether the caller should `delete` it**.
With the same judgment as in the Iterator chapter, we return a `std::unique_ptr`.

```cpp
std::unique_ptr<Waveform> clone() const;
```

The `Manager` in Hiroshi Yuki's book collects the prototypes in a `HashMap`. In C++ it looks like this.

```cpp
std::vector<std::unique_ptr<Waveform>> waveforms_;
```

And "duplicate the whole library" just runs `clone()` without knowing the real types of the elements.

```cpp
WaveformLibrary WaveformLibrary::duplicate() const
{
  WaveformLibrary copy;
  for (const std::unique_ptr<Waveform> & waveform : waveforms_) {
    copy.waveforms_.push_back(waveform->clone());
  }
  return copy;
}
```

**This function is the very reason Prototype exists.** You need no `dynamic_cast` and no chain of `if`.

## 6.5 Dangers only C++ has

### Danger 1: Slicing

This accident never happens in Java.

```cpp
SineSweep sweep{10.0, 200.0, 8};
Waveform sliced = sweep;        // The SineSweep part is cut off
```

`sliced` has the size of just one `Waveform`. **The derived state and the vtable are both lost.**
We will look at the real output in 6.7.

There are two ways to stop it.

| Method | Effect |
| --- | --- |
| Put a pure virtual function in the base (make it an abstract class) | It cannot be taken by value. **This exercise does this** |
| Make the copy constructor of the base `protected` | Derived classes can call it, but slicing from outside is stopped |

The `Waveform` in this exercise does both. If you really write `Waveform sliced = pulse;`,

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// abstract_slicing.cpp
class Waveform
{
public:
  virtual ~Waveform() = default;
  virtual double sample(double time) const = 0;   // pure virtual function = abstract class
};

class PulseTrain : public Waveform
{
public:
  double sample(double time) const override { return time < 0.5 ? 1.0 : 0.0; }
};

int main()
{
  PulseTrain pulse;
  Waveform sliced = pulse;
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic abstract_slicing.cpp -o abstract_slicing
```

</details>

```
abstract_slicing.cpp: In function ‘int main()’:
abstract_slicing.cpp:18:21: error: cannot allocate an object of abstract type ‘Waveform’
   18 |   Waveform sliced = pulse;
      |                     ^~~~~
abstract_slicing.cpp:2:7: note:   because the following virtual functions are pure within ‘Waveform’:
    2 | class Waveform
      |       ^~~~~~~~
abstract_slicing.cpp:6:18: note:     ‘virtual double Waveform::sample(double) const’
    6 |   virtual double sample(double time) const = 0;   // pure virtual function = abstract class
      |                  ^~~~~~
abstract_slicing.cpp:18:12: error: cannot declare variable ‘sliced’ to be of abstract type ‘Waveform’
   18 |   Waveform sliced = pulse;
      |            ^~~~~~
```

**Assignment to the base** is also the same accident. Stop it with `Waveform & operator=(const Waveform &) = delete;`.

```cpp
Waveform & a = pulse;
Waveform & b = sweep;
a = b;                          // The contents get mixed. If it is deleted, this does not compile
```

### Danger 2: Shallow copy and double free

Java's `Object.clone()` is a **shallow copy** by default. It copies the field references as they are.
The implicit copy constructor of C++ is the same: it is a **member-wise copy**.
If you have a raw pointer member, this happens.

```cpp
class PulseTrain
{
public:
  explicit PulseTrain(std::size_t length) : pattern_(new double[length]()), length_(length) {}
  ~PulseTrain() { delete[] pattern_; }

private:
  double * pattern_;
  std::size_t length_;
};

PulseTrain a{4};
PulseTrain b = a;      // The "value" of pattern_ is copied = two objects point to the same array
                       // Both destructors call delete[] = double free
```

If you run it on your machine, **it compiles without any warning about the double free, and crashes at run time** (SIGABRT, exit code 134 with g++ on Linux; SIGTRAP, exit code 133 with Apple clang on macOS.
Apple clang prints one separate warning, about the unused `length_`).
This is a way of breaking that cannot happen in Java.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// double_free.cpp
#include <cstddef>

class PulseTrain
{
public:
  explicit PulseTrain(std::size_t length) : pattern_(new double[length]()), length_(length) {}
  ~PulseTrain() { delete[] pattern_; }

private:
  double * pattern_;
  std::size_t length_;
};

int main()
{
  PulseTrain a{4};
  PulseTrain b = a;      // The "value" of pattern_ is copied = two objects point to the same array
                         // Both destructors call delete[] = double free
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic double_free.cpp -o double_free
./double_free; echo "exit code: $?"
```

</details>


A `std::unique_ptr` member **makes the language stop this.**

```cpp
std::unique_ptr<double[]> pattern_;
```

A `unique_ptr` cannot be copied, so **the implicit copy constructor of this type is automatically `delete`d**.
If you try to write a shallow copy, it becomes a compile error, and if you want a deep copy, you have to write it yourself.

```cpp
PulseTrain::PulseTrain(const PulseTrain & other)
: Waveform(other),
  label_(other.label_),
  pattern_(new double[other.length_]()),
  length_(other.length_)
{
  std::copy(other.pattern_.get(), other.pattern_.get() + other.length_, pattern_.get());
}
```

Do not forget the copy constructor of the base, `Waveform(other)`.
If you do not write it, the base part is **default-constructed**. No warning is shown.

### Danger 3: "If you have a `vector<unique_ptr>`, copying is automatically prohibited" is a lie

You will really step on this.

```cpp
class WaveformLibrary
{
private:
  std::vector<std::unique_ptr<Waveform>> waveforms_;
};

static_assert(!std::is_copy_constructible<WaveformLibrary>::value, "…");   // fails
```

`std::vector` only **declares** the copy constructor. That the contents cannot be copied
becomes an error only when you actually try to instantiate the copy.
So `std::is_copy_constructible` returns **`true`**.

If you want to **state "no copy" as a property of the type**, you have to write it yourself.

```cpp
WaveformLibrary(const WaveformLibrary &) = delete;
WaveformLibrary & operator=(const WaveformLibrary &) = delete;
WaveformLibrary(WaveformLibrary &&) = default;
WaveformLibrary & operator=(WaveformLibrary &&) = default;
```

### Rule of Zero / Rule of Five

It is not an accident that we wrote four lines above. C++ has five special member functions.

destructor / copy constructor / copy assignment / move constructor / move assignment

**Rule of Five**: If you write even one of them yourself, **write your intent for the other four too**
(either `= default`, `= delete`, or a hand-written one).
If you make the copy user-declared, **the move is no longer generated implicitly**,
so if you stay silent, the move disappears.

**Rule of Zero**: The best is to write none of them.
If the members are only types that **clean up after themselves**, such as `std::string` / `std::vector` / `std::unique_ptr`,
all five are generated correctly for you.

In this exercise:

- `SineSweep` has only value members → **Rule of Zero**. Write none of the five
- `PulseTrain` has a `unique_ptr<double[]>` and needs a deep copy → write the copy constructor by hand, and
  `= delete` the copy assignment (not needed in this exercise)
- `Waveform` is a base → virtual destructor, `protected` copy constructor, `= delete` copy assignment

## 6.6 Does the standard library or the language already have the same thing?

**The copy constructor itself is Prototype.** It is built into the language from the start.
As we saw in 6.1, Java needed `clone()` because it did not have this feature.

For polymorphic copying only, the standard library has no ready-made tool. You write `clone()` yourself.
But there is a way in the standard that **avoids polymorphism**.

```cpp
using Waveform = std::variant<PulseTrain, SineSweep>;

Waveform original = SineSweep{10.0, 200.0, 8};
Waveform copy = original;        // This copies correctly. No clone() needed
```

You can use `std::variant` when "the candidate types are finite and can be listed at compile time".
**Then copying is already polymorphic.** You need no `clone()`, no vtable, and no heap allocation.
If the kinds of waveforms are fixed at compile time, this is the shortest answer.
(`std::variant` and `std::visit` are covered properly in Chapter 13, Visitor.)

`std::shared_ptr` does not copy. It **shares**. Do not mix them up.

```cpp
std::shared_ptr<Waveform> b = a;   // Not a copy. Two owners just hold the same object
```

This is the closest to Java's `=`. If you write it "thinking in Java", you get this,
so make it clear every time whether you want to **copy or share**.

## 6.7 Try it yourself

We put slicing and `clone()` side by side. **Predict the output** before you run it.

```cpp
#include <iostream>
#include <memory>
#include <string>

class Waveform
{
public:
  virtual ~Waveform() = default;
  virtual std::string name() const { return "Waveform"; }

  // A public non-virtual clone. The body is left to the private virtual do_clone (NVI)
  std::unique_ptr<Waveform> clone() const { return std::unique_ptr<Waveform>(do_clone()); }

private:
  virtual Waveform * do_clone() const { return new Waveform(*this); }
};

class SineSweep : public Waveform
{
public:
  std::string name() const override { return "SineSweep"; }

private:
  // With a raw pointer, a covariant return type is allowed
  SineSweep * do_clone() const override { return new SineSweep(*this); }
};

int main()
{
  SineSweep sweep;

  // (1) Take it by value = slicing
  Waveform sliced = sweep;
  std::cout << "sliced: " << sliced.name() << "\n";

  // (2) Through clone() = it is copied with the derived type kept
  std::unique_ptr<Waveform> original = std::make_unique<SineSweep>();
  std::unique_ptr<Waveform> copy = original->clone();
  std::cout << "clone : " << copy->name() << "\n";
  std::cout << "same address? " << (original.get() == copy.get() ? "yes" : "no") << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: What does <code>sliced.name()</code> return? Is there a warning?</summary>

```
sliced: Waveform
clone : SineSweep
same address? no
```

Although `sliced` was made from a `SineSweep`, it returns `"Waveform"`.
**The derived part was cut off completely.** And
with `-Wall -Wextra -Wpedantic`, **not a single warning appears.** An accident that cannot happen in Java passes silently.

If you change `do_clone()` to `std::unique_ptr<SineSweep>` and make `clone()` itself virtual,
you can reproduce the compile error of 6.3. Try it.

If you add one pure virtual function to `Waveform`, the line `Waveform sliced = sweep;` becomes
a compile error. **An abstract class stops slicing with the type.**
</details>

## 6.8 Conclusion for microcontrollers

If you write `clone()` in the obvious way, **it uses the heap.** If you call it in a loop, the heap gets fragmented.
Also, with `-fno-rtti` you cannot use `dynamic_cast`,
so the style "check the type after copying" is also closed.

Choose in this order.

**First choice: do not use Prototype at all. Copy by value.**

```cpp
// A preset of control parameters. No polymorphism needed
struct GainPreset
{
  float kp;
  float ki;
  float kd;
};

// A template in ROM
constexpr GainPreset kArmPreset{2.0F, 0.1F, 0.05F};

void start_arm_control()
{
  GainPreset preset = kArmPreset;     // This is the copy. Zero allocation
  preset.kp *= 0.8F;                  // Adjust on site
  apply(preset);
}
```

A `constexpr` template is **placed in ROM and uses not one byte of RAM**.
This already meets the goal of Prototype: "register a prototype, copy it, and use it".

**Second choice: if you need polymorphism, construct in a fixed pool.**

```cpp
// Make the copy in a pre-allocated area, without using the heap
class Waveform
{
public:
  virtual ~Waveform() = default;

  /// Constructs a copy of itself in buffer and returns its start.
  /// Returns nullptr if the capacity is not enough (no throw, because of -fno-exceptions).
  virtual Waveform * clone_into(void * buffer, std::size_t capacity) const = 0;

  virtual std::size_t object_size() const = 0;
};

class SineSweep : public Waveform
{
public:
  Waveform * clone_into(void * buffer, std::size_t capacity) const override
  {
    if (capacity < sizeof(SineSweep)) {
      return nullptr;
    }
    return new (buffer) SineSweep(*this);   // placement new. It does not allocate
  }

  std::size_t object_size() const override { return sizeof(SineSweep); }
};
```

Placement new **does not allocate**. It only constructs in the area the caller prepared.
In exchange, you **must not `delete`** it. You call the destructor explicitly.

```cpp
alignas(SineSweep) unsigned char pool[64];

Waveform * copy = prototype.clone_into(pool, sizeof(pool));
if (copy != nullptr) {
  // use it
  copy->~Waveform();      // Call the destructor directly, not delete
}
```

If you forget `alignas`, some MCUs crash with an alignment fault (such as Cortex-M0).

Do not forget the cost of the vtable. With `clone_into`, `object_size`, and the virtual destructor,
each derived class puts one vtable in ROM. **If there are only two kinds of waveforms,
an `enum` and a `switch` are usually smaller and faster.**

## 6.9 Conclusion for ROS 2 (supplement)

The GoF `Prototype` does not appear in rclcpp. Message types are
**plain structs** such as `sensor_msgs::msg::Imu`, and the copy constructor works as it is.

```cpp
sensor_msgs::msg::Imu imu_copy = imu_msg;   // This copies it
```

Be careful on the side of `publish()`. If you pass a `std::unique_ptr`, it is a **move** (transfer of ownership),
and if you pass a const reference, the middleware makes a copy.
Do not mix up **copy, share, and move.**
This leads to the zero-copy topic in Chapter 14.

`rclcpp::Parameter` and `rclcpp::QoS` are also value types, and you can copy them to duplicate them.
There is almost no case where you look for `clone()`.

## 6.10 Common pitfalls

| Symptom | Cause |
| --- | --- |
| `error: invalid covariant return type for 'virtual std::unique_ptr<SineSweep> SineSweep::clone() const'` | You tried to use a covariant return type with `unique_ptr`. Use the NVI version of 6.3 |
| You made a copy, but changing the original also changes the copy | Shallow copy. The contents of the pointer member are not copied |
| Crash at run time (SIGABRT, exit code 134; SIGTRAP, exit code 133 with clang on macOS) | Double free of a raw pointer member. Use `unique_ptr` |
| You called `clone()` but `name()` returns the base one | `do_clone()` does `new` of the base. Use `new Derived(*this)` |
| You took it by value and the derived information disappeared | Slicing. Make the base an abstract class, or make the copy `protected` |
| You wrote the copy constructor and the move stopped working | Rule of Five. State the move explicitly with `= default` |
| Base members are not copied | The derived copy constructor does not write `Base(other)` |
| `static_assert(!is_copy_constructible<...>)` fails | Even if you hold a `vector<unique_ptr>`, the implicit copy is not deleted. Danger 3 in 6.5 |

## 6.11 Matching exercise

```bash
./drill run dp06
```

In `exercises/dp06_prototype/src/waveform.cpp`, you implement

1. `Waveform::clone()`: wrap `do_clone()` in a `std::unique_ptr`
2. The copy constructor of `PulseTrain`: a deep copy
3. `PulseTrain::do_clone()` / `SineSweep::do_clone()`: covariant return types
4. `WaveformLibrary::duplicate()`: duplicate all elements without knowing their real types

The tests check **whether it is a deep copy** (change the original and check),
**whether the derived type is kept even through `unique_ptr<Waveform>`** (`dynamic_cast`), and
**whether the copy is a different object** (compare addresses).
They also use `static_assert` to check that classes that should not be copyable really are not.

## 6.12 Summary of this chapter

- Java's `clone()` existed **because Java has no language feature for copying.** C++ has it
- **First ask whether the copy constructor is enough.** If you know the type, you do not need `clone()`
- You need `clone()` only **when you have nothing but a `unique_ptr<Base>`**
- **Covariant return types work only for pointers and references.** `unique_ptr<Base>` → `unique_ptr<Derived>` is not allowed
- The workaround is **NVI**. A public non-virtual `clone()` wraps a private virtual `do_clone()` (raw pointer, covariant)
- **Slicing** is an accident Java does not have. Stop it with an abstract class or a `protected` copy constructor
- The implicit copy of a raw pointer member is a **double free**. With a `unique_ptr` member, the language stops it
- **Rule of Zero comes first.** If you write one, write your intent for all five by the Rule of Five
- If the candidate types are finite, use **`std::variant`**. Copying is already polymorphic
- On microcontrollers, the first choice is "do not use it, copy by value", and if needed, **copy into a fixed pool with placement new**

---

Previous: [5. Singleton](05_Singleton.md) / Next: [7. Builder](07_Builder.md)
