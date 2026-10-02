# 15. Facade

> **Matches chapter 15 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep `PageMaker` / `Database` / `HtmlWriter` open next to you.
>
> **Goal of this chapter**: In C++, **half of this pattern is absorbed by the language**.
> Yuki's `PageMaker` is a class with "a private constructor + one static method", but
> C++ has top-level free functions and namespaces, so **there is no reason to make it a class**.
> The other half, a window that "initializes, uses, and cleans up", is the real core of the C++ Facade,
> and you write it with **RAII**. This is work that the Java Facade does not have.

## 15.1 First, most Facades can be functions, not classes

Yuki's `PageMaker` looks like this.

```java
public class PageMaker {
    private PageMaker() {                       // do not let anyone create an instance
    }
    public static void makeWelcomePage(String mailaddr, String filename) {
        // combine Database and HtmlWriter
    }
}
```

**The line `private PageMaker() {}` says everything about this design.**
It is a note that says, "This is not a class. Java just has no way to put a function outside a class."
C++ has no such restriction.

```cpp
// robot_startup.hpp
namespace robot
{
StartupResult start_once(const StartupConfig & config);
}
```

That is all. You do not need one character of `class`.

### Do not write a class that has only `static` member functions

When you port to C++, you will want to write this. **Do not write it.**

```cpp
// Bad example: Java habits come straight through
class RobotStarter
{
public:
  RobotStarter() = delete;
  static StartupResult start_once(const StartupConfig & config);
};
```

You gain nothing. You lose three things.

| Point | Class with only `static` members | Namespace + free function |
| --- | --- | --- |
| Call | `RobotStarter::start_once(c)` | `robot::start_once(c)` (can be shortened with `using`) |
| Adding one function | Rewrite the class definition in the header -> **every user recompiles** | Can be added in another header. Can be split |
| ADL (argument-dependent lookup) | Does not work | Works |
| Splitting across namespaces | Not possible (a class is closed in one place) | Possible |

The second one matters in practice. A class is not "open", so
if you only want to add `start_for_test` next to `start_once`,
every `.cpp` that includes that header is recompiled.
A namespace is open, so if you add it in another header, only the side that added it is affected.

**The decision rule is one thing: "does it have state?"**

- No (it takes arguments, does something, and returns) -> **namespace + free function**
- Yes (it initializes, uses, and cleans up) -> **class, and specifically an RAII class**

It is the same point as "abstracting something that has only one implementation" in chapter 0.
The moment you write `class`, the reader starts looking for "is there state?", "will I create several?", "will I inherit?".
**If all the answers are "no", it is a function.**

## 15.2 Porting the Java version to C++ as it is

Let's look at the subject of the exercise. Behind "start the robot" there are 4 steps.

```
power on -> sensor init -> calibration -> link up
```

### Change 1: `PageMaker` class -> `namespace robot` + free function

As in 15.1. What corresponds to Yuki's `makeWelcomePage` is `robot::start_once()`.

```cpp
namespace robot
{
StartupResult start_once(const StartupConfig & config, std::vector<std::string> * log = nullptr);
}
```

### Change 2: changed the return value from `void` to `StartupResult`

The Java `makeWelcomePage` is `void`, and on failure it throws an exception.
In C++ we return **at which stage it failed** as a value.

```cpp
struct StartupResult
{
  bool ok = false;
  StartupStage failed_stage = StartupStage::kPower;
};
```

The reason is the same as chapter 14. On microcontrollers `-fno-exceptions` is normal, so
an API built on `throw` cannot be brought over as it is.
"Startup failed" is **expected**, and it is not the time for an exception.

### Change 3: removed the internal classes from the header

Yuki's `Database` / `HtmlWriter` have `package` scope (no access modifier), and
they are invisible from outside the package.
C++ has no `package`, but **there is a stronger way**.
If you put the subsystem in an unnamed namespace in the `.cpp`, **the name does not even exist for other translation units**.
We do this in 15.6.

### What does not change: a Facade is thin

As Yuki's book stresses. If you start writing decisions in a Facade, it is no longer a Facade.
It is the same in C++: **the content of a Facade should be only "the order of calls"**.

## 15.3 Write a Facade that has state with RAII: this is the real core

`start_once()` is a window that "starts up and stops right there".
What you actually want is probably "start up, **use**, and stop".
Here, for the first time, a class is justified.

```cpp
class RobotSession
{
public:
  explicit RobotSession(StartupConfig config, std::vector<std::string> * log = nullptr);
  ~RobotSession();

  RobotSession(const RobotSession &) = delete;
  RobotSession & operator=(const RobotSession &) = delete;
  RobotSession(RobotSession && other) noexcept;
  RobotSession & operator=(RobotSession &&) = delete;

  bool is_ready() const;
  StartupStage failed_stage() const;
  bool drive(int duty);

private:
  StartupConfig config_;
  std::vector<std::string> * log_;
  int completed_stages_;
  bool ready_;
  StartupStage failed_stage_;
};
```

The user side looks like this.

```cpp
{
  RobotSession session{StartupConfig{}};
  if (!session.is_ready()) {
    return 1;
  }
  session.drive(50);
}   // here: link down -> calibration discarded -> sensor stopped -> power cut
```

**There is no line anywhere that calls `stop()`.** This is the biggest difference from the Java version.

If you write the same thing in Java, it looks like this.

```java
RobotSession session = new RobotSession(config);
try {
    session.drive(50);
} finally {
    session.close();          // if you forget to write it, the power stays on
}
```

`try-with-resources` makes it somewhat better, but **the responsibility for calling `close()` is on the user**.
In C++, the very fact of leaving the scope is the cleanup.
**"A Facade makes one window" and "RAII puts cleanup in one place" look like
separate matters, but in this form they live together in one single type.**

### Make the caller call `init()`, or do it in the constructor

Because this is RAII, doing it in the constructor is the rule. But there are two exceptions.

1. When **the object is in static storage** (microcontrollers).
   Right after startup, neither the clock nor the stack is ready, so
   you must not touch peripherals at static initialization time. We cover this in 15.9
2. When **you want to return failure as a return value** but exceptions cannot be used.
   A constructor has no return value

For the second, this exercise takes the form **"the constructor does not throw even on failure, and you ask with `is_ready()`"**.
It is a design that allows the state "constructed, but not operational".
Opinions differ. If you want to be strict, use

```cpp
static std::optional<RobotSession> create(StartupConfig config);   // nullopt on failure
```

a factory like this and make the constructor `private`; then
the invariant "if a `RobotSession` exists, startup is done" is guaranteed by the type.
**For a team library, this one causes fewer accidents.**
The exercise uses the `is_ready()` approach so that the tests can observe the rollback on failure.

## 15.4 Who owns what

A Facade is a pattern that "bundles subsystems", so
you must always decide **whether the Facade owns the subsystems or only borrows them**.
In Java, this question does not arise thanks to the GC.

There are three ways.

| Form | How to write | When to use |
| --- | --- | --- |
| **Own (value member)** | `PowerRail power_;` | The subsystem is dedicated to the Facade. **This first** |
| **Own (`unique_ptr`)** | `std::unique_ptr<PowerRail> power_;` | You do not want to expose the type in the header (Pimpl), or you need polymorphism |
| **Borrow (reference / raw pointer)** | `PowerRail & power_;` | When you **can guarantee** the subsystem outlives the Facade |

The third is the same trap as chapter 14. If the one you borrowed from dies first, it is undefined behavior.
A design of "a Facade only gathers things, so it does not own them" sounds natural, but
**do not choose it if you cannot write down who guarantees the lifetime**.

The exercise's `RobotSession` holds no subsystem at all.
Its state is only "how far we initialized".

```cpp
int completed_stages_;   ///< 0 to 4
```

**If this is enough, this is the best.** When the subsystems are the hardware itself
(a power IC, an IMU, a CAN controller) and there is no entity to hold as an object on the C++ side,
the only state the Facade should hold is "how far we got".

## 15.5 A C++-specific danger: rollback when something fails midway

**This is where the most accidents happen in this chapter.**

If the 3rd of the 4 steps fails, you need to **undo the 2 that are already done, in reverse order**.
It is the same in Java, but C++ has two tools, and choosing wrong breaks things.

### (a) Count the stages and gather the cleanup in one place

This is what you write in the exercise.

```cpp
void teardown(int completed_stages, std::vector<std::string> * log)
{
  if (completed_stages >= 4) { link_down(log); }
  if (completed_stages >= 3) { calibration_clear(log); }
  if (completed_stages >= 2) { sensor_deinit(log); }
  if (completed_stages >= 1) { power_off(log); }
}
```

The key is to line up the `if`s in descending order of `>=`, as **independent `if`s, not fall-through**.
When a stage is added, this is the one place you fix.

In the free-function version (`start_once`), **you have to call this yourself**.

```cpp
if (!calibrate(config, log)) {
  teardown(2, log);                                       // <- leak if you forget
  return StartupResult{false, StartupStage::kCalibration};
}
```

There are 5 `return`s, and **every one of them needs `teardown()`**.
If you forget one, nobody tells you. This is the motivation for the RAII version.

```cpp
RobotSession::~RobotSession()
{
  teardown(completed_stages_, log_);   // there is no way to forget to call it
}
```

**Even if you `return` in the middle of the constructor, the destructor always runs.**
This is because the object is already constructed (it is a different story if you *throw an exception* during construction;
then the destructor does not run, and instead only the already-constructed members are destroyed).

### (b) Line up the subsystems as RAII members

If the subsystems have a real existence as C++ objects, this one is stronger.

```cpp
class Robot
{
public:
  explicit Robot(bool calibration_succeeds)
  : power_("power", true),
    sensor_("sensor", power_.ok()),
    calib_("calib", sensor_.ok() && calibration_succeeds),
    link_("link", calib_.ok())
  {
  }

private:
  Stage power_;    // constructed in declaration order,
  Stage sensor_;
  Stage calib_;
  Stage link_;     // destroyed in reverse order
};
```

**There is not one line of rollback code.** The language rule that members are constructed in declaration order and destroyed in reverse order
becomes the "initialization order" and the "cleanup order" as it is.

There are two costs.

1. **All members are constructed.** Even if the 3rd stage fails, the constructor of the 4th stage runs.
   So each `Stage` must decide by itself to do nothing when it does not hold anything (`ok_ == false`).
   This is why `link FAILED` appears right after `calib FAILED` in the output of 15.8
2. **The declaration order of members becomes the specification.** If someone just refactors with
   "I sorted them alphabetically", the initialization order changes and it breaks.
   **Write a comment: `// declaration order = initialization order. do not reorder`**

Also, the order of the initializer list follows the declaration order of the members, **not the order you wrote**.
If the written order differs from the declaration order, `-Wreorder` (included in `-Wall`) appears. Do not ignore it.

### Add `explicit`

```cpp
explicit RobotSession(StartupConfig config, std::vector<std::string> * log = nullptr);
```

It has a default argument, so it can effectively be called with one argument. Without `explicit`,

```cpp
void arm(const RobotSession & session);
arm(StartupConfig{});     // <- this compiles. The robot starts here
```

**You thought you were calling a function, and the hardware starts.**
The heavier the side effects of a type, the more `explicit` is a must. The exercise tests check this with
`std::is_convertible<StartupConfig, RobotSession>`.

### Copy forbidden, move allowed

`RobotSession` represents "one started piece of hardware". Copying has no meaning.
If it could be copied, **cleanup would run twice** when both copies are destroyed.

```cpp
RobotSession(const RobotSession &) = delete;
RobotSession & operator=(const RobotSession &) = delete;
```

We allow move. It would be inconvenient if you could not "return a started session from a function".
But **always empty the moved-from object**.

```cpp
RobotSession::RobotSession(RobotSession && other) noexcept
: config_(std::move(other.config_)),
  log_(other.log_),
  completed_stages_(other.completed_stages_),
  ready_(other.ready_),
  failed_stage_(other.failed_stage_)
{
  other.log_ = nullptr;
  other.completed_stages_ = 0;    // <- if you forget this, cleanup runs twice
  other.ready_ = false;
}
```

**The destructor also runs for an object after `std::move`.**
It is not "it disappeared because it was moved". It is "its contents became empty, and it is still there".
If you misunderstand this, the power goes off twice.

We made move assignment `= delete`. Move assignment needs a decision:
"when, and in what order, do we shut down the started state that the assignment target already holds?"
If you do not need that decision, **it is safer not to write it**.
Note that once you write the move constructor yourself, the copy operations and the move assignment are implicitly deleted,
but for the same reason as chapter 14, **write them explicitly**.

## 15.6 Reduce the surface you show: what you do not write in the header

**The essence of a Facade is not "making one window" but "reducing the surface you show".**
Even if you make one window, if all the subsystems are listed in the header, nothing has been reduced.

What appears in the exercise header `drill/robot_startup.hpp` is

- `StartupConfig` / `StartupStage` / `StartupResult`
- `robot::start_once()`
- `robot::RobotSession`

only. `power_on` / `sensor_init` / `calibrate` / `link_up`, and
their 4 cleanups live in the unnamed namespace in the `.cpp`.

```cpp
namespace robot
{
namespace
{
bool power_on(const StartupConfig & config, std::vector<std::string> * log);
void power_off(std::vector<std::string> * log);
// ...
}  // namespace
}  // namespace robot
```

If you put them in an unnamed namespace they get internal linkage, and **the names do not exist for other translation units**.
This is stronger hiding than Java's `package` scope. You get three things.

1. **Users cannot call them directly by mistake.** "Turn on only the power and do not initialize the sensor" cannot happen
2. **Fewer files include the header.** Only the `.cpp` side sees the SPI and register definition headers
3. **Changing the order does not affect users.** The signatures are not public API

### Relation to Pimpl (chapter 9)

What it does goes in the same direction as Pimpl: **"push the implementation out of the header"**.
The purpose is different.

| | Facade | Pimpl |
| --- | --- | --- |
| Purpose | Make **usage** simple | Cut **compile dependencies** |
| What it hides | The very existence of the subsystems | The types of the class members |
| Number of public types | Decreases (many -> 1) | Unchanged (1 -> 1) |
| Runtime cost | Zero | One pointer indirection + one heap allocation |

If **the Facade holds almost no state**, as in the exercise,
an unnamed namespace is enough and you do not need Pimpl.
Only when you want the Facade to hold subsystems as value members and also do not want to expose their types in the header
do you add Pimpl. **The order is this: unnamed namespace first.**

## 15.7 Does the standard library or the language have the same thing

**Yes, and a lot of it.** The standard library is full of Facades.

| Standard Facade | What it hides |
| --- | --- |
| `std::fstream` | `open` / `read` / `write` / `close` + buffer management. **The textbook Facade with RAII** |
| `std::filesystem::copy_file` | `stat` / `open` x 2 / `read` / `write` / `close` x 2 / copying permissions |
| `std::stoi` | `strtol` + checking `errno` + range checks |
| `std::async` | Thread creation + passing the result + forwarding exceptions |
| `std::lock_guard` | `lock` / `unlock` (pure RAII rather than a Facade) |

It is worth looking at `std::fstream`.

```cpp
{
  std::ofstream file{"log.csv"};    // open
  file << "t,v\n";                  // write (through the buffer)
}                                   // flush and close
```

**"One window", "automatic cleanup", and "the internal FILE* is not visible"** are all there.
The `RobotSession` we wrote in 15.3 applies this same shape to the robot's startup sequence.
When in doubt, copy the shape of `fstream`.

`std::stoi` is also an easy example. Written by hand, it looks like this.

```cpp
errno = 0;
char * end = nullptr;
const long value = std::strtol(text, &end, 10);
if (end == text || errno == ERANGE || value > INT_MAX || value < INT_MIN) {
  // ...
}
```

This became the single line `std::stoi(text)`. **A Facade is not a tool to reduce lines, but
a tool to reduce "places where you can make mistakes".** The code above has 4 ways to go wrong.

### "If it exists, do not write it"

It is exactly the checklist item of chapter 0. **Look for the standard one before you write your own.**
If you write your own "Facade that copies one file", that is `std::filesystem::copy_file`.

## 15.8 Try it yourself

This is the form of 15.5 (b), "line up the subsystems as RAII members".
**Predict the output** before you run it. In particular, guess
whether the `link` line appears in the run where `calib` fails,
and if it does, what appears.

```cpp
#include <iostream>
#include <string>

namespace
{

int indent = 0;

void trace(const std::string & text)
{
  for (int i = 0; i < indent; ++i) {
    std::cout << "  ";
  }
  std::cout << text << "\n";
}

/// RAII for one subsystem. If ok_ is false, it means "does not hold anything".
class Stage
{
public:
  Stage(const char * name, bool succeeds)
  : name_(name), ok_(succeeds)
  {
    trace(ok_ ? std::string{name_} + " up" : std::string{name_} + " FAILED");
  }

  ~Stage()
  {
    if (ok_) {
      trace(std::string{name_} + " down");
    }
  }

  Stage(const Stage &) = delete;
  Stage & operator=(const Stage &) = delete;

  bool ok() const { return ok_; }

private:
  const char * name_;
  bool ok_;
};

/// A Facade that is just members lined up. The language handles order and cleanup.
class Robot
{
public:
  explicit Robot(bool calibration_succeeds)
  : power_("power", true),
    sensor_("sensor", power_.ok()),
    calib_("calib", sensor_.ok() && calibration_succeeds),
    link_("link", calib_.ok())
  {
  }

  bool ready() const { return link_.ok(); }

private:
  Stage power_;
  Stage sensor_;
  Stage calib_;
  Stage link_;
};

}  // namespace

int main()
{
  trace("--- all succeed ---");
  indent = 1;
  {
    const Robot robot{true};
    trace(std::string{"ready="} + (robot.ready() ? "true" : "false"));
  }
  indent = 0;

  trace("--- calib fails ---");
  indent = 1;
  {
    const Robot robot{false};
    trace(std::string{"ready="} + (robot.ready() ? "true" : "false"));
  }
  indent = 0;

  trace("--- left ---");
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: when <code>calib</code> fails, does the <code>link</code> line appear</summary>

```
--- all succeed ---
  power up
  sensor up
  calib up
  link up
  ready=true
  link down
  calib down
  sensor down
  power down
--- calib fails ---
  power up
  sensor up
  calib FAILED
  link FAILED
  ready=false
  sensor down
  power down
--- left ---
```

**`link FAILED` does appear.** This is where the thinking differs from Java.
It is not "the 3rd stage failed, so the 4th is not created".
**All members are constructed.** The constructor of `link_` always runs,
and the `calib_.ok()` it was given was `false`, so it just "did not hold anything".

And only 2 `down` lines appear, `sensor` and `power`.
**The destructors of the members that held nothing do run, but they do nothing inside.**
"Whether the destructor runs" and "whether cleanup happens" are different matters, and
the latter is decided by each `Stage` with its own `ok_`.

The `down` order is `link -> calib -> sensor -> power` because
members are destroyed **in reverse declaration order**.
If you move `Stage power_;` to the very bottom of the declarations, the power goes off first and
then the sensor is stopped. **The order of the members is the specification.**
</details>

## 15.9 Conclusion for microcontrollers

There are three policies.

1. **A Facade without state is a namespace + free functions.** Do not write a class with only `static` members
2. **Put a Facade with state in static storage, only one.** Do not use the heap
3. **Do not touch peripherals in the constructor. Make the caller call `init()` explicitly**

Number 3 is the biggest difference from macOS / ROS 2. Here is the reason.

The constructor of a global object runs **before** `main()`
(the startup code runs it, like `__libc_init_array`).
At that point there is no guarantee that clock setup has finished.
It leads to the accident of **"right after boot, the PLL is not up yet, and you hit SPI"**.
Also, the initialization order against other globals is **unspecified** across translation units (the topic of chapter 5, Singleton).

So **make the constructor a `constexpr` that does nothing, and
call `init()` from `main()` after the clock setup**.

```cpp
#include <cstdio>

namespace
{

// --- What the Facade hides. Not shown in the header -------------------------
// Not a class. The state is on the peripheral register side, and the C++ side does not need to hold it.

bool clock_enable()
{
  std::printf("clock on\n");
  return true;
}
void clock_disable() { std::printf("clock off\n"); }

bool power_rail_on(int battery_mv)
{
  if (battery_mv < 11000) {
    std::printf("power FAILED (%d mV)\n", battery_mv);
    return false;
  }
  std::printf("power on\n");
  return true;
}
void power_rail_off() { std::printf("power off\n"); }

bool imu_init()
{
  std::printf("imu init\n");
  return true;
}
void imu_deinit() { std::printf("imu deinit\n"); }

bool can_open()
{
  std::printf("can open\n");
  return true;
}
void can_close() { std::printf("can close\n"); }

constexpr int kStageCount = 4;

/// It has state (how far we initialized), so only this is a class.
/// Does not use the heap. Does not use std::string or std::optional.
class RobotFacade
{
public:
  /// Does nothing. Even in static storage it does not depend on other static objects.
  constexpr RobotFacade() = default;

  ~RobotFacade() { deinit(); }

  RobotFacade(const RobotFacade &) = delete;
  RobotFacade & operator=(const RobotFacade &) = delete;

  /// Call it explicitly from main **after** the clock is up and the stack is ready.
  bool init(int battery_mv)
  {
    if (!clock_enable()) {
      return false;
    }
    ++stages_;
    if (!power_rail_on(battery_mv)) {
      return false;
    }
    ++stages_;
    if (!imu_init()) {
      return false;
    }
    ++stages_;
    if (!can_open()) {
      return false;
    }
    ++stages_;
    return true;
  }

  void deinit()
  {
    if (stages_ >= 4) { can_close(); }
    if (stages_ >= 3) { imu_deinit(); }
    if (stages_ >= 2) { power_rail_off(); }
    if (stages_ >= 1) { clock_disable(); }
    stages_ = 0;
  }

  bool is_ready() const { return stages_ == kStageCount; }

private:
  int stages_ = 0;
};

}  // namespace

// One in static storage. Zero allocations.
RobotFacade g_robot;

int main()
{
  std::printf("--- 12.0 V ---\n");
  if (g_robot.init(12000)) {
    std::printf("ready\n");
  }
  g_robot.deinit();

  std::printf("--- 10.5 V ---\n");
  if (!g_robot.init(10500)) {
    std::printf("not ready\n");
  }
  g_robot.deinit();
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti mcu.cpp -o mcu && ./mcu
```

```
--- 12.0 V ---
clock on
power on
imu init
can open
ready
can close
imu deinit
power off
clock off
--- 10.5 V ---
clock on
power FAILED (10500 mV)
not ready
clock off
```

In the run where the voltage is too low, check that **`clock off` appears but `power off` does not**.
`stages_` stopped at 1, so it does not go to release what it does not hold.

Five points.

- **`constexpr RobotFacade() = default;`.** The constructor does nothing, so
  `g_robot` needs only static initialization (just zeroing `.bss`), and
  there is no code that runs before `main()`. **The static initialization order problem disappears**
- **Call `init()` explicitly from `main()`.** It departs from the RAII principle, but
  on a microcontroller you cannot control when the constructor runs, so this is the right answer
- **Write the destructor, but do not count on it.** Embedded `main()` normally
  never exits with `while (true)`, so `~RobotFacade()` does not run.
  Make `deinit()` public so that sleep transitions and reboot sequences can call it
- **Do not use `std::string` / `std::optional` / exceptions.** Names are `const char *`,
  failure is `bool`, and if you need details, return an `enum`
- **Zero virtual functions.** A Facade does not need polymorphism. **If you want to swap subsystems,
  that is Strategy (chapter 10) or Bridge (chapter 9), not a Facade**

### Should `init()` be callable twice

The code above can call `init()` again after `deinit()`.
**If you do not want that, reject it at the top of `init()`.**

```cpp
bool init(int battery_mv)
{
  if (stages_ != 0) {
    return is_ready();     // already initialized. Do not initialize twice
  }
  // ...
}
```

What actually happens in your team's code is "the error recovery code calls `init()` again".
**Write one line in the header comment about which specification you chose.**

## 15.10 Conclusion for ROS 2 (supplement)

rclcpp itself is a pile of Facades.

- `rclcpp::init()`: a window that bundles the initialization of rcl / rmw / DDS. **Exactly `PageMaker`**
- `Node::create_publisher<T>()`: bundles the QoS conversion, getting the type support, and creating the rmw publisher
- `rclcpp::spin(node)`: bundles creating the executor, adding to it, and running the loop

If you write your own, **do not make the node the Facade**. A common failure is

```cpp
// Bad example: the node has become the window for everything
class RobotNode : public rclcpp::Node
{
  // 10 publishers, 8 subscribers, 4 services, a state machine, a control law, log formatting...
};
```

This is not a Facade but **just a huge class**. A Facade was supposed to be thin.
Cut the control law and the hardware abstraction out into plain C++ classes that do not depend on ROS 2,
and make the node "only connect topics to those plain classes".
**If you do that, the same class can be used as it is on the microcontroller side.**
This is why all the exercises of this track are written in plain C++17.

Wrapping `rclcpp::init` / `shutdown` in RAII is effective.

```cpp
class RclcppContext
{
public:
  RclcppContext(int argc, char ** argv) { rclcpp::init(argc, argv); }
  ~RclcppContext() { rclcpp::shutdown(); }
  RclcppContext(const RclcppContext &) = delete;
  RclcppContext & operator=(const RclcppContext &) = delete;
};
```

The accident where you forget to call `shutdown()` in a test and the next test fails goes away.

## 15.11 Common pitfalls

| Symptom | Cause |
| --- | --- |
| You want to write `RobotStarter::start()` | A Java habit. If there is no state, use a namespace + free function |
| Adding one function to the header recompiled everything | You made a class with only `static` members. With a namespace you can split it |
| Startup failed but the power stays on | You did not write the rollback. Every `return` path needs `teardown()` |
| Cleanup runs twice (the power goes off twice) | You did not empty the moved-from object. The destructor runs even after `std::move` |
| The robot started just because I called a function | The constructor has no `explicit`, and an implicit conversion happened |
| I reordered the members and the initialization order changed and broke | Declaration order of members = initialization order. Do not ignore `-Wreorder` |
| The 3rd stage failed but the log of the 4th stage appears | All members are constructed. Each stage looks at `ok_` and chooses to "do nothing" |
| It hangs before `main()` (microcontroller) | A global constructor touches peripherals. Split out `init()` |
| The destructor is not called (microcontroller) | `main()` never exits with `while (true)`. Make `deinit()` public |
| The Facade became 2000 lines | You wrote decisions and control laws in the Facade. A Facade is only "the order of calls" |
| I want to swap the subsystems | That is not the job of a Facade. Use Strategy (chapter 10) or Bridge (chapter 9) |

## 15.12 Matching exercise

```bash
./drill run dp15
```

In `exercises/dp15_facade/src/robot_startup.cpp`, you implement the window for the robot's startup sequence
(power on -> sensor init -> calibration -> link up).

1. **8 subsystems** (`power_on` / `power_off` / `sensor_init` / `sensor_deinit` /
   `calibrate` / `calibration_clear` / `link_up` / `link_down`):
   free functions in an unnamed namespace. Do not show them in the header
2. **`teardown()`**: takes the number of completed stages and cleans up in **reverse** order
3. **`robot::start_once()`**: the namespace + free-function version of the Facade.
   Calls `teardown()` on every `return` path
4. **The constructor of `RobotSession`**: runs the startup sequence. **Do not write cleanup**
5. **The destructor of `RobotSession`**: just calls `teardown()`
6. **The move constructor of `RobotSession`**: empties the moved-from object
7. **`RobotSession::drive()`**: does nothing if startup did not succeed

The tests check

- creating the Facade just once runs the 4 internal steps, all of them, **in the correct order**
- leaving the scope runs cleanup **in reverse order**
- if something fails midway, nothing after it runs, and **only what was already initialized** is rolled back
  (fail at power -> zero cleanup / fail at sensor -> power only / fail at calibration -> 2 stages)
- **the logs of the namespace + free-function version and the RAII class version match exactly**
- even after a move, cleanup runs **exactly once**
- copy is forbidden, move construction is allowed, move assignment is forbidden, and it is `explicit` (`static_assert`)

## 15.13 Summary of this chapter

- Yuki's `PageMaker` is a product of "Java has no functions outside a class".
  In C++ make it a **namespace + free function**. **Do not write a class with only `static` members**
- The dividing line is one point: **"does it have state?"** If not, a function; if so, a class
- A Facade with state uses **RAII**. Initialize in the constructor, clean up in the destructor.
  **This is the real core of the C++ Facade**, and on the Java side the responsibility to call `close()` remains
- **Rollback** when something fails midway is work only on the C++ side.
  Either count the stages and gather them in `teardown()`, or line up the subsystems as RAII members
- Members are **constructed in declaration order and destroyed in reverse order**. Reordering is a spec change. Watch `-Wreorder`
- **Copy forbidden, move allowed.** If you do not empty the moved-from object, **cleanup runs twice**
- Always put **`explicit`** on the constructor of a type with heavy side effects
- The essence of a Facade is "reducing the surface you show". Put subsystems in **an unnamed namespace in the `.cpp`**.
  If that is not enough, add Pimpl (chapter 9)
- The standard library is full of Facades (`fstream` / `filesystem::copy_file` / `stoi` / `async`).
  **Look before you write your own**
- On microcontrollers, use a **`constexpr` constructor + an explicit `init()`**.
  Do not touch peripherals before `main()`. No heap and no virtual functions are needed
- A Facade is thin. **If you start writing decisions or swapping, it is a Strategy or a Bridge**

---

Previous: [14. Chain of Responsibility](14_ChainOfResponsibility.md) / Next: [16. Mediator](16_Mediator.md)
