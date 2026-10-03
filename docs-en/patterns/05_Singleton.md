# 5. Singleton

> **Corresponds to Chapter 5 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Open the `Singleton` class and `Main` from the book.
>
> **Goal of this chapter**: In Java, the Singleton is done in one line: `private static Singleton singleton = new Singleton();`.
> If you write the same one line in C++, it breaks, because **the initialization order across translation units is undefined**.
> C++ also has one more accident that Java does not have: the object can be copied.
> In this chapter, you will see why we rewrite it as a **Meyers Singleton** (a function-local static),
> by running code that really breaks.
> Before that, we put **the decision not to use it at all** first.

## 5.1 First, decide whether to use it at all

Here is what chapter 0, section 0.2 ([0. Before you use them](00_before_you_use_them.md)) said. I will say it again.

**A Singleton is often just a global variable with the name "design pattern".**

```cpp
// This is not a pattern. It is just a global variable
class Config
{
public:
  static Config & instance();
  int motor_max_duty = 100;
};
```

Count the problems this solved. The answer is **zero**.

| Problem of a global variable | With a Singleton |
| --- | --- |
| Anyone can change it from anywhere | Same |
| You cannot tell from the code who depends on it | Same (it does not appear in the arguments) |
| You cannot replace it in tests | Same |
| The state of the previous test leaks into the next | Same |
| It breaks with multiple threads | Only the initialization becomes safe. **Races on the contents stay** |

Only the guilt went away. That is the worst harm.

### Think first: can you pass it as an argument?

```cpp
// Singleton version: the dependency is not visible in the function signature
void drive_motor(double duty)
{
  const int limit = Config::instance().motor_max_duty;   // hidden dependency
  // ...
}

// Argument version: the type tells you what it depends on
void drive_motor(const Config & config, double duty)
{
  const int limit = config.motor_max_duty;
}
```

For the argument version, a test only needs to pass a different `Config`. You do not need `reset()`.
**If three functions use `Config`, pass it to the three.** That is all.

### Things that really are only one

There is only one criterion.

> **"Does it break if a second one can be constructed?"**

- `Config`: Nothing breaks with two. You just use one of them → **do not make it a singleton**
- `UART1`: Two `Uart` objects would hit the same registers, each thinking it has its own state → **it breaks**

Microcontroller peripherals (UART, SPI, I2C, timers, DMA channels) are
**physically one in the world**. This is the only legitimate use of a singleton.
"It is annoying to pass it around" is not a reason.

The rest of this chapter is for the case where you have already decided "there really is only one".

## 5.2 If you copy the Java version straight into C++

The `Singleton` in Hiroshi Yuki's book looks like this.

```java
public class Singleton {
    private static Singleton singleton = new Singleton();
    private Singleton() {
        System.out.println("An instance was created.");
    }
    public static Singleton getInstance() {
        return singleton;
    }
}
```

If you move it to C++ in the obvious way, you get this.

```cpp
// config.hpp
class Config
{
public:
  Config();
  int max_duty() const { return max_duty_; }

private:
  int max_duty_;
};

extern Config g_config;      // The definition is in config.cpp
```

```cpp
// config.cpp
Config g_config;             // Corresponds to the static field in Java
```

**This breaks.** There are three changes to make.

### Change 1: Stop using a global that corresponds to the static field (the main point)

In Java, `private static Singleton singleton = new Singleton();` is
**initialized when the class is first used** (the job of the class loader). The language specification says so.

C++ objects at namespace scope are different.
**Inside the same translation unit, they are initialized in the order you wrote them.**
But **across translation units, the order is undefined.** This is called the
**static initialization order fiasco**.

### Change 2: `= delete` the copy and move operations

In Java, `Singleton s = Singleton.getInstance();` only stores a reference.
In C++, **a copy is made**. We will do this in 5.4.

### Change 3: `getInstance()` returns a reference, not a pointer

```cpp
static Config * instance();      // The caller cannot tell whether they may delete it
static Config & instance();      // The type says "do not free it"
```

This is the same idea as `unique_ptr` in Chapter 1. **Declare ownership in the type.**

## 5.3 Break it for real: the static initialization order fiasco

We make three files.

```cpp
// config.hpp
#pragma once
#include <iostream>

class Config
{
public:
  Config()
  : max_duty_(100)
  {
    std::cout << "Config constructor\n";
  }
  int max_duty() const { return max_duty_; }

private:
  int max_duty_;
};

extern Config g_config;   // The definition is in config.cpp
```

```cpp
// config.cpp
#include "config.hpp"

Config g_config;          // A global in the translation unit config.cpp
```

```cpp
// limiter.cpp
#include "config.hpp"

class Limiter
{
public:
  Limiter()
  : limit_(g_config.max_duty())   // Uses a global from another translation unit
  {
    std::cout << "Limiter constructor: limit_ = " << limit_ << "\n";
  }
  int limit() const { return limit_; }

private:
  int limit_;
};

Limiter g_limiter;
```

```cpp
// main_siof.cpp
#include "config.hpp"

int main()
{
  std::cout << "main: g_config.max_duty() = " << g_config.max_duty() << "\n";
  return 0;
}
```

Build with `limiter.cpp` first.

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic limiter.cpp config.cpp main_siof.cpp -o siof1 && ./siof1
```

This is the actual output.

<!-- measure: files=config.hpp,config.cpp,limiter.cpp,main_siof.cpp -->
```
Limiter constructor: limit_ = 0
Config constructor
main: g_config.max_duty() = 100
```

**`limit_` is 0.** The constructor of `Limiter` ran
before the constructor of `Config`, and it read `g_config`, which was not constructed yet.
`max_duty_` was only zero-initialized.

It compiles. There is no warning. **It does not even crash when you run it.**
The motor limit should be 100, but it is 0, and all you hear is "it does not move".
This is the nastiest way to break.

### Even worse, changing the link order fixes it

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic config.cpp limiter.cpp main_siof.cpp -o siof2 && ./siof2
```

<!-- measure: files=config.hpp,config.cpp,limiter.cpp,main_siof.cpp -->
```
Config constructor
Limiter constructor: limit_ = 100
main: g_config.max_duty() = 100
```

**Without changing one character of the source, only the link order fixed it.**
So "it worked on my machine" guarantees nothing.
It comes back when you just add one file to a CMake target.

### The fix: Meyers Singleton

```cpp
// config2.hpp
#pragma once
#include <iostream>

class Config
{
public:
  static Config & instance()      // Meyers Singleton
  {
    static Config the_config;     // Constructed only the first time we pass here
    return the_config;
  }
  int max_duty() const { return max_duty_; }

private:
  Config()
  : max_duty_(100)
  {
    std::cout << "Config constructor\n";
  }
  int max_duty_;
};
```

On the `limiter.cpp` side, you only replace `g_config` with `Config::instance()`.
With either link order, you get this.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// config2.hpp
#pragma once
#include <iostream>

class Config
{
public:
  static Config & instance()      // Meyers Singleton
  {
    static Config the_config;     // Constructed only the first time we pass here
    return the_config;
  }
  int max_duty() const { return max_duty_; }

private:
  Config()
  : max_duty_(100)
  {
    std::cout << "Config constructor\n";
  }
  int max_duty_;
};
```

```cpp
// limiter2.cpp
#include "config2.hpp"

class Limiter
{
public:
  Limiter()
  : limit_(Config::instance().max_duty())   // Uses a global from another translation unit
  {
    std::cout << "Limiter constructor: limit_ = " << limit_ << "\n";
  }
  int limit() const { return limit_; }

private:
  int limit_;
};

Limiter g_limiter;
```

```cpp
// main_meyers.cpp
#include "config2.hpp"

int main()
{
  std::cout << "main: " << Config::instance().max_duty() << "\n";
  return 0;
}
```

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic limiter2.cpp main_meyers.cpp -o meyers1 && ./meyers1
c++ -std=c++17 -Wall -Wextra -Wpedantic main_meyers.cpp limiter2.cpp -o meyers2 && ./meyers2
```

</details>

<!-- measure: files=config2.hpp,limiter2.cpp,main_meyers.cpp cmd="(c++ -std=c++17 -Wall -Wextra -Wpedantic limiter2.cpp main_meyers.cpp -o meyers1 && ./meyers1) > meyers1.txt; (c++ -std=c++17 -Wall -Wextra -Wpedantic main_meyers.cpp limiter2.cpp -o meyers2 && ./meyers2) > meyers2.txt; cmp meyers1.txt meyers2.txt && cat meyers1.txt" -->
```
Config constructor
Limiter constructor: limit_ = 100
main: 100
```

**Why it works.** The standard says a function-local `static` is
**initialized the first time execution reaches that line**.
The moment of initialization changed from "somewhere at program start" to "the moment the user calls it".
So **it is guaranteed to be initialized before use**. Humans no longer have to manage the order.

For the explanation of `static` lifetime and linkage themselves, see
[C++ Basics 7. static](../cpp-basics/07_static.md). I will not repeat it here.

### It is also thread-safe

Since C++11, **the standard guarantees that initialization of a function-local static is thread-safe**
(often called "magic statics").
Even if many threads enter `instance()` at the same time, the construction happens only once,
and the other threads wait until the construction is done.

```cpp
// You do not need this. It only makes things slower
static std::mutex g_mutex;
Config & Config::instance()
{
  std::lock_guard<std::mutex> lock(g_mutex);   // not needed
  static Config the_config;
  return the_config;
}
```

Articles about "double-checked locking" written in the C++03 era are still around,
but **since C++11 you do not need it**. Just put one `static` line.

**But only the initialization is protected.** If many threads call
`set_baud_rate()` after construction, that is an ordinary data race. You must protect that yourself.

## 5.4 An accident that does not happen in Java: it can be copied

This is a danger only C++ has. If you miss it, the singleton quietly falls apart.

```cpp
Config copied = Config::instance();   // ← In Java, this only stores a reference. In C++, it is a copy
```

Even if you make the constructor `private`, **the copy constructor is implicitly generated, and it is public**.
So you thought you made it "impossible to create from outside", but **you can still duplicate the existing instance**.

The same accident happens just by taking it by value by mistake.

```cpp
void configure(Config config);        // By value, not by reference. Copied here
auto config = Config::instance();     // auto is deduced as Config, not Config&
```

The third one is especially common. `auto` drops the reference. You should write `auto &`.

The fix is four lines.

```cpp
class Config
{
public:
  static Config & instance();

  Config(const Config &) = delete;
  Config & operator=(const Config &) = delete;
  Config(Config &&) = delete;
  Config & operator=(Config &&) = delete;

private:
  Config() = default;
};
```

If you `= delete` the two copy operations, the move operations are not generated implicitly,
but **please write all four.** It tells the reader that you prohibited them on purpose.

## 5.5 Try it yourself

It is complete in one file. **Predict the output** before you run it.

```cpp
// try.cpp
#include <iostream>

class Config
{
public:
  static Config & instance()
  {
    static Config the_config;
    return the_config;
  }
  void set_max_duty(int duty) { max_duty_ = duty; }
  int max_duty() const { return max_duty_; }

private:
  Config() = default;
  int max_duty_ = 100;
};

int main()
{
  Config copied = Config::instance();     // A copy is made
  copied.set_max_duty(30);

  std::cout << "instance: " << &Config::instance()
            << " duty=" << Config::instance().max_duty() << "\n";
  std::cout << "copied  : " << &copied
            << " duty=" << copied.max_duty() << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// try_deleted.cpp
#include <iostream>

class Config
{
public:
  static Config & instance()
  {
    static Config the_config;
    return the_config;
  }
  void set_max_duty(int duty) { max_duty_ = duty; }
  int max_duty() const { return max_duty_; }

  Config(const Config &) = delete;
  Config & operator=(const Config &) = delete;

private:
  Config() = default;
  int max_duty_ = 100;
};

int main()
{
  Config copied = Config::instance();     // A copy is made
  copied.set_max_duty(30);

  std::cout << "instance: " << &Config::instance()
            << " duty=" << Config::instance().max_duty() << "\n";
  std::cout << "copied  : " << &copied
            << " duty=" << copied.max_duty() << "\n";
  return 0;
}
```

```bash
clang++ -std=c++17 -Wall -Wextra -Wpedantic try_deleted.cpp -o try_deleted
```

</details>

<details>
<summary>Predict: Does it compile? If it does, what happens to the two addresses and the duty?</summary>

**It compiles. There is no warning.** This is the output (the addresses change with the environment).

<!-- measure: files=try_deleted.cpp cmd="grep -v '= delete' try_deleted.cpp > try_without_delete.cpp && g++ -std=c++17 -Wall -Wextra -Wpedantic try_without_delete.cpp -o try_without_delete && ./try_without_delete" -->
```
instance: 0x555555558010 duty=100
copied  : 0x7ffffffc5904 duty=30
```

The addresses are different. **Two `Config` objects exist.**
`copied.set_max_duty(30)` changed only the copy,
so the original is still 100. This is the cause of the bug "I set the value, but it is not applied".

Add the next two lines just before `private:`, and build again.

```cpp
  Config(const Config &) = delete;
  Config & operator=(const Config &) = delete;
```

This is the actual error (Apple clang 21).

<!-- measure: env=clang -->
```
try_deleted.cpp:25:10: error: call to deleted constructor of 'Config'
   25 |   Config copied = Config::instance();     // A copy is made
      |          ^        ~~~~~~~~~~~~~~~~~~
try_deleted.cpp:15:3: note: 'Config' has been explicitly marked deleted here
   15 |   Config(const Config &) = delete;
      |   ^
1 error generated.
```

**Only when you write `= delete` does the compiler stop the accident for you.**
If you do not write it, you can notice it only at run time.
</details>

## 5.6 Another order: it breaks at destruction

Even if you solve the initialization order with a Meyers Singleton, the **destruction order** is still there.
Function-local statics are destroyed in the reverse order of construction.
If you have two singletons and the destructor of one uses the other, it breaks.

```cpp
class Logger
{
public:
  static Logger & instance()
  {
    static Logger the_logger;
    return the_logger;
  }
  ~Logger() { alive_ = false; }

  void log(const std::string & message)
  {
    std::cout << "[log alive=" << alive_ << "] " << message << "\n";
  }

private:
  Logger() = default;
  bool alive_ = true;
};

class Uart
{
public:
  static Uart & instance()
  {
    static Uart the_uart;
    return the_uart;
  }
  ~Uart() { Logger::instance().log("Closed the Uart"); }   // It may already be destroyed

private:
  Uart() = default;
};
```

This is the actual output when `main` touches `Uart::instance()` first and `Logger::instance()` after it.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// destruction_order.cpp
#include <iostream>
#include <string>

class Logger
{
public:
  static Logger & instance()
  {
    static Logger the_logger;
    return the_logger;
  }
  ~Logger() { alive_ = false; }

  void log(const std::string & message)
  {
    std::cout << "[log alive=" << alive_ << "] " << message << "\n";
  }

private:
  Logger() = default;
  bool alive_ = true;
};

class Uart
{
public:
  static Uart & instance()
  {
    static Uart the_uart;
    return the_uart;
  }
  ~Uart() { Logger::instance().log("Closed the Uart"); }   // It may already be destroyed

private:
  Uart() = default;
};

int main(int argc, char *[])
{
  const bool is_logger_first = argc > 1;
  if (is_logger_first) {
    Logger::instance();   // touch Logger first if there is an argument
    Uart::instance();
  } else {
    Uart::instance();     // touch Uart first if there is no argument
    Logger::instance();
  }
  std::cout << "main finished\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic destruction_order.cpp -o destruction_order
./destruction_order              # touch Uart first
./destruction_order logger_first # touch Logger first
```

</details>

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic destruction_order.cpp -o destruction_order && ./destruction_order" -->
```
main finished
[log alive=0] Closed the Uart
```

`alive=0` means that **we are touching a `Logger` whose destructor already ran**.
This is undefined behavior. This time the value happened to be readable,
but if it had a `std::string` or a `std::vector`, it would touch freed memory.

If you touch `Logger::instance()` first and `Uart::instance()` after it, you get this.

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic destruction_order.cpp -o destruction_order && ./destruction_order logger_first" -->
```
main finished
[log alive=1] Closed the Uart
```

**The code is the same. Only the order of touching is different.** The same shape as 5.3 comes back on the destruction side.

### What to do

| Method | How to write it | Cost |
| --- | --- | --- |
| Do not use other singletons in a destructor | Move the cleanup into an explicit `close()` | You may forget to call it |
| Construct the user first | Call `Logger::instance();` at the top of `main` | A convention. It comes back if you forget it |
| **Do not free it on purpose (intentional leak)** | `static Logger * p = new Logger(); return *p;` | The destructor **does not run** |

For the third one, you probably thought "isn't that a leak?" You are right, but
**the OS reclaims everything when the process exits**, so there is no real harm.
The judgment is that this is much better than "stepping on undefined behavior because of destruction order".
The person who proposed the Meyers Singleton also mentions this form (a Nifty Counter or an intentional leak) depending on the situation.

**Guideline**: If the destructor really has something to do (close a file, stop hardware), use
the first or the second. If the destructor is practically empty, the third is fine.
**Decide "what the destructor does" first, then choose.**

## 5.7 Testability: state leaks into tests

This is the most practical harm of a singleton.

```cpp
TEST(UartTest, CanSetBaudRate)
{
  UartPort::instance().set_baud_rate(9600);
  EXPECT_EQ(UartPort::instance().baud_rate(), 9600u);
}

TEST(UartTest, DefaultBaudRateIs115200)
{
  EXPECT_EQ(UartPort::instance().baud_rate(), 115200u);   // fails
}
```

The second one fails. The 9600 written by the first one is still there.
**The tests are not independent.** The result changes if you change the run order.
gtest can shuffle the order with `--gtest_shuffle`, so this fails at random in CI.

There are roughly two ways.

### Way 1: Provide `reset()` (treating the symptom)

```cpp
class UartPortTest : public ::testing::Test
{
protected:
  void SetUp() override { UartPort::instance().reset(); }
};
```

The exercise asks you to implement this one. But **`reset()` is a function that production code does not need.**
You pay the cost of "one more public API for the sake of tests".
Also, if you forget to reset some member in `reset()`, the leak stays.

### Way 2: Split out an interface and pass it by reference (the real answer)

```cpp
class ISerialPort
{
public:
  virtual ~ISerialPort() = default;
  virtual void write_line(const std::string & line) = 0;
};

// The user does not know about the singleton
void report_status(ISerialPort & port, int battery_mv);
```

In production, you call `report_status(UartPort::instance(), mv);`.
In tests, you pass a `FakeSerialPort`. **You do not need `reset()`.**
You just create a new `FakeSerialPort` for each test.

But go back to section 0.1 of [0. Before you use them](00_before_you_use_them.md).
**There are two implementations (production and fake), so this is a legitimate abstraction.**
The condition in that section was: if you can write "I want to replace it for tests" as the reason, you may add it.

**In summary**: Make the singleton only "the mechanism that hands out the one instance",
and **let the users receive it through an interface.** This is the form that gives you both.

## 5.8 Conclusion for microcontrollers

**Peripherals are practically singletons. You may use it. But keep these four rules.**

### 1. Do not use the heap

```cpp
// Bad: uses the heap. Needs malloc. It can fail
Uart & Uart::instance()
{
  static Uart * port = new Uart();
  return *port;
}

// Good: placed in .bss. Zero heap
Uart & Uart::instance()
{
  static Uart the_uart;
  return the_uart;
}
```

We basically do not use the "intentional leak" of 5.6 on microcontrollers.
The process never exits in the first place, so the destruction order problem itself does not occur.

Also, a function-local static has a **guard variable** (an "already initialized" flag),
and every call has one extra branch. If you really want to cut it,
you can make a `constexpr` constructor and place the object at namespace scope (it is constant initialization, so there is no order problem).
But **measure first.** It is one branch.

### 2. Do not touch hardware in the constructor

This is the one people get wrong most.

```cpp
// Bad
Uart::Uart()
{
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;   // Is the clock already enabled?
  USART1->CR1 |= USART_CR1_UE;
  NVIC_EnableIRQ(USART1_IRQn);            // An interrupt can come right now
}
```

When `instance()` is called for the first time **depends on the caller**.
It may be before the clock setup. The moment you enable the interrupt, the ISR runs,
and if that ISR calls `Uart::instance()`, **it re-enters the function-local static that is still being constructed**.
(By the standard, re-entering during initialization is undefined behavior.)

```cpp
// Good: separate construction from hardware initialization
class Uart
{
public:
  static Uart & instance()
  {
    static Uart the_uart;
    return the_uart;
  }

  void open(std::uint32_t baud_rate);   // Only here do we touch the registers for the first time
  void close();

private:
  Uart() = default;                     // Only builds the state in memory
};
```

At the top of `main`, after the clock setup, call `Uart::instance().open(115200);`.
We went back to **a form where a human can decide "when it is initialized".**

### 3. Registers are `volatile`. But that is a different topic from singletons

```cpp
volatile std::uint32_t * const uart_dr =
  reinterpret_cast<volatile std::uint32_t *>(0x40011004);
```

All `volatile` says is: "compiler, do not remove or reorder this access."
**`volatile` is neither thread-safe nor interrupt-safe.**
Mixing these up is a classic accident. If you need synchronization, see rule 4.

### 4. If you touch it from an ISR, think about data races

```cpp
void USART1_IRQHandler()
{
  Uart::instance().on_rx_byte(...);      // The main loop also touches it
}
```

What `instance()` returns is one object.
**If the main loop and the ISR touch the same member, there is a race.**

| Situation | What to do |
| --- | --- |
| Only raise a 1-byte flag | `std::atomic<bool>` (check that it is lock-free) |
| Ring buffer (one writer, one reader) | An SPSC queue with `std::atomic` indices |
| Anything more | A critical section (disable only the relevant interrupt for a short time) |

Disabling all interrupts is the last resort. It directly affects the control period.

**`std::mutex` basically cannot be used on microcontrollers** (it needs an OS, and an ISR cannot take it).
In 5.3 I wrote that "magic statics are thread-safe",
but that is only about initialization. **You must protect races with ISRs separately.**

## 5.9 Conclusion for ROS 2 (supplement)

**You can avoid it. Please avoid it.**

An rclcpp node is an object. You can give the settings as parameters, and pass the dependencies to the constructor.
"There is only one" is something decided at run time. It is not something to force with types.

```cpp
// Bad: make the node a singleton
auto & node = MyNode::instance();

// Good: create it normally and pass it
auto node = std::make_shared<MyNode>();
```

With components (`rclcpp_components`), it is normal that **two of the same node run in the same process**.
If you made it a singleton, it breaks at that moment.

On the other hand, the global context touched by `rclcpp::init()` / `shutdown()` is practically a singleton.
But rclcpp exposes it as an object called `rclcpp::Context`, and
**you can create multiple contexts.** There is just one default.
The design is **"one is enough, but we do not force it to be one"**, which is a good reference.

## 5.10 Common pitfalls

| Symptom | Cause |
| --- | --- |
| A value you set is not applied | You copy with `auto config = Config::instance();`. Use `auto &`. If you write `= delete`, it becomes a compile error |
| A value stays 0. It is fixed when you change the link order | Static initialization order fiasco. See 5.3. Use a Meyers Singleton |
| It crashes only at program exit / strange values appear | Destruction order. See 5.6. A destructor uses another singleton |
| A test passes alone but fails when you run all tests | The state of the previous test leaks. Call `reset()` in `SetUp()` |
| It fails at random with `--gtest_shuffle` | Same as above. The tests depend on the state of the singleton |
| `error: call to deleted constructor` | You take it by value. Take it as `Config &` |
| `instance()` uses `std::mutex` and is slow | Not needed since C++11. A function-local static alone is thread-safe |
| The microcontroller hangs right after startup | The constructor initializes hardware. Split it into `open()` |
| Values get corrupted when touched from an ISR | The initialization is thread-safe, but **you must protect races on the contents yourself** |

## 5.11 Matching exercise

```bash
./drill run dp05
```

In `exercises/dp05_singleton/src/uart_port.cpp`, you implement

1. `UartPort::instance()`: a Meyers Singleton
2. `UartPort::construction_count()`: shows that initialization happens only once
3. `UartPort::reset()`: **put the state back without recreating the object**
4. `LazyProbe::instance()` / `LazyProbe::was_constructed()`: check lazy initialization

The tests check that the same address is returned, that copy and move are
prohibited by `static_assert`, that the constructor runs only once, and that
`reset()` cuts the state between tests.

## 5.12 Summary of this chapter

- **First, think whether you can pass it as an argument.** A singleton easily becomes another name for a global variable
- You may use it only for **"things that break if a second one can be constructed"**. Microcontroller peripherals are such things
- If you turn Java's `private static` field straight into a C++ global,
  **the initialization order across translation units is undefined**. The result changes with the link order
- The standard answer is a **Meyers Singleton** (a function-local static). It is initialized at the moment of use, so the order problem disappears
- **Since C++11, initialization of a function-local static is thread-safe.** Do not write your own lock.
  But **only the initialization is protected**
- **`= delete` the copy and move operations.** If you do not, `auto config = Config::instance();` duplicates it.
  This accident does not happen in Java
- The destruction order is also undefined. Do not use other singletons in a destructor.
  If there is nothing to do, an **intentional leak** is also an option
- A singleton **leaks state between tests**. `reset()` treats the symptom, and
  the real answer is to **split out an interface and pass it by reference**
- Microcontrollers: do not use the heap / do not touch hardware in the constructor / protect races with ISRs separately
- ROS 2: you can avoid it. If two of the same node run as components, it breaks

---

Previous: [4. Factory Method](04_FactoryMethod.md) / Next: [6. Prototype](06_Prototype.md)
