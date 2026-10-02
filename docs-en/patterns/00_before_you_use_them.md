# 0. Before you use them — when patterns break your code

> **Goal of this chapter**: Right after the reading group is the time when you are most likely to break your code.
> Far more often than "the code is messy because I did not know the pattern", the cause is
> **"the code is messy because I overused a pattern I had just learned."**
> First, let us look at four common ways to break things.

This chapter has no coding exercise. It takes 5 minutes to read.

## 0.1 Abstracting when there is only one implementation (Strategy disease)

The day after you read about Strategy in the reading group, this happens.

```cpp
// Bad example: there is only one implementation, MotorDriverImpl
class IMotorDriver
{
public:
  virtual ~IMotorDriver() = default;
  virtual void set_duty(double duty) = 0;
};

class MotorDriverImpl : public IMotorDriver
{
public:
  void set_duty(double duty) override { /* ... */ }
};
```

If an interface has only one implementation, it is not an abstraction. It is **just one more file**.
The caller cannot tell what happens by looking at `IMotorDriver`, and only has the extra work of going to find the implementation.

```cpp
// This is enough to start with
class MotorDriver
{
public:
  void set_duty(double duty) { /* ... */ }
};
```

**Criterion**: "Is there a second implementation **now**, or will one surely come within the next month?"
You may count a mock for tests as the second one, but then
write the reason: "I want to swap it for tests."

## 0.2 Renaming a global variable to Singleton

This comes after chapter 5.

```cpp
// Bad example: just a global variable
class Config
{
public:
  static Config & instance();
  int motor_max_duty = 100;
};
```

Even if you make it a Singleton, **it does not solve even one of the problems of global variables**.

- It can be rewritten from anywhere
- You cannot swap it in tests (the state of the previous test leaks into the next)
- You cannot read from the code who depends on it
- It breaks with multiple threads

Worse, because "I used a pattern", **only your guilt about global variables disappears**. This does the most harm.

**First, think about whether you can pass it as an argument.** If three functions use `Config`, you only need to pass it to those three.
You really need a singleton only when **there is only one piece of hardware** (there is only one `UART1` in the world).
We cover that in chapter 5.

## 0.3 Creation becomes three steps

This happens when you read Factory Method and Abstract Factory one after another.

```cpp
auto factory = SensorFactoryProvider::instance().get_factory("imu");
auto builder = factory->create_builder();
auto sensor  = builder->with_address(0x68)->build();
```

Something that `Imu sensor{0x68};` can do has become three lines.
The only thing you gain from this code is "you can switch the sensor type with a string at run time".
**In your team's library, is there really a case where you choose the sensor type by a string at run time?**
If it is decided at compile time, you do not need a Factory.

## 0.4 Notifications loop in Observer

Chapter 17. In C++, lifetime problems are added on top.

```cpp
// When A is updated, notify B. When B is updated, notify A
a.add_observer(&b);
b.add_observer(&a);   // infinite loop
```

There is also a problem specific to C++: if **the subscriber dies first**, the raw pointer dangles.

```cpp
{
  Display display;
  sensor.add_observer(&display);
}                                 // display dies. sensor still holds &display
sensor.notify();                  // undefined behavior
```

In Java, the GC keeps the object alive (which is a leak in its own way), so it does not crash.
In C++, it crashes. This is the main topic of chapter 17.

## 0.5 Summary: checks before you use a pattern

Before you add a pattern, ask yourself these four questions.

1. **Are there two or more implementations now?** If there is one, do not add it
2. **If I remove this abstraction, which change becomes hard?** If you cannot answer, do not add it
3. **Can I say in one line who releases this object?** If you cannot, the design is not finished
4. **Does the standard library already have the same thing?** Iterator, Proxy, Flyweight, and Command already exist

"Use patterns so that the code does not become a mess" is half right and half backwards.
**What keeps code from becoming a mess is not patterns, but "clear ownership" and "a small number of abstractions".**
Patterns are only one part of the tools for that.

---

Next: [1. Iterator](01_Iterator.md)
