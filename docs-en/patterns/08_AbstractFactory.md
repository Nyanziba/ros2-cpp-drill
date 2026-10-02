# 8. Abstract Factory

> **Corresponds to Chapter 8 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Open `Factory`, `Link`, `Tray`, `Page`, and the implementation on the `ListFactory` side from the book.
>
> **Goal of this chapter**: Abstract Factory is **the pattern among the 23 chapters that is most "easy to add even though you do not need it"**.
> So we reverse the order. **We first do "the decision not to add it"**,
> and then go on to how to write it in C++ when you decide to add it (a pure virtual class that returns `unique_ptr`).
> After that, we show that **if you do not need run-time polymorphism, you can write it with templates.**
> The vtable and the heap disappear. On microcontrollers, that is the real answer.

## 8.1 First, the decision not to add it

Remember the example in 0.3 of [0. Before you use them](00_before_you_use_them.md), "Creation becomes three steps".

```cpp
auto factory = SensorFactoryProvider::instance().get_factory("imu");
auto builder = factory->create_builder();
auto sensor  = builder->with_address(0x68)->build();
```

Abstract Factory is the main culprit of this. Before you add it, ask yourself **two** questions.

### Question 1: Are there really two or more product families?

What Abstract Factory handles is not "one kind of product". It is a **set of several products that go together as a pair**.
In the example of Hiroshi Yuki's book, `Link`, `Tray`, and `Page` make one set, and there are two sets: the HTML version and the bullet-list version.

In your own code, do you really have two such sets?
If you only have "two implementations of a motor driver", that is **one product**.
Factory Method (Chapter 4) is enough. You do not need Abstract Factory.

**If you add Abstract Factory when there is only one set, the number of files triples and you gain nothing.**

### Question 2: Do you really need to switch at run time?

This question is specific to C++. In Java, run-time polymorphism is the only choice,
so this question does not come up. C++ has ways to **decide at compile time**.

```cpp
// Is it decided at run time? Really?
const char * mode = std::getenv("ROBOT_MODE");
auto factory = make_factory(mode);
```

In your own code, is there a case where you **switch the real-machine version and the simulation version inside the same binary**?
If you build them separately, the switch happens at compile time. In that case,

```cpp
// This is enough. No vtable, no heap
using Kit = SimulationKitTraits;
```

passing it as a template argument is enough (we do this in 8.6).

**Decision table**

| Situation | What to use |
| --- | --- |
| One kind of product, two implementations | Factory Method (Chapter 4). Or one function |
| Two or more product families, decided at **build time** | Templates (policy classes) |
| Two or more product families, switched at **run time** | Abstract Factory (this chapter) |
| Only one product family | **Add nothing.** Use `new` directly or hold it by value |

In the exercise of this chapter, you write **both the run-time version and the template version**.
If you do not write them side by side, you cannot judge "which one is enough".

## 8.2 The difference from Factory Method (Chapter 4) is "one product vs a product family"

These two are often confused because their names are similar. There is only one difference.

| | Factory Method (Chapter 4) | Abstract Factory (Chapter 8) |
| --- | --- | --- |
| What it makes | **One kind** of product | **Several** products that go together as a pair |
| Form of the abstraction | A virtual function that creates (one) | A type that **has several** virtual functions that create |
| What you want to protect | Sharing the creation procedure | The **consistency of the product family** (no mixing) |
| What is easy to add | New **kinds** of product | New product **families** |
| What is hard to add | — | New kinds of product (every factory needs a change) |

The third row is the main point. **The value of Abstract Factory is not "putting creation together".
It is "you cannot mix the parts of A and the parts of B".**
If you do not need this, there is no point in adding it.

## 8.3 If you copy the Java version straight into C++

The `Factory` in Hiroshi Yuki's book looks like this.

```java
public abstract class Factory {
    public abstract Link createLink(String caption, String url);
    public abstract Tray createTray(String caption);
    public abstract Page createPage(String title, String author);
}
```

We replace it with the subject of the exercise (a pair of motor output and encoder input), and move it to C++ in the obvious way.

```cpp
class ActuatorKitFactory
{
public:
  virtual ~ActuatorKitFactory() = default;

  virtual std::unique_ptr<MotorOutput> create_motor() const = 0;
  virtual std::unique_ptr<EncoderInput> create_encoder() const = 0;

  virtual KitId kit_id() const = 0;
};
```

There are three changes from the Java version.

### Change 1: Added `virtual ~ActuatorKitFactory() = default;`

It is the same every time since Chapter 1. **If you write even one pure virtual function, add a virtual destructor.**
In Abstract Factory, three base classes (`MotorOutput` / `EncoderInput` / `ActuatorKitFactory`)
appear, so **the places where you can forget it also triple.**

### Change 2: Used a class with only pure virtual functions, not an `abstract class`

Java's `abstract class` is "an abstract class that can have part of the implementation".
The `Factory` in Hiroshi Yuki's book also has a static method `getFactory(String)`.

If you do the same in C++, `dynamic_cast` and RTTI get involved and it becomes a hassle,
so **we make the abstract class only pure virtual functions, and put the way to create the factory
(which factory to use) on the caller side.** If you write the creation of the factory in the factory, you get the "three steps" of 0.3.

If even one function has `= 0`, the class cannot be instantiated.
The mark that corresponds to Java's `abstract` goes on the **member function side** in C++.

### Change 3: Made the return type `std::unique_ptr`

The Java version just does `new` and returns. In C++, we write the ownership in the type. We cover it in the next section.

Note that `create_motor()` has `const`.
This is because the state of the factory itself does not change. **Java has no such distinction.**

## 8.4 Who owns it

```cpp
MotorOutput * create_motor() const;                  // Who does delete?
std::unique_ptr<MotorOutput> create_motor() const;   // The caller owns it
```

The judgment is the same as in Chapter 1 and Chapter 4. **Where Java does `new` and returns, we return a `std::unique_ptr`.**

The caller side looks like this.

```cpp
RunResult run_open_loop(const ActuatorKitFactory & factory, int duty, int steps)
{
  const std::unique_ptr<MotorOutput> motor = factory.create_motor();
  const std::unique_ptr<EncoderInput> encoder = factory.create_encoder();

  for (int step = 0; step < steps; ++step) {
    motor->set_duty(duty);
  }

  return RunResult{factory.kit_id(), encoder->read_count()};
}
```

Check that **not one name of a concrete factory appears in this function.**
If one appears, you did not abstract it. There is no point in adding Abstract Factory.

On the other hand, the factory itself is taken as `const ActuatorKitFactory &`.
You may want to take it as `std::unique_ptr<ActuatorKitFactory>`, but **this function does not own the factory.**
Things you do not own are taken by reference. This is also a difference from Java (in Java everything is a reference, so there is no doubt).

## 8.5 A danger only C++ has: it is the "human", not the compiler, that can mix them

The selling point of Abstract Factory is "product families do not mix", but
**if you do not go through the factory, they mix easily.**

```cpp
SimulationBus bus;
HardwareRegisterFile registers;

auto motor = std::make_unique<SimMotor>(bus);          // For simulation
auto encoder = std::make_unique<HwEncoder>(registers); // For the real machine. It compiles
```

`MotorOutput` and `EncoderInput` are different types, so this combination is **not contradictory as types**.
The compiler does not stop it. The control loop keeps raising the duty while the encoder does not move.

So, **what guarantees "no mixing" is not the types but the convention "create things only through the factory".**
To enforce the convention, do not expose the concrete product classes in a header.

```cpp
// Inside actuator_kit.cpp
namespace
{
class SimMotor final : public MotorOutput { /* ... */ };
class HwEncoder final : public EncoderInput { /* ... */ };
}  // namespace
```

**If you confine them in an anonymous namespace, you cannot even write their names from outside.**
Going through the factory becomes the only entrance. The exercise is written in this form too.

There is one more C++-specific pitfall. **A product cannot outlive the factory.**

```cpp
std::unique_ptr<MotorOutput> make_motor()
{
  SimulationBus bus;                       // local
  const SimulationKitFactory factory{bus};
  return factory.create_motor();           // bus dies here
}                                          // The returned motor points to a dead bus
```

The product holds a reference to the bus (the shared state). In Java, the GC keeps `bus` alive. In C++, it crashes.
It is the same shape as section 1.3 in Chapter 1, and in Abstract Factory **the reference has two levels: "shared state → factory → product"**,
so it is harder to notice.

## 8.6 The template version (policy-based): the vtable and the heap disappear

**If you do not switch at run time, the whole structure of Abstract Factory can move to templates.**

A description of a product family as "a set of types" is called **Traits** (a policy).

```cpp
struct SimulationKitTraits
{
  using Bus = SimulationBus;
  using Motor = SimMotorCore;
  using Encoder = SimEncoderCore;
  static constexpr KitId kit_id = KitId::Simulation;
};

struct HardwareKitTraits
{
  using Bus = HardwareRegisterFile;
  using Motor = HwMotorCore;
  using Encoder = HwEncoderCore;
  static constexpr KitId kit_id = KitId::Hardware;
};
```

This is **the replacement of the abstract factory.** There is not one `virtual`.
The client is written like this.

```cpp
template <typename KitTraits>
RunResult run_open_loop_static(typename KitTraits::Bus & bus, int duty, int steps)
{
  typename KitTraits::Motor motor{bus};              // No heap allocation. Placed right here
  const typename KitTraits::Encoder encoder{bus};

  for (int step = 0; step < steps; ++step) {
    motor.set_duty(duty);
  }

  return RunResult{KitTraits::kit_id, encoder.read_count()};
}
```

`typename` is needed because `KitTraits::Motor` is a **name that depends on a template argument.**
The compiler cannot tell whether it is a type or a value, so `typename` tells it "this is a type".
If you forget it, you get a hard-to-read error such as `error: expected ';' after expression`.

**Correspondence with the run-time version**

| | Run-time version | Template version |
| --- | --- | --- |
| Abstract factory | A pure virtual class | A `struct` of Traits |
| Concrete factory | A derived class | Two Traits instances |
| Creating products | `std::make_unique` (heap) | Local variables (stack) |
| Call cost | One virtual call each | Zero (may be inlined) |
| Size of one product | vtable pointer + contents | Contents only |
| Switching | At run time | **Only at compile time** |
| Unit of replacement | An object | A type |

**The consistency of the product family is protected in the template version too.** `KitTraits::Motor` and `KitTraits::Encoder`
are taken from the same Traits, so there is no way to mix them. It is even protected more strongly than in the run-time version.

There are two costs.

1. **You cannot switch at run time.** If you use both product families in one binary, both get instantiated
2. **All the code goes in the header (or in the translation unit that defines the template).** The compile time gets longer

To avoid 2, the exercise defines the templates inside the `.cpp`, and exposes
**only non-template entrances** named `run_open_loop_static_sim` / `run_open_loop_static_hw`.
It is a misunderstanding that a template must always be in a header.
**It is enough if it is in the same translation unit as the place where it is used.**

## 8.7 Does the standard library or the language already have the same thing?

**No.** Unlike Iterator (Chapter 1) and Proxy (Chapter 21),
the standard library has no part that corresponds to Abstract Factory, so you write it yourself.

But sometimes **a language feature replaces it.**

| What you want to do | Standard way |
| --- | --- |
| Switch the product family at compile time | A template argument (8.6) |
| Replace the type of a product later | A `using` alias, `if constexpr` |
| Switch by build configuration | Choose the `.cpp` in CMake. No pattern needed |

The last row is a serious option. Just **changing which files you link for the real machine and for simulation**
can remove the need for both abstract classes and templates.
Always think first, "can we switch it in the build?"

## 8.8 Try it yourself

Before you solve the exercise, compile this one file and **predict the output** before you run it.

```cpp
#include <cstddef>
#include <iostream>
#include <memory>

struct SimBus  { int count = 0; };
struct HwRegs  { int count = 0; };

class Motor
{
public:
  virtual ~Motor() = default;
  virtual void set_duty(int duty) = 0;
};

class Encoder
{
public:
  virtual ~Encoder() = default;
  virtual int read() const = 0;
};

class SimMotor : public Motor
{
public:
  explicit SimMotor(SimBus & bus) : bus_(bus) {}
  void set_duty(int duty) override { bus_.count += duty; }

private:
  SimBus & bus_;
};

class SimEncoder : public Encoder
{
public:
  explicit SimEncoder(const SimBus & bus) : bus_(bus) {}
  int read() const override { return bus_.count; }

private:
  const SimBus & bus_;
};

class HwEncoder : public Encoder
{
public:
  explicit HwEncoder(const HwRegs & regs) : regs_(regs) {}
  int read() const override { return regs_.count; }

private:
  const HwRegs & regs_;
};

// A product of the template version (no virtual functions)
struct SimMotorCore
{
  explicit SimMotorCore(SimBus & bus) : bus_(bus) {}
  void set_duty(int duty) { bus_.count += duty; }
  SimBus & bus_;
};

int main()
{
  SimBus bus;
  HwRegs regs;

  // If you assemble by hand without the factory, you can mix product families
  std::unique_ptr<Motor> motor = std::make_unique<SimMotor>(bus);
  std::unique_ptr<Encoder> encoder = std::make_unique<HwEncoder>(regs);
  motor->set_duty(10);
  std::cout << "mixed:  " << encoder->read() << "\n";

  std::unique_ptr<Encoder> right = std::make_unique<SimEncoder>(bus);
  std::cout << "paired: " << right->read() << "\n";

  std::cout << "sizeof(SimMotor)     = " << sizeof(SimMotor) << "\n";
  std::cout << "sizeof(SimMotorCore) = " << sizeof(SimMotorCore) << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: What does <code>mixed</code> print? And where does the difference between the two <code>sizeof</code> values come from?</summary>

```
mixed:  0
paired: 10
sizeof(SimMotor)     = 16
sizeof(SimMotorCore) = 8
```

**`mixed` is 0.** Even if you give a duty to the simulation motor, the real-machine encoder sees nothing.
**It compiles.** The types are not contradictory.
In a control loop, this is the state "commands go out but no feedback comes back",
the integral term saturates, and the motor runs at full power. This is the accident Abstract Factory wants to prevent.

The difference of 8 bytes in `sizeof` is the **vtable pointer.**
`SimMotorCore` has only one reference (8 bytes), and `SimMotor` has the vtable pointer on top of it.
If you hold 100 products, this difference appears in RAM as it is.
(These are the values for a 64-bit environment. On a 32-bit microcontroller, they would be 4 and 8.)
</details>

## 8.9 Conclusion for microcontrollers

**I write the conclusion first.**

1. **First consider the template version (8.6).** It needs no heap and no vtable
2. **If you cannot say why you want to switch at run time, switch in the build**
3. If you still need run-time polymorphism, write an **Abstract Factory that does not return `unique_ptr`**

How to write 3 is the main topic. On microcontrollers, the problem is that `create_motor()` calls `std::make_unique`.
Once at startup might be acceptable, but **there is no need to create it in the first place.**
Peripherals exist, one each, from power-on.

So we make a form where **the factory holds the products as value members and hands out references.**

```cpp
/// An abstract kit. It does not "create"; it "hands out references to what already exists".
class ActuatorKit
{
public:
  virtual ~ActuatorKit() = default;
  virtual MotorOutput & motor() = 0;
  virtual EncoderInput & encoder() = 0;
};

/// The concrete kit holds the products by value. There is no heap allocation anywhere.
class HardwareKit final : public ActuatorKit
{
public:
  explicit HardwareKit(Registers & regs)
  : motor_(regs), encoder_(regs)
  {
  }

  MotorOutput & motor() override { return motor_; }
  EncoderInput & encoder() override { return encoder_; }

private:
  HwMotor motor_;
  HwEncoder encoder_;
};

Registers g_regs{0, 0};
HardwareKit g_kit{g_regs};      // Static storage. The allocation is finished before startup

std::uint32_t drive(ActuatorKit & kit, std::uint32_t duty)
{
  kit.motor().set_duty(duty);
  return kit.encoder().read_count();
}
```

This passes with zero warnings even with `-fno-exceptions -fno-rtti` (measured).

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti -c micro.cpp
```

**We changed only one thing. `create_*()` no longer returns `unique_ptr`; it returns `&`.**
With this,

- Heap allocation becomes zero
- The lifetime of a product matches the lifetime of the kit (**a dangling pointer cannot be made structurally**)
- The consistency of the product family is kept (the kit passes the same `Registers` to both)

The only remaining cost is the vtable. With `ActuatorKit` / `MotorOutput` / `EncoderInput`,
the vtables increase by 3 plus the concrete classes. If you mind this, use the template version of 8.6.

There are two points to note.

- **`g_kit` is a global variable.** If it spans several translation units, the initialization order problem appears
  (the story of Chapter 5, Singleton). Put `g_regs` and `g_kit` in the same `.cpp`
- **`motor()` cannot be `const`.** It returns a non-const reference.
  If you feel like making it `const`, that is a sign to review the design

## 8.10 Conclusion for ROS 2 (supplement)

The ROS 2 side has looser constraints, so you can use the `unique_ptr` version of 8.3 to 8.4 as it is.

What is really close is **`pluginlib`.** `pluginlib::ClassLoader`
"chooses a class by a string at run time and returns a `shared_ptr`", so its role is the same as Abstract Factory.
But it provides **only one kind of product**, so, to be exact, it is on the Factory Method side.
pluginlib does not protect the consistency of a product family. **You need to write that yourself.**

`rclcpp` itself has no Abstract-Factory-like abstraction.
For switching between simulation and the real machine, the ROS 2 way is not a pattern but **replacing nodes** (in a launch file).
**In most cases that is better.**
Think first, "before writing an Abstract Factory, can we separate it with launch?"

## 8.11 Common pitfalls

| Symptom | Cause |
| --- | --- |
| You command the motor but the encoder count does not change | `create_motor` and `create_encoder` look at different bus / registers. The product families are mixed |
| You replaced the factory but the behavior did not change | A concrete class name is written inside `run_open_loop` |
| The program crashes when you free the factory | There is no virtual destructor. All three bases need one |
| It crashed when you returned a product from a function that returns a factory | The lifetime problem of 8.5. The reference has two levels: shared state → factory → product |
| `error: expected ';'` appears in the template version | You forgot `typename` in `typename KitTraits::Motor` |
| You added a product and every factory needed a change | A structural weakness of Abstract Factory. Product **families** are easy to add, but product **kinds** are hard to add |
| `create_motor()` cannot be made `const` | The factory holds state. Review the design |
| You want to use a concrete product class directly from a test | Because it is confined in an anonymous namespace. That is correct. Write tests through the factory |

## 8.12 Matching exercise

```bash
./drill run dp08
```

The subject is **a pair of motor output and encoder input.**
There is a product family for simulation and a product family for the real machine, and they must not be mixed.

In `exercises/dp08_abstract_factory/src/actuator_kit.cpp`, you implement

1. The four concrete products (`SimMotor` / `SimEncoder` / `HwMotor` / `HwEncoder`) **inside an anonymous namespace**
2. `create_motor()` / `create_encoder()` / `kit_id()` of `SimulationKitFactory` / `HardwareKitFactory`
3. `run_open_loop()`: **without writing any name of a concrete factory**

In `exercises/dp08_abstract_factory/src/static_kit.cpp`, you implement

4. `run_open_loop_static<KitTraits>()`: the template version

The tests check that **the same abstract code runs on both product families just by replacing the factory**,
that **the created parts belong to the same product family**,
that **the caller owns the products**, and
that **the template version gives the same result as the run-time version.**
They also use `static_assert` to check that "the products of the template version have no vtable".

## 8.13 Summary of this chapter

- **The decision not to add it comes first.** Add it only when there are two or more product families and they are switched at run time
- The difference from Factory Method is **one product vs a product family.** The names are similar, but the goals are different
- **The real value is not "putting creation together" but "product families do not mix".**
  If you do not need this, there is no point in adding it
- Java's `interface` / `abstract class` → a class with only pure virtual functions.
  **Three bases appear, so forgetting the virtual destructor also triples**
- Where Java does `new` and returns, **return a `std::unique_ptr`**
- **Types do not stop mixing.** Confine the concrete products in an anonymous namespace, and make the factory the only entrance
- **If you do not need run-time polymorphism, use Traits + templates.** The vtable and the heap disappear. The consistency even becomes stronger
- On microcontrollers, do not return `unique_ptr`; **hand out references.** The kit holds the products by value. Zero allocation, and the lifetime is safe too
- In ROS 2, first think whether you can replace whole nodes with launch

---

Previous: [7. Builder](07_Builder.md) / Next: [9. Bridge](09_Bridge.md)
