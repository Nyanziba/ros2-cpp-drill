# 7. Builder

> **Corresponds to Chapter 7 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Open `Builder` / `Director` / `TextBuilder` / `HTMLBuilder` from the book.
>
> **Goal of this chapter**: What people call "Builder" is actually **two different things**.
> The one in Hiroshi Yuki's book is a pattern that separates the procedure (Director) from the parts (Builder).
> The `.set_a().set_b().build()` you see in practice is a tool to fill the gap that **C++ has no named arguments**.
> They share a name, but they solve different problems. This chapter writes both, but **does not mix them**.
> The C++-specific topics are the return value of the chain (reference or value), the **ref-qualifier** `build() &&`,
> and **assembling at compile time with `constexpr` and putting the result in ROM**.

## 7.1 First, separate two things that only share a name

If we do not do this first, this chapter stays confusing from start to end.

| | Builder in Hiroshi Yuki's book (GoF) | Builder in practice (method chaining) |
| --- | --- | --- |
| Problem it solves | You want to make **different representations** from **the same procedure** | A call has too many arguments and cannot be read |
| Actors | Director / Builder / ConcreteBuilder | Only one Builder |
| Who holds the procedure | The **Director** | The **caller** (can call in any order) |
| What gets replaced | ConcreteBuilder (polymorphism) | Nothing is replaced. No virtual function appears at all |
| Typical example | Output the same data as CSV and as JSON | Make a `MotorConfig` without 7 positional arguments |

The GoF book never mentions "method chaining" even once.
`.set_a().set_b().build()` is **a different technique spread by Item 2 of Effective Java**.
Both are useful, but **when someone says "I used the Builder pattern", ask which one.**

This course covers both in the articles and the exercises. Sections 7.2 to 7.3 are the form from Hiroshi Yuki's book, and 7.4 and later are the form used in practice.

## 7.2 If you copy the Java version straight into C++

The `Builder` in Hiroshi Yuki's book looks like this (an abstract class).

```java
public abstract class Builder {
    public abstract void makeTitle(String title);
    public abstract void makeString(String str);
    public abstract void makeItems(String[] items);
    public abstract void close();
}
```

If you move it to C++ in the obvious way, you get this.

```cpp
class TelemetryBuilder
{
public:
  virtual ~TelemetryBuilder() = default;
  virtual void make_header(const std::string & title) = 0;
  virtual void make_field(const std::string & key, double value) = 0;
  virtual void make_footer() = 0;
};
```

There are three changes.

### Change 1: Added `virtual ~TelemetryBuilder() = default;`

It is the same every time since Chapter 1. **If you write even one pure virtual function, add a virtual destructor.**
The `Director` in this chapter holds the Builder only by reference, so it does not crash right now,
but the moment you hold it with `std::unique_ptr<TelemetryBuilder>` in the future, it becomes undefined behavior.
Write it from the start instead of **adding it later**.

### Change 2: Changed `String title` to `const std::string & title`

Java's `String` is passed by reference. If you write `void make_header(std::string title)` in C++,
it **copies the string on every call**. If you only read it, use `const std::string &`.

For `String[] items`, if you take a `std::vector<std::string>` by value in C++,
the whole array is copied. In this exercise, we call `make_field(key, value)` several times.
Remember that **just changing "pass an array" to "pass one at a time" removes the copy.**

### Change 3: Did not put `getResult()` in the base class

In Hiroshi Yuki's book too, `getResult()` is on the `TextBuilder` / `HTMLBuilder` side.
The reason is the same in Java and C++: **the return type may differ for each ConcreteBuilder.**

```cpp
class CsvTelemetryBuilder : public TelemetryBuilder
{
public:
  // ...
  const std::string & result() const { return text_; }   // Cannot be put in the base
};
```

In C++ we can go one step further. `result()` **does not need to be virtual.**
The `Director` does not call `result()`. The only caller is the `main` side, which
knows `CsvTelemetryBuilder` as a concrete type.
**Limit virtual functions to "only what is called polymorphically".** The vtable gets smaller.

Note that it returns `const std::string &`. If it returned `std::string`, one copy would run.
We use it when "the Builder is still alive", so a reference is enough.

## 7.3 Who owns it

The `main` in Hiroshi Yuki's book is written like this.

```java
Builder builder = new TextBuilder();
Director director = new Director(builder);
director.construct();
```

In C++ it looks like this. **You do not need `new`.**

```cpp
CsvTelemetryBuilder builder;              // On the stack
TelemetryDirector director{builder};      // Just holds it by reference
director.construct();
std::cout << builder.result();
```

In Chapter 1 and Chapter 4, I said "Java's `new` is `std::unique_ptr`", but
**this is different.** The lifetime of the `Builder` is the same as the scope of `main`, and it is not handed to anyone.
If ownership does not move, **we do not allocate dynamically.**

Instead, the `Director` holds a `TelemetryBuilder &` by reference, so we need a **promise about lifetime**.

```cpp
class TelemetryDirector
{
public:
  explicit TelemetryDirector(TelemetryBuilder & builder) : builder_(builder) {}
  void construct();

private:
  TelemetryBuilder & builder_;   // The Director must not outlive the Builder
};
```

In Java, the GC keeps the Builder alive. In C++,
if the Builder dies first and you call the `Director`, it is undefined behavior. This is the same story as section 1.3 in Chapter 1.

Adding `explicit` is also specific to C++. Without it,
`TelemetryDirector director = builder;` would compile (implicit conversion).
**Use `explicit` for a constructor with one argument.** Java does not have this problem.

## 7.4 From here, the form used in practice: C++ has no named arguments

We make the settings for a motor. There are 7 items.

```cpp
struct MotorConfig
{
  std::uint8_t motor_id = 0;
  std::string name = "unnamed";
  double max_duty = 1.0;
  double current_limit_ampere = 5.0;
  std::uint32_t encoder_counts_per_rev = 4096;
  bool invert_direction = false;
  bool brake_on_stop = true;
};
```

If you make it with a constructor, it looks like this.

```cpp
MotorConfig config{3, "drive_left", 0.8, 12.0, 8192, true, false};
```

**You cannot read what the last `true, false` mean from this line.**
Even if you swap `invert_direction` and `brake_on_stop`, **it still compiles.**
And on the real machine, the motor that should stop does not stop, and it turns in reverse.

In Python, you can write `MotorConfig(motor_id=3, invert_direction=True)`.
C++ has no named arguments. **This is the motivation of the Builder in practice.**
It is not because we like patterns.

### Alternative 1: Make strong types (strong typedef)

Stop using `bool` everywhere, and use types that carry meaning.

```cpp
enum class Direction : std::uint8_t { normal, inverted };
enum class StopBehavior : std::uint8_t { coast, brake };

MotorConfig config{3, "drive_left", 0.8, 12.0, 8192,
                   Direction::inverted, StopBehavior::coast};
```

**This is better than a Builder.** If you get the argument order wrong, you get a **compile error**.
A Builder finds mistakes only at run time.
When you see a list of `bool`s, first think whether you can make them `enum class`.

But where **numbers of the same type are lined up**, like `double max_duty` and `double current_limit_ampere`,
this does not help. That is where the Builder comes in.

### Alternative 2: Pass a settings struct

```cpp
MotorConfig config;
config.motor_id = 3;
config.name = "drive_left";
config.max_duty = 0.8;
```

It is readable. **And it is simpler than a Builder.**
It has two weak points: **you cannot make it `const`**, and
**nobody checks "are the required items filled in?"**.
Even if you forget to write `motor_id`, it works as the settings of motor 0.

In C++20, with a designated initializer you can write `MotorConfig config{.motor_id = 3, .max_duty = 0.8};`
and you can also add `const`. **In the standard, this is from C++20.**
(My local Apple clang accepted it even with `-std=c++17 -pedantic-errors`.
It is only accepted as a compiler extension, and by the standard it is not a C++17 feature.
Other compilers and older toolchains reject it. You cannot rely on it in a C++17 codebase.)

**The decision is this.**

| Situation | What to choose |
| --- | --- |
| `bool`s are lined up | `enum class` (alternative 1) |
| 3 to 4 items, all optional | A settings struct (alternative 2) |
| Required and optional items are mixed | **Builder** |
| Values are known at compile time | **`constexpr` Builder** (7.11) |

## 7.5 The return value of the chain: return a reference

```cpp
class MotorConfigBuilder
{
public:
  MotorConfigBuilder & motor_id(std::uint8_t id);
  MotorConfigBuilder & name(std::string motor_name);
  // ...
};
```

**It is `&`.** If you make it a value, this happens.

```cpp
MotorConfigBuilder motor_id(std::uint8_t id);   // Bad
```

If you chain three times, like `.motor_id(3).name("drive_left").max_duty(0.8)`,
**the whole Builder is copied three times.** `MotorConfig` has a `std::string`,
so every copy allocates on the heap. In Java, returning a reference is the normal thing,
so if you copy the Java Builder, you get this wrong.

The tests of the exercise check this by address.

```cpp
MotorConfigBuilder builder;
EXPECT_EQ(&builder, &builder.motor_id(7));   // Fails if it returns by value
```

The same idea applies to the setter argument. `name` **takes a `std::string` by value and uses `std::move`**.

```cpp
MotorConfigBuilder & MotorConfigBuilder::name(std::string motor_name)
{
  config_.name = std::move(motor_name);
  return *this;
}
```

If you take it as `const std::string &` and assign it, one copy always runs, even when the caller passes a temporary object.
If you take it by value and `move`, a temporary object costs zero allocations.

## 7.6 Ref-qualifiers: `build() &&` and `build() const &`

This is **the most C++-like part of this chapter.** Java does not have it.

`build()` is called in two ways.

```cpp
// (A) Throwaway. The Builder disappears on this line
const auto config = MotorConfigBuilder{}.motor_id(3).name("drive_left").build();

// (B) Give it a name, and fill it with branches. The Builder is still alive afterwards
MotorConfigBuilder builder;
builder.motor_id(3);
if (is_left_side) { builder.invert_direction(true); }
const auto config = builder.build();
```

In (A), you **may take away** the contents of the Builder. Nobody looks at it anymore.
In (B), you **must not take them away.** The Builder may still be used.

You can **write these two separately** by adding `&` / `&&` to the member function.

```cpp
class MotorConfigBuilder
{
public:
  std::optional<MotorConfig> build() const &;   // Called on an lvalue → copy
  std::optional<MotorConfig> build() &&;        // Called on an rvalue → move
};
```

```cpp
std::optional<MotorConfig> MotorConfigBuilder::build() const &
{
  if (!has_motor_id_) { return std::nullopt; }
  return config_;                 // copy
}

std::optional<MotorConfig> MotorConfigBuilder::build() &&
{
  if (!has_motor_id_) { return std::nullopt; }
  return std::move(config_);      // move. One less std::string allocation
}
```

**The compiler chooses between them automatically.** The caller writes nothing.
`MotorConfigBuilder{}...build()` is the `&&` version, and `builder.build()` is the `const &` version.
Only when you want to call the `&&` version explicitly, you write `std::move(builder).build()`.

### What happens if you write only one

If you write only `build() &&` and call it on an lvalue, you get this (actual output).

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// build_rvalue_only.cpp
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

struct MotorConfig
{
  std::uint8_t motor_id = 0;
  std::string name = "unnamed";
  double max_duty = 1.0;
  double current_limit_ampere = 5.0;
  std::uint32_t encoder_counts_per_rev = 4096;
  bool invert_direction = false;
  bool brake_on_stop = true;
};

class MotorConfigBuilder
{
public:
  MotorConfigBuilder & motor_id(std::uint8_t id);
  std::optional<MotorConfig> build() &&;        // Called on an rvalue → move

private:
  MotorConfig config_;
  bool has_motor_id_ = false;
};

MotorConfigBuilder & MotorConfigBuilder::motor_id(std::uint8_t id)
{
  config_.motor_id = id;
  has_motor_id_ = true;
  return *this;
}

std::optional<MotorConfig> MotorConfigBuilder::build() &&
{
  if (!has_motor_id_) { return std::nullopt; }
  return std::move(config_);      // move. One less std::string allocation
}

int main()
{
  MotorConfigBuilder builder;
  builder.motor_id(3);
  const auto config = builder.build();
  return config.has_value() ? 0 : 1;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic build_rvalue_only.cpp -o build_rvalue_only
```

</details>

<!-- measure: -->
```
build_rvalue_only.cpp: In function ‘int main()’:
build_rvalue_only.cpp:46:36: error: passing ‘MotorConfigBuilder’ as ‘this’ argument discards qualifiers [-fpermissive]
   46 |   const auto config = builder.build();
      |                       ~~~~~~~~~~~~~^~
build_rvalue_only.cpp:36:28: note:   in call to ‘std::optional<MotorConfig> MotorConfigBuilder::build() &&’
   36 | std::optional<MotorConfig> MotorConfigBuilder::build() &&
      |                            ^~~~~~~~~~~~~~~~~~
```

**If you write only one ref-qualifier, the other one cannot be used.**
If you decide "this can be used only as a throwaway", writing only `&&` is a valid design
(it prevents the bug of calling `.build()` twice **at compile time**).
If not, **write both.** Writing only one and regretting it is a common accident.

## 7.7 When a required item is not filled in

`motor_id` has no safe default value. Motor 0 is a real motor.
What do we do if `build()` is called while it is not filled in? **There are three choices.**

| Method | How to write it | Good and bad points |
| --- | --- | --- |
| Throw an exception | `throw std::logic_error{...}` | Fine on Linux / ROS 2. **Cannot be used on microcontrollers** |
| Return `std::optional` | `return std::nullopt;` | Works everywhere. **The caller may forget to check** |
| Enforce it with a type | Make the required item a constructor argument | The strongest. **No run-time check is needed** |

Microcontrollers usually use `-fno-exceptions`, so you cannot write `throw`.
**This exercise uses `std::optional`.** The same code runs on microcontrollers and on ROS 2.

The third one is worth writing down.

```cpp
class MotorConfigBuilder
{
public:
  // The required item can be passed only here. Forgetting to fill it is "impossible to write"
  explicit MotorConfigBuilder(std::uint8_t motor_id) { config_.motor_id = motor_id; }

  MotorConfig build() const &;   // No optional needed
};
```

**If there are one or two required items, this is the best.** The run-time check disappears, and `optional` disappears too.
If you have five required items, the Builder has little meaning, so doubt the design in that case.

(There is also a way of "advancing the state with types": each time you fill a required item, you return a different type,
and only the type with everything filled gets `build()`. It can be prevented fully at compile time,
but the number of types grows with the number of items. It is too heavy for your team's library.)

## 7.8 As a tool to make immutable objects

This is where the Builder really pays off.

```cpp
const MotorConfig config = *MotorConfigBuilder{}.motor_id(3).max_duty(0.8).build();
```

**It has `const`.** While assembling, the Builder is mutable,
and the finished `MotorConfig` cannot be changed from start to end.
With the style of filling a settings struct step by step (alternative 2 in 7.4), you cannot make `config` `const`.

What changes when it is immutable?

- "Somebody somewhere changed `max_duty`" **can no longer happen**
- It is safe to read from threads at the same time (multi-threaded execution in ROS 2)
- Even if you read it from an interrupt handler, the value does not change in the middle (microcontrollers)

**The combination of "a mutable Builder" and "an immutable product"** is the core of the Builder.
Being able to write it as a chain is a bonus.

## 7.9 Does the standard library already have the same thing?

**No.** Nothing in the standard library corresponds to the GoF Builder.
The situation is different from the Iterator in Chapter 1 and the Proxy (smart pointers) in Chapter 21.

The closest one is `std::ostringstream`.

```cpp
std::ostringstream oss;
oss << "battery " << 12.5 << " V";
const std::string text = oss.str();
```

The shape "add a little at a time, and take it out at the end" is the same.
The mechanism that you can chain because `operator<<` returns `std::ostream &` is also the same as 7.5.
If you read it as **`std::ostringstream` being a Builder specialized for strings**, it makes sense.

In C++20 you have `std::format`, and many cases of building strings can be done with it.
`std::optional` (7.7) and ref-qualifiers (7.6) can be used in C++17.

## 7.10 Try it yourself

Before you solve the exercise, compile this one file and **predict the output** before you run it.

```cpp
// try.cpp
#include <iostream>
#include <string>
#include <utility>

struct Config
{
  std::string name = "unnamed";
  int retry = 0;
};

class Builder
{
public:
  Builder & name(std::string value) { config_.name = std::move(value); return *this; }
  Builder & retry(int value) { config_.retry = value; return *this; }

  Config build() const & { std::cout << "build() const &\n"; return config_; }
  Config build() && { std::cout << "build() &&\n"; return std::move(config_); }

  const Config & peek() const { return config_; }

private:
  Config config_{};
};

struct Limits
{
  float max_velocity = 10.0F;
  float max_current = 5.0F;
};

class LimitsBuilder
{
public:
  constexpr LimitsBuilder & max_velocity(float v) { limits_.max_velocity = v; return *this; }
  constexpr LimitsBuilder & max_current(float v) { limits_.max_current = v; return *this; }
  constexpr Limits build() const { return limits_; }

private:
  Limits limits_{};
};

// The assembly finishes at compile time. Nothing happens at run time
constexpr Limits kDrive = LimitsBuilder{}.max_velocity(20.0F).build();
static_assert(kDrive.max_velocity == 20.0F, "");
static_assert(kDrive.max_current == 5.0F, "");

int main()
{
  const std::string long_name(64, 'x');   // Make it too long to fit in SSO

  Builder builder;
  std::cout << "same object? "
            << (&builder == &builder.name(long_name).retry(3)) << "\n";

  const char * before = builder.peek().name.data();

  const Config copied = builder.build();
  std::cout << "copied buffer moved? " << (copied.name.data() == before) << "\n";

  const Config moved = std::move(builder).build();
  std::cout << "moved buffer moved?  " << (moved.name.data() == before) << "\n";

  std::cout << "constexpr: " << kDrive.max_velocity << " " << kDrive.max_current << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: Which <code>build()</code> is called when, and how does the string buffer move?</summary>

<!-- measure: files=try.cpp cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try" -->
```
same object? 1
build() const &
copied buffer moved? 0
build() &&
moved buffer moved?  1
constexpr: 20 5
```

There are four ways to read it.

1. `same object? 1`: The setters return `Builder &`, so no copy happened in the middle of the chain
2. `builder.build()` is called **on a named variable**, so it is the `const &` version.
   `copied buffer moved? 0` means the buffer is a different one, that is, it was **copied**
3. `std::move(builder).build()` is the `&&` version. Because it is `1`,
   **the buffer of 64 characters was taken over as it is.** There was zero allocation
4. The two `static_assert`s of `constexpr` pass.
   `kDrive` was never assembled at run time. **It was completed at compile time**

If you change `long_name` to `std::string long_name = "abc";`, item 3 may become `0`.
A short string goes directly inside the `std::string` (SSO), so even with a move, the buffer does not move.
**"Move is fast" is true only once the string is large enough to own heap memory.**
</details>

## 7.11 Conclusion for microcontrollers

**The form in Hiroshi Yuki's book (Director + Builder) is heavy as it is.**

- `ConcreteBuilder` has 3 virtual functions and a virtual destructor → a vtable goes in ROM
- Assembling with `std::string` → heap allocation on every loop

First doubt whether you really need to output telemetry as both CSV and JSON on the real machine.
If there is one output format, you need neither a Director nor a Builder (0.1 of [0. Before you use them](00_before_you_use_them.md)).

**The form used in practice transforms when you make it `constexpr`.** This is the real answer on microcontrollers.

```cpp
struct ControlLimits
{
  float max_velocity_rad_per_sec = 10.0F;
  float max_accel_rad_per_sec2 = 50.0F;
  float max_current_ampere = 5.0F;
};

class ControlLimitsBuilder
{
public:
  constexpr ControlLimitsBuilder & max_velocity(float value)
  {
    limits_.max_velocity_rad_per_sec = value;
    return *this;
  }
  constexpr ControlLimitsBuilder & max_accel(float value)
  {
    limits_.max_accel_rad_per_sec2 = value;
    return *this;
  }
  constexpr ControlLimitsBuilder & max_current(float value)
  {
    limits_.max_current_ampere = value;
    return *this;
  }
  constexpr ControlLimits build() const { return limits_; }

private:
  ControlLimits limits_{};
};

// The assembly finishes here. Not one instruction runs at run time
constexpr ControlLimits kDriveLimits =
  ControlLimitsBuilder{}.max_velocity(20.0F).max_accel(80.0F).build();

static_assert(kDriveLimits.max_velocity_rad_per_sec == 20.0F, "");
static_assert(kDriveLimits.max_current_ampere == 5.0F, "");   // Still the default value
```

What you get:

- **`kDriveLimits` is placed in `.rodata` (ROM).** It uses not one byte of RAM
- No code is generated for the assembly. `ControlLimitsBuilder` does not exist at run time
- You can check the validity of values **at compile time** with `static_assert`
- And the call is still readable with names

Since C++14, you can change members inside a `constexpr` function, so
**the style of filling in with a chain runs as it is at compile time.** You could not write it in C++11.

There are two limits.

1. **The moment you put in a `std::string`, it cannot be `constexpr`.** Use a fixed-length array or a `string_view`
2. **If you put in a virtual function, it cannot be `constexpr`** (in C++17).
   So the Director + Builder of Hiroshi Yuki's book cannot live together with `constexpr`. **It is one or the other**

Because exceptions cannot be used, for the check of required items, use `std::optional` (7.7) or
the way of making them required with constructor arguments. You cannot write `throw`.

## 7.12 Conclusion for ROS 2 (supplement)

In rclcpp, `rclcpp::QoS` is exactly this form.

```cpp
auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable().transient_local();
```

The required item (the history depth) is a **constructor argument**, and the optional items are the chain.
It is a real example of choice 3 in 7.7, "enforce the required item with the constructor".

`rclcpp::NodeOptions` is the same, and you chain `.use_intra_process_comms(true).automatically_declare_parameters_from_overrides(true)`.
In both, **the Director of Hiroshi Yuki's book does not appear.** Only the form used in practice.

`rcl_interfaces::msg::ParameterDescriptor`, around parameter declaration, is
the form of "fill the settings struct directly" in alternative 2 of 7.4, and there is no Builder.
**It is not consistent even inside rclcpp**, so when you copy it, look at "why that form".

## 7.13 Common pitfalls

| Symptom | Cause |
| --- | --- |
| The more you chain, the slower it gets. The profiler shows `std::string` copies | The setters return `Builder` (by value). Make it `Builder &` |
| `error: passing 'MotorConfigBuilder' as 'this' argument discards qualifiers` | You wrote only `build() &&`. Also write `build() const &` |
| You called `build()` twice and the second one was empty | The `build() &` version does `std::move`. The lvalue version should copy |
| Copies do not decrease even with `std::move(builder).build()` | The string is short and fits in SSO. A move helps only when the string is large enough to own heap memory |
| It does not compile after adding `constexpr` | A member has a `std::string` or a virtual function. They cannot live together with `constexpr` |
| `TelemetryDirector director = builder;` compiles | The constructor has no `explicit` |
| It crashed after returning a Director from a function | The Director holds the Builder by reference. The Builder died first |
| You did not notice you got the argument order wrong | A list of `bool`s. Use `enum class` before a Builder (alternative 1 in 7.4) |
| You used `*` on the result of `build()` without checking | It returns a `std::optional`. Check `has_value()` |

## 7.14 Matching exercise

```bash
./drill run dp07
```

**The form in Hiroshi Yuki's book**: `exercises/dp07_builder/src/telemetry_builder.cpp`

1. `TelemetryDirector::construct()`: call the Builder 5 times in the fixed order.
   Its job is to **write no formatting at all**
2. `CsvTelemetryBuilder`: assemble as CSV
3. `JsonTelemetryBuilder`: assemble as JSON. The trick is to put the comma **before** each item

**The form used in practice**: `exercises/dp07_builder/src/motor_config.cpp`

4. The 7 setters of `MotorConfigBuilder`: **return `MotorConfigBuilder &`**
5. `MotorConfigBuilder::build() const &`: copy and return. If a required item is missing, return `std::nullopt`
6. `MotorConfigBuilder::build() &&`: `std::move` and return

What the tests check:

- The same `Director` can produce both CSV and JSON
- The **call order** of the Director (we plug in a recording Builder to check)
- All values set by the chain are applied, and items not set keep their default values
- `build()` with a missing required item returns `std::nullopt`
- The chain returns **the address of the same Builder** (no copy happened)
- `build() &&` moves the buffer, and `build() const &` copies it
- A `constexpr` Builder is assembled **at compile time** (`static_assert`)

## 7.15 Summary of this chapter

- There are **two** "Builders". The Director + Builder of Hiroshi Yuki's book, and the method chain used in practice. **Do not mix them**
- In the form of the book, the `Director` holds the procedure and the `ConcreteBuilder` holds the format.
  Do **not** put `result()` in the base, and do **not** make it virtual
- The lifetime of a Builder is fine with a scope. **You do not need `unique_ptr`.** Instead, take responsibility for the lifetime of the reference
- The motivation of the Builder in practice is that **C++ has no named arguments.**
  First consider `enum class` (strong types) and a settings struct
- Setters **return `Builder &`**. If they return by value, every step of the chain copies
- **`build() const &` copies, and `build() &&` moves.** Separate them with ref-qualifiers. Java does not have this
- For required items, use `optional` or **enforce them with constructor arguments**. You cannot `throw` on microcontrollers
- The core of the Builder is "a mutable Builder → an **immutable product**". The value is that you can add `const`
- The standard library has **no** Builder. The closest is `std::ostringstream`
- On microcontrollers, use a **`constexpr` Builder**. It can go in ROM, with zero RAM and zero run-time cost.
  But it cannot live together with `std::string` or virtual functions

---

Previous: [6. Prototype](06_Prototype.md) / Next: [8. Abstract Factory](08_AbstractFactory.md)
