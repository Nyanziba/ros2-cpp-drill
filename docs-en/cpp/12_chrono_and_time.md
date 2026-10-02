# 12. `std::chrono` and time

> **Goal of this chapter**: **`chrono_literals` appears 28 times** in the drill.
> Notation such as `500ms` and `2s` works only because of
> the single line `using namespace std::chrono_literals;`.
> This notation, where `ms` or `s` follows a number (a **user-defined literal**),
> is an advanced C++ feature, but in ROS 2 it is something you "use as a matter of course".
> Also, the time type (`std::chrono::duration`) treats different units as different types,
> so even simple addition can hit "pitfalls caused by type differences".
> Besides `std::chrono`, ROS 2 has its own time types (`rclcpp::Duration` / `rclcpp::Time`),
> and these **support simulation time**. If you do not know this difference,
> you get a mysterious bug: "it works on the real robot, but times out in the simulator".

## 12.1 User-defined literals — how `500ms` works

### `using namespace std::chrono_literals`

```cpp
using namespace std::chrono_literals;

auto delay = 500ms;  // std::chrono::milliseconds(500)
auto second = 1s;    // std::chrono::seconds(1)
auto microsec = 100us;  // std::chrono::microseconds(100)
```

The notation `500ms` is not **a reserved word that the C++ compiler has built in**.
It works because the namespace `std::chrono_literals` defines the **user-defined literal operator** `operator""ms`.

```cpp
// Inside the C++ standard library (simplified)
namespace std::chrono_literals {
  constexpr std::chrono::milliseconds operator""ms(unsigned long long count) {
    return std::chrono::milliseconds(count);
  }
}
```

`using namespace` brings the contents of that namespace into the global scope,
so `500ms` becomes valid.

You can also write it without user-defined literals.

```cpp
// Same meaning, but verbose
auto delay = std::chrono::milliseconds(500);
```

The drill's `solutions/01_publisher/src/minimal_publisher.cpp` uses this form.

```cpp
using namespace std::chrono_literals;
timer_ = this->create_wall_timer(500ms, std::bind(&MinimalPublisher::timer_callback, this));
```

### Available literals

```cpp
using namespace std::chrono_literals;

std::chrono::nanoseconds ns(100ns);         // nanoseconds
std::chrono::microseconds us(100us);        // microseconds
std::chrono::milliseconds ms(100ms);        // milliseconds
std::chrono::seconds s(100s);               // seconds
std::chrono::minutes min(100min);           // minutes
std::chrono::hours h(100h);                 // hours
```

## 12.2 `std::chrono::duration` — the time type

`duration` is a **template**, and it is decided by both the "number type" and the "time unit".

```cpp
template<typename Rep, typename Period>
class duration {
  // ...
};
```

It is simplified with typedefs, so you normally use the short type names below.

```cpp
std::chrono::milliseconds ms(500);    // 500 ms
std::chrono::seconds sec(5);          // 5 seconds
std::chrono::nanoseconds ns(1000000); // 1 million nanoseconds
```

### Important fact: different units are different types

```cpp
std::chrono::milliseconds a(500);
std::chrono::seconds b(1);

// If you add a and b...
auto sum = a + b;
// What is the type of sum? milliseconds (unified to the smaller unit)
std::cout << sum.count() << "\n";  // 1500
```

On the other hand, in a direct addition the types collide.

```cpp
// With old conventions or strict type checking
std::chrono::milliseconds ms(500);
std::chrono::seconds s(1);
if (ms < s) { }  // the types differ, so the comparison may not be possible
```

Formally, you should convert to a common unit before using them.

## 12.3 Unit conversion — implicit and explicit

### Implicit conversion: from a large unit to a small unit (lossless)

Seconds to milliseconds can be converted without loss (1 second = 1000 milliseconds).

```cpp
std::chrono::seconds sec(1);
std::chrono::milliseconds ms = sec;
std::cout << ms.count() << "\n";  // 1000
```

Here, an implicit conversion happens.
Converting 1 of `seconds` to 1000 of `milliseconds`
is done automatically because no information is lost.

### Explicit conversion: from a small unit to a large unit (lossy)

Milliseconds to seconds **needs rounding**.

```cpp
std::chrono::milliseconds ms(500);
// The following is not converted automatically (500ms → 0s truncation)
// std::chrono::seconds sec = ms;  // error

// An explicit cast is required
std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(ms);
std::cout << sec.count() << "\n";  // 0 (truncated)
```

`duration_cast` **truncates**. Converting `500ms` to seconds gives `0 seconds`.

If needed, you can round off or get the remainder.

```cpp
// Calculate the remainder
std::chrono::milliseconds ms(1500);
std::chrono::seconds sec = std::chrono::duration_cast<std::chrono::seconds>(ms);
std::chrono::milliseconds rem = ms - sec;
std::cout << sec.count() << "s " << rem.count() << "ms\n";
// Output: 1s 500ms
```

## 12.4 Arithmetic on durations

```cpp
using namespace std::chrono_literals;

auto a = 500ms;
auto b = 1s;
auto sum = a + b;        // 1500ms (the type is milliseconds)
auto diff = b - a;       // 500ms (convert b to seconds first and then take diff?)
auto doubled = 2 * a;    // 1000ms
```

**In addition and subtraction, the type is unified to the "smaller unit".**

## 12.5 `steady_clock` and `system_clock`

### `system_clock` — the wall clock

```cpp
auto now = std::chrono::system_clock::now();
```

It is the time in the real world. When it is synchronized by NTP (Network Time Protocol),
**the time can even go backward.**
A user can also change the time by hand.

In simulation, `system_clock::now()` is **not the time elapsed since the simulation started.
It is the real time of the real machine.**

### `steady_clock` — the clock for measuring

```cpp
auto start = std::chrono::steady_clock::now();
// ... some work ...
auto end = std::chrono::steady_clock::now();
auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "Elapsed time: " << elapsed.count() << " ms\n";
```

`steady_clock` **is guaranteed to be monotonically increasing.**
The time never goes backward, and NTP does not adjust it.

**To measure elapsed time, use `steady_clock`.**

## 12.6 How time is used in the drill

### Exercises 01 and 15: timers

```cpp
// solutions/01_publisher/src/minimal_publisher.cpp
using namespace std::chrono_literals;
timer_ = this->create_wall_timer(500ms, std::bind(&MinimalPublisher::timer_callback, this));
```

You pass a `std::chrono::duration` as the first argument of `create_wall_timer`.
Here you **write the `duration` 500ms directly.**

### Exercise 13: waiting with a timeout

```cpp
// tools/drill_harness.hpp (the drill runtime environment)
std::future<T> future = ...;
if (future.wait_for(2s) == std::future_status::ready) {
  // a response came back
} else {
  // timeout
}
```

`wait_for()` also takes a `std::chrono::duration` as its argument.

### Tests: the spin interval

```cpp
// tools/drill_harness.hpp
exec.spin_all(20ms);  // spin the node for 20ms
```

`spin_all(duration)` spins the node for that period.

## 12.7 ROS 2 time types — separate from `std::chrono`

### `std::chrono::duration` and `rclcpp::Duration` are different things

ROS 2 has its own time types.

```cpp
#include <rclcpp/time.hpp>

rclcpp::Duration ros_duration(5, 0);  // 5 seconds 0 nanoseconds
std::chrono::seconds std_duration(5);  // 5 seconds

// ros_duration and std_duration are different types
// you cannot add them directly
```

### Support for simulation time

When ROS 2 **enables the `use_sim_time` parameter,**
the node's time follows the "simulation time published on the `/clock` topic".

- `rclcpp::Time::now()` — the current time in simulation time (changes with `use_sim_time`)
- `rclcpp::Clock::steady_clock::now()` — the system time (does not change)

On the other hand, `std::chrono` is **always the system time**.

```cpp
// This does not support simulation time
auto now = std::chrono::system_clock::now();

// This supports simulation time
auto now = this->now();  // now() of rclcpp::Node
```

### `create_wall_timer` is WALL time

```cpp
timer_ = this->create_wall_timer(500ms, callback);
```

The "wall" in `create_wall_timer` means "wall clock", that is, **real time**.

If the robot is moving physically, the timer runs in real time.
If you want to run fast in a simulator, do not use `create_wall_timer`.
Use `create_timer(500ms, callback)` and make it follow the `/clock` topic.

Drills 01 to 15 all assume real time, so `create_wall_timer` is fine.

### Why `std::chrono` is needed

`std::chrono` is used for "measuring physical elapsed time".
Use it when you need a "measured value" that is not affected by simulation time.

```cpp
// Measure the reaction time of the hardware (no need to support simulation)
auto start = std::chrono::steady_clock::now();
sensor.read();
auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
  std::chrono::steady_clock::now() - start);
```

ROS 2's time types (`rclcpp::Time` / `rclcpp::Duration`)
are used for the "robot's control schedule".

```cpp
// Synchronizing a control loop (should support simulation time)
if (this->now() - last_update > rclcpp::Duration(0, 500000000)) {  // 500ms
  update();
  last_update = this->now();
}
```

## Try it yourself

Check the types, conversions, and measurement of `std::chrono::duration`.

```cpp
// chrono_time_demo.cpp
#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

int main()
{
  std::cout << "=== Duration types ===\n";
  std::chrono::milliseconds ms(500);
  std::chrono::seconds sec(1);
  std::chrono::nanoseconds ns(1000000);
  std::cout << "500ms: " << ms.count() << "\n";
  std::cout << "1s: " << sec.count() << "\n";
  std::cout << "1000000ns: " << ns.count() << "\n";

  std::cout << "\n=== Implicit conversion (seconds -> milliseconds) ===\n";
  sec = 2s;
  std::chrono::milliseconds from_sec = sec;
  std::cout << "2s -> milliseconds: " << from_sec.count() << "\n";

  std::cout << "\n=== Explicit conversion (milliseconds -> seconds) ===\n";
  ms = 500ms;
  std::chrono::seconds from_ms =
    std::chrono::duration_cast<std::chrono::seconds>(ms);
  std::cout << "500ms -> seconds: " << from_ms.count() << " (truncated to 0)\n";

  std::cout << "\n=== Arithmetic ===\n";
  auto sum = 500ms + 1s;
  std::cout << "500ms + 1s = " << sum.count() << " ms\n";
  auto doubled = 2 * 500ms;
  std::cout << "2 * 500ms = " << doubled.count() << " ms\n";

  std::cout << "\n=== Measurement with steady_clock ===\n";
  auto start = std::chrono::steady_clock::now();
  // Simulate work
  for (int i = 0; i < 50000000; ++i) {
    volatile int x = i;
    (void)x;
  }
  auto end = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Loop took " << elapsed.count() << " ms\n";

  std::cout << "\n=== User-defined literals ===\n";
  auto delay1 = 500ms;
  auto delay2 = std::chrono::milliseconds(500);
  std::cout << "500ms: " << delay1.count() << "\n";
  std::cout << "milliseconds(500): " << delay2.count() << "\n";
  std::cout << "Both are equivalent.\n";

  return 0;
}
```

**Predict: What are the types and values of the output? In particular, the results of `500ms` and `duration_cast<seconds>(500ms)`.**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic chrono_time_demo.cpp -o chrono_time_demo && ./chrono_time_demo
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/KsbaqsE7c)

Measured result:

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: env=static reason="elapsed time measured with steady_clock (Loop took N ms) changes on every run" -->
```
=== Duration types ===
500ms: 500
1s: 1
1000000ns: 1000000

=== Implicit conversion (seconds -> milliseconds) ===
2s -> milliseconds: 2000

=== Explicit conversion (milliseconds -> seconds) ===
500ms -> seconds: 0 (truncated to 0)

=== Arithmetic ===
500ms + 1s = 1500 ms
2 * 500ms = 1000 ms

=== Measurement with steady_clock ===
Loop took 100 ms

=== User-defined literals ===
500ms: 500
milliseconds(500): 500
Both are equivalent.
```

</details>

The value after `Loop took` changes with the environment you run it in (it was measured here on Ubuntu 24.04 / g++ 13.3 / x86_64 in Docker).

**Key points to check:**
1. `2s` is `2`, but converted to milliseconds it becomes `2000` (1000 times larger)
2. Converting `500ms` to seconds truncates it to `0`
3. The result of `500ms + 1s` is `1500ms` (unified to the smaller unit)
4. Measuring with `steady_clock` reflects the real elapsed time

## Common pitfalls

**`500ms` is not found**

```cpp
auto delay = 500ms;  // error
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// no_literals.cpp
#include <chrono>

int main()
{
  auto delay = 500ms;  // error
  return 0;
}
```

```bash
g++ -std=c++17 no_literals.cpp -o no_literals
```

</details>

<!-- measure: -->
```
no_literals.cpp: In function ‘int main()’:
no_literals.cpp:6:16: error: unable to find numeric literal operator ‘operator""ms’
    6 |   auto delay = 500ms;  // error
      |                ^~~~~
no_literals.cpp:6:16: note: use ‘-fext-numeric-literals’ to enable more built-in suffixes
```

Add `using namespace std::chrono_literals;`.

```cpp
using namespace std::chrono_literals;
auto delay = 500ms;  // OK
```

**Comparing or calculating with different units gives an error**

```cpp
std::chrono::milliseconds ms(500);
std::chrono::seconds sec(1);
if (ms < sec) { }   // an error with old compilers
```

Unify one of them.

```cpp
auto ms_as_sec = std::chrono::duration_cast<std::chrono::seconds>(ms);
if (ms_as_sec < sec) { }
```

**You pass milliseconds as a plain integer to `wait_for()`**

```cpp
future.wait_for(500);  // error, or the type is unclear
```

Pass a `std::chrono::duration`.

```cpp
using namespace std::chrono_literals;
future.wait_for(500ms);

// or
future.wait_for(std::chrono::milliseconds(500));
```

**It times out when you run fast in the simulator**

```cpp
timer_ = this->create_wall_timer(500ms, callback);  // always real time
```

`create_wall_timer` runs in real time.
To make it follow simulation time, use `create_timer`.

```cpp
timer_ = this->create_timer(500ms, callback);  // supports simulation time
```

Drills 01 to 15 assume real time, so this problem does not happen.

**You were told that `system_clock::now()` is forbidden**

```cpp
auto now = std::chrono::system_clock::now();
// discouraged in the ROS design
```

Inside a ROS 2 node, use `this->now()`.
This is an `rclcpp::Time`, and it supports simulation time.

```cpp
rclcpp::Time now = this->now();
```

Use `std::chrono::steady_clock` only for "real measurement".

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 01, 15 | `using namespace std::chrono_literals;` and the `500ms` literal |
| 01, 15 | `create_wall_timer(500ms, ...)` |
| 13 | Specifying time with `future.wait_for(2s)` |
| 14, 15 | Receiving the return value of `auto request = std::make_shared<...>();` with `auto` (chapter 8) |
| Tests | Specifying time with `drill::spin_until(..., 5s)` |
| Tests | The spin interval with `exec.spin_all(20ms)` |

The publisher of exercise 01 and the future wait of exercise 13 are the core of this chapter.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `cpp12_chrono` — handle time with types

```bash
./drill run cpp12
```

From the exercise, you can come back to this chapter with `./drill read`.

## References

- [cppreference: std::chrono](https://en.cppreference.com/w/cpp/chrono)
- [cppreference: duration_cast](https://en.cppreference.com/w/cpp/chrono/duration/duration_cast)
- [cppreference: steady_clock](https://en.cppreference.com/w/cpp/chrono/steady_clock)
- [rclcpp Time / Duration documentation](https://docs.ros.org/en/humble/Concepts/Intermediate/About-Time.html)
- [ROS 2 simulation time](https://docs.ros.org/en/humble/Tutorials/Intermediate/Understanding-ROS2-Node-Executors.html)

---

Previous → [11. The standard library toolbox](11_standard_library_toolbox.md)
Next → [13. Minimal concurrency](13_minimal_concurrency.md)
