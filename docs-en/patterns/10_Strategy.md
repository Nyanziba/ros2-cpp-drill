# 10. Strategy

> **Matches chapter 10 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep the `Strategy` interface, `WinningStrategy` / `ProbStrategy`,
> and `Player` open next to you.
>
> **Goal of this chapter**: The Java version of Strategy can be written in only one way. You make an `interface` and `implements` it. That is all.
> **C++ has at least 3 ways.** Virtual functions, `std::function`, and templates.
> All of them have the same purpose, "swap the algorithm",
> and **the only differences are when you can swap (run time or compile time) and the cost.**
> In this chapter we implement all 3, confirm that the same input gives the same output,
> and then decide **which one to choose on a microcontroller.**

## 10.0 Before that: if there is only one implementation, do not add it

It is not an accident that **0.1 "Strategy disease"** of [0. Before you use them](00_before_you_use_them.md) is placed right before this chapter.
The day after reading Strategy in the reading group, an `IMotorDriver` grows in a place that has only one implementation.

**Reading this chapter does not change that judgment.**

> Right now, are there 2 or more implementations of the algorithm? Will a second one surely come in the next month?
> If the answer is No, do not add Strategy. Write one function and finish.

This chapter is about what comes after you know that "the second one really exists".
And in C++, from there you have **3 more choices**.
Deciding to "use Strategy" does not finish the design. **That is where the real work starts.**

## 10.1 Porting the Java version to C++ as is

Yuki's `Strategy` looks like this.

```java
public interface Strategy {
    public abstract Hand nextHand();
    public abstract void study(boolean win);
}
```

If you move it to C++ in a straightforward way, it looks like this.

```cpp
class Strategy
{
public:
  virtual ~Strategy() = default;
  virtual Hand next_hand() const = 0;
  virtual void study(bool win) = 0;
};
```

There are 3 changes from the Java version.

### Change 1: Added `virtual ~Strategy() = default;`

This is the same as chapter 1. If you hold it with `std::unique_ptr<Strategy>` and release it, and
the destructor of the derived class is not called, it is undefined behavior. **If you write a pure virtual function, write a virtual destructor.**

### Change 2: Added `const` to `next_hand()`

`next_hand()` only chooses a hand and does not change state, so it is a `const` member function.
`study()` records the win or loss, so it does not get `const`. **Java has no such distinction.**

This is not just good manners. **It matters later.** A `const` Strategy is
a sign of "has no state (= can be shared from anywhere)",
and it leads to the way of writing in 10.6 where you **share a `static const` instance**.

### Change 3: You have to decide how `Player` holds the `Strategy`

The `Player` of the Java version looks like this.

```java
private Strategy strategy;
public Player(String name, Strategy strategy) {
    this.strategy = strategy;
}
```

In Java, if you write `Strategy strategy;`, it can only mean "hold a reference".
In C++ there are **4 ways to write it, and all of them mean different things**.

```cpp
Strategy strategy_;                       // ① value. Cannot compile (abstract class)
Strategy * strategy_;                     // ② raw pointer. Does not own
std::unique_ptr<Strategy> strategy_;      // ③ owns
Strategy & strategy_;                     // ④ reference. Does not own. Cannot be swapped
```

**① cannot be written.** `Strategy` has a pure virtual function, so you cannot create an object of it.
And even if you could, the moment you assign a `ProbStrategy`,
the derived part would be cut off (**slicing**).

The closest to Java's `Strategy strategy;` is **② or ③**. Which to choose is the topic of 10.2.

## 10.2 Who owns the Strategy?

The decision is made in one line.

> **Can you keep the Strategy object alive longer than the Context?**

| How to hold it | Owns | Swap at run time | Heap | When to use |
| --- | --- | --- | --- | --- |
| `std::unique_ptr<Strategy>` | Yes | Can | **Needed** | You do not want to manage the object's lifetime. PC side |
| `Strategy *` (raw pointer) | No | Can | Not needed | The object is `static` or held by the caller. **Microcontrollers** |
| `Strategy &` | No | **Cannot** | Not needed | When it is fixed to one for the whole life |
| `std::shared_ptr<Strategy>` | Shares | Can | Needed | Several Contexts use the same Strategy and the lifetime is unclear |

**If you choose `Strategy &`, you cannot swap.** This is because a reference cannot be rebound.

```cpp
class Player
{
public:
  explicit Player(Strategy & strategy) : strategy_(strategy) {}
  void set_strategy(Strategy & strategy)
  {
    strategy_ = strategy;   // the target of the reference does not change. It becomes an assignment of the contents (and breaks)
  }
private:
  Strategy & strategy_;
};
```

`strategy_ = strategy;` is not "swap the reference". It is
**"assign to the object the reference points to".** For an abstract class the assignment operator cannot be used, so
it is a compile error, and for a concrete class **it slices and breaks silently**.
The essence of the Strategy pattern is swapping, so **a reference member is basically not suitable.**

In this exercise we hold it with a **raw pointer** (`const VelocityFilter * filter_;`).
It does not own, it can be swapped, and it needs no heap. **This is what you use on a microcontroller.**

In exchange, **if the Strategy object dies before the Context, it is fatal right away**.

```cpp
VirtualCommander make_commander()
{
  ClampFilter filter{1.0};        // local
  return VirtualCommander{filter};// filter dies here
}                                 // the returned Commander points to a dead Strategy

auto commander = make_commander();
commander.update(2.0);            // undefined behavior
```

This is the exact same kind of accident as the iterator in chapter 1.
**In Java the GC keeps `filter` alive, so it does not crash. In C++ it crashes.**
If you hold it with a raw pointer, your job is to write in the header "keep the object alive longer than the Context".

### A Strategy with state is even more dangerous

Yuki's `WinningStrategy` has `won` and `prevHand`. It is a **Strategy with state**.
When there is state, this happens.

- If 2 Contexts share the same Strategy instance, **the learning results get mixed**
- If you swap the Strategy and swap it back, **the previous state is still there** (correct, if that is what you want)
- You cannot make it a `static` object (it breaks with multiple threads)

**As a design, first think whether the state can be put on the Context side.**
This exercise does that. The "previous value" of the filter is held by the Commander, not by the Strategy.

```cpp
// The Strategy has no state. So it is const, can be shared, and can be static
virtual double apply(double previous, double raw) const = 0;
```

If you move the state out to the Context, **the Strategy can be treated like a value.**
This matters in 10.6.

## 10.3 Way 2: `std::function`

C++ has a choice that Java does not have. **A Strategy without inheritance.**

```cpp
class FunctionCommander
{
public:
  using FilterFn = std::function<double(double previous, double raw)>;
  explicit FunctionCommander(FilterFn filter) : filter_(std::move(filter)) {}
  void set_filter(FilterFn filter) { filter_ = std::move(filter); }
  double update(double raw) { previous_ = filter_(previous_, raw); return previous_; }
private:
  FilterFn filter_;
  double previous_ = 0.0;
};
```

The caller does not write a single class.

```cpp
FunctionCommander commander{
  [](double, double raw) { return std::clamp(raw, -1.0, 1.0); }};
```

What took 5 lines in the Java version as `class ClampStrategy implements Strategy { ... }`
becomes **one line of lambda**. You do not even need the `Strategy` interface itself.

On top of that, `std::function` can **accept a function pointer, a class with `operator()`,
or a bound member function**. Its biggest advantage is that it is not tied to an inheritance hierarchy.

### But a heap allocation may happen

`std::function` holds the contents by erasing their type (type erasure). It does not know what will be put in, so
**it puts things that are too big on the heap**. This is the result I measured on my machine (Apple clang / arm64).

```
sizeof(std::function<double(double)>) = 32
small lambda  : allocations = 0      ← captures 1 double
large lambda  : allocations = 1      ← captures 5 doubles
```

With the small lambda there were 0 allocations. This is called **SBO (small buffer optimization)**.
It fit in the small buffer that `std::function` has inside.

**But this is not guaranteed by the standard.**
What the standard guarantees is only something like "storing a function pointer or a `reference_wrapper` does not throw",
and **it does not guarantee that a lambda is not allocated**.
The size of the buffer also depends on the implementation (the 32 bytes above is the value of Apple clang,
and it is different in other standard libraries).

So `std::function` has the following properties.

- **Just by adding one capture, `malloc` can suddenly start to run one day**
- You cannot tell whether it ran by reading the code
- If you change the library implementation, the threshold changes

On a PC nobody has a problem.
**On a microcontroller, the moment `malloc` runs inside the control loop, it is over.**
With `-fno-exceptions`, the behavior when the allocation fails is also doubtful.

## 10.4 Way 3: templates (policy)

If swapping at **compile time** is enough, this is the best.

```cpp
template <typename FilterPolicy>
class StaticCommander
{
public:
  explicit StaticCommander(FilterPolicy policy) : policy_(std::move(policy)) {}
  double update(double raw) { previous_ = policy_.apply(previous_, raw); return previous_; }
private:
  FilterPolicy policy_;
  double previous_ = 0.0;
};
```

All that is required of `FilterPolicy` is **"`apply(double, double)` can be called"**.
There is no base class, no `virtual`, and no `override`. Because no inheritance relation is needed,
**you can pass an unrelated type that someone else wrote later as a Strategy**.

Java generics cannot do this (you have to write `<T extends Strategy>`).
**C++ templates are duck typing.** This is a big difference from Java.

In exchange, **you cannot swap at run time.**
`StaticCommander<ClampPolicy>` and `StaticCommander<SlewRatePolicy>` are **different types**.
You cannot put them in the same variable.

### Is the cost really zero?

This is the code that was actually produced with `-O2` (arm64). The virtual function version:

```
__Z11run_virtualRK6Filterd:
	ldr	x8, [x0]        ; read the vptr
	ldr	x1, [x8, #16]   ; read the address of apply from the vtable
	br	x1              ; indirect jump
```

The policy version:

```
__Z10run_policyRK11ClampPolicyd:
	ldr	d1, [x0]        ; read the member m_
	fcmp	d1, d0
	fcsel	d0, d1, d0, mi  ; just compare and select. The call has disappeared
	ret
```

**In the policy version the function call itself disappears and only one comparison is left.**
The virtual function version reads memory twice and then does an indirect jump.
An indirect jump is likely to be mispredicted by the branch predictor, and this matters inside a loop.

The size is also different.

```
sizeof(Clamp)        = 16      ← vptr 8 + double 8
sizeof(ClampPolicy)  = 8       ← double 8
```

Because of the vtable pointer, it grows by **8 bytes per instance**.
In addition, the vtable itself uses ROM, and RTTI information is attached (you can remove it with `-fno-rtti`).
If you line up 100 sensors, that is 800 bytes. If the RAM of the microcontroller is 20 KB, you cannot ignore it.

## 10.5 The 4th: function pointer

It is easy to forget, but **a function pointer is also a perfectly good Strategy.**

```cpp
using FilterFn = double (*)(double previous, double raw);

class RawCommander
{
public:
  explicit RawCommander(FilterFn filter) : filter_(filter) {}
  void set_filter(FilterFn filter) { filter_ = filter; }   // can be swapped at run time
  double update(double raw) { previous_ = filter_(previous_, raw); return previous_; }
private:
  FilterFn filter_;
  double previous_ = 0.0;
};
```

- Can be swapped at run time (the same as `std::function`)
- Uses no heap at all (different from `std::function`)
- The size is one pointer (8 bytes. `std::function` is 32 bytes)
- Needs no vtable and no RTTI

**And a lambda without captures can be implicitly converted to a function pointer.**

```cpp
double (*fp)(double) = [](double raw) { return raw * 2.0; };
std::printf("%.1f\n", fp(3.0));      // 6.0
```

So if you only want to "write it as a lambda", you do not need `std::function`.
**If you do not use captures, you can receive it with a function pointer.**

If it captures, it cannot be converted. The actual error looks like this.

```
error: no viable conversion from '(lambda at err.cpp:4:26)' to 'double (*)(double)'
```

There are 2 weak points. It **cannot hold state** (it cannot capture), and
the call is always an indirect call (it is not inlined).
C language APIs (the comparison function of `qsort`, registering an interrupt handler) all have this form
because it is the smallest Strategy.

## 10.6 Comparison table of the 4 ways

| Way | Swapping | Heap | Size | Inlining | State | Inheritance |
| --- | --- | --- | --- | --- | --- | --- |
| Virtual function | Run time | Depends on how you hold it | vptr +8 | Not done | Can hold | Needed |
| `std::function` | Run time | **May happen** | 32 bytes | Not done | Can hold | Not needed |
| Function pointer | Run time | None | 8 bytes | Not done | **Cannot hold** | Not needed |
| Template | **Compile time** | None | Zero or more | **Done** | Can hold | Not needed |

There are 2 steps to choose.

1. **Do you really need to switch at run time?** If not, use a **template**
2. If yes, **do you need state?** If not, use a **function pointer**. If yes, use a **virtual function**

`std::function` is the first choice only when all of
"switch at run time", "need state", "do not want to write a class", and "do not care about the heap"
are true. **On the PC side it is fine. On a microcontroller, none of these is allowed.**

## 10.7 Is there something in the standard library / language that does the same?

**Yes. The standard library is full of Strategy.**

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });   // the comparison is a Strategy
std::unique_ptr<T, MyDeleter> p;                                     // the way to release is a Strategy
std::vector<T, MyAllocator<T>> v;                                    // the way to allocate is a Strategy
std::unordered_map<K, V, MyHash, MyEq> m;                            // the hash and equality are a Strategy
```

What to notice is **which way they are written in**.

| Standard feature | What is swapped | Way |
| --- | --- | --- |
| The 3rd argument of `std::sort` | Comparison | **Template** (deduced from the actual argument) |
| The 2nd type argument of `std::unique_ptr` | Release | **Template** |
| The `Allocator` of `std::vector` | Allocation | **Template** |
| `std::function` | What to call | Type erasure (= this itself is a tool for "run-time Strategy") |
| `std::pmr::polymorphic_allocator` | Allocation | **Virtual function** (`memory_resource`) |

**The default of the standard library is templates.** Because they cost nothing.
And for the demand to switch at run time, C++17 prepared
`std::pmr` (polymorphic allocators) as **a separate virtual function version**.
The relation between `std::allocator` and `std::pmr::memory_resource` is
exactly the relation between "the template version and the virtual function version" in this chapter.

**The standard library also struggled with the same 3 choices, and prepared both.**

Also look at the fact that the `Deleter` of `std::unique_ptr<T, Deleter>` is the 2nd **type parameter**.
`sizeof(std::unique_ptr<int>)` is the size of one pointer.
Because the Deleter is a template, a Deleter without state does not add even 1 byte
(**empty base optimization**). If it had been written with virtual functions, this would not be possible.

## 10.8 Try it yourself

We replace `operator new` and count whether `std::function` really allocates.
**Predict first, then run it.**

```cpp
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <new>

static std::size_t g_alloc_count = 0;

void * operator new(std::size_t size)
{
  ++g_alloc_count;
  void * p = std::malloc(size);
  if (p == nullptr) {
    throw std::bad_alloc{};
  }
  return p;
}

void operator delete(void * p) noexcept { std::free(p); }
void operator delete(void * p, std::size_t) noexcept { std::free(p); }

class Filter
{
public:
  virtual ~Filter() = default;
  virtual double apply(double raw) const = 0;
};

class Clamp : public Filter
{
public:
  explicit Clamp(double m) : m_(m) {}
  double apply(double raw) const override { return raw > m_ ? m_ : raw; }
private:
  double m_;
};

class ClampPolicy
{
public:
  explicit ClampPolicy(double m) : m_(m) {}
  double apply(double raw) const { return raw > m_ ? m_ : raw; }
private:
  double m_;
};

int main()
{
  std::printf("sizeof(Clamp)        = %zu\n", sizeof(Clamp));
  std::printf("sizeof(ClampPolicy)  = %zu\n", sizeof(ClampPolicy));
  std::printf("sizeof(std::function<double(double)>) = %zu\n",
              sizeof(std::function<double(double)>));

  double a = 1.0, b = 2.0, c = 3.0, d = 4.0, e = 5.0;

  g_alloc_count = 0;
  std::function<double(double)> small = [a](double raw) { return raw * a; };
  std::printf("small lambda  : allocations = %zu\n", g_alloc_count);

  g_alloc_count = 0;
  std::function<double(double)> big =
    [a, b, c, d, e](double raw) { return raw * (a + b + c + d + e); };
  std::printf("large lambda  : allocations = %zu\n", g_alloc_count);

  double (*fp)(double) = [](double raw) { return raw * 2.0; };
  std::printf("via function pointer = %.1f\n", fp(3.0));
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: how many allocations does each of the 2 lambdas make? And what is the size difference between <code>Clamp</code> and <code>ClampPolicy</code>?</summary>

On my machine (Apple clang / arm64) the result was this.

```
sizeof(Clamp)        = 16
sizeof(ClampPolicy)  = 8
sizeof(std::function<double(double)>) = 32
small lambda  : allocations = 0
large lambda  : allocations = 1
via function pointer = 6.0
```

- The lambda that captures 1 `double` was **not allocated** (it fit in the SBO)
- When it captures 5 `double` (40 bytes), it does not fit in the 32 bytes of `std::function`, and **1 allocation** ran
- The size difference of 8 bytes is the vtable pointer

You can see that **just adding one capture changes the behavior**.
And this boundary is not written in the standard. **Other compilers and other standard libraries give different values.**
"It was not allocated on my machine" is not a basis on a microcontroller.
</details>

## 10.9 Conclusion for microcontrollers

**The first choice is the template (policy) version.**

```cpp
template <typename FilterPolicy>
class VelocityCommander
{
public:
  explicit VelocityCommander(FilterPolicy policy) : policy_(policy) {}
  double update(double raw) { previous_ = policy_.apply(previous_, raw); return previous_; }
private:
  FilterPolicy policy_;
  double previous_ = 0.0;
};

// the user side. The type is fixed here
VelocityCommander<ClampPolicy> commander{ClampPolicy{1.0}};
```

No vtable, no heap, and it is inlined. As we saw in 10.4,
the call to `apply` itself disappears. **For the microcontroller side of the club, write it this way as a rule.**

**Do not use `std::function`.** The reason is as in 10.3:
the standard does not guarantee whether an allocation happens.
"It is small, so it is fine" will be broken by a junior who adds one capture.

### When you really want to switch at run time

There are cases where it is **decided at run time**, such as "read the DIP switch at startup and decide the control law" or "switch the filter with a debug command".
Only then do you use the virtual function version.
**Do not use the heap. Put the objects in `static` and swap the pointer.**

```cpp
// the objects live as long as the program. No heap
static const ClampFilter    kClamp{1.0};
static const SlewRateFilter kSlew{0.5};

class VelocityCommander
{
public:
  explicit VelocityCommander(const VelocityFilter & filter) : filter_(&filter) {}
  void set_filter(const VelocityFilter & filter) { filter_ = &filter; }  // swap
  double update(double raw) { previous_ = filter_->apply(previous_, raw); return previous_; }
private:
  const VelocityFilter * filter_;   // does not own
  double previous_ = 0.0;
};

VelocityCommander commander{kClamp};
if (dip_switch_is_on()) {
  commander.set_filter(kSlew);      // neither make_unique nor new appears
}
```

**`std::unique_ptr<VelocityFilter>` does not appear.**
All kinds of Strategy are known at compile time, so it is enough to place the objects statically beforehand.
What moves is only **one pointer**.

Here the "put the state on the Context side" of 10.2 pays off.
`apply()` is `const` and has no state, so it can be `static const`,
and several Commanders can share the same object.

### If you want to cut further

If the Strategy has no state and there are few kinds, a **function pointer** is enough (10.5).
The 32 bytes of `std::function` become 8 bytes, and the vtable also disappears from ROM.

```cpp
using FilterFn = double (*)(double previous, double raw);
static double clamp_filter(double, double raw) { return raw > 1.0 ? 1.0 : raw; }
```

### Ways you must not write it

```cpp
// Bad: the Strategy is recreated inside the loop
void control_loop()
{
  while (true) {
    auto filter = std::make_unique<ClampFilter>(1.0);   // heap allocation every cycle
    output = commander.update(filter.get(), raw);
  }
}
```

With a 1 kHz control cycle, that is 1000 allocations and releases per second. The heap will fragment and eventually fail.
**Create the Strategy at startup, and after that only swap it.**

## 10.10 Conclusion for ROS 2 (supplement)

In ROS 2 (on Linux) you may use all 3. In fact rclcpp
**uses `std::function` everywhere**.

```cpp
subscription_ = this->create_subscription<sensor_msgs::msg::Imu>(
  "imu", 10,
  [this](sensor_msgs::msg::Imu::SharedPtr msg) { this->on_imu(msg); });
```

This callback is a Strategy. The `Subscription` does not know "what to do when a message arrives".
It only calls what is passed from the outside. No inheritance and no `override` are needed.
**In the Java version, you would write a class that implements a `MessageListener` interface.**

There are 2 points to note.

- The callback captures `this`, so **do not keep it alive longer than the node.**
  This is the lifetime story of 10.2 appearing as it is
- On a real-time path (a path that wants to keep its period, for example with `rclcpp::executors::StaticSingleThreadedExecutor`),
  the allocation of `std::function` becomes a problem for the same reason as on a microcontroller.
  Only there, move to templates or function pointers

If you switch the control law with a parameter (`declare_parameter("filter_type", "slew")`),
it is **decided at run time**, so use the virtual function version or the `std::function` version.
In this case you may own it with `std::unique_ptr<VelocityFilter>`. The heap is free.

## 10.11 Common pitfalls

| Symptom | Cause |
| --- | --- |
| `error: field type 'Strategy' is an abstract class` | You try to hold the Strategy as a member by value. Use a pointer, a reference, or `unique_ptr` |
| After assigning a Strategy, the derived behavior is not called | You received it by value and it was sliced. Only the base part is left |
| You wrote `set_strategy()` but got a compile error | The member is `Strategy &`. A reference cannot be rebound. Use a pointer |
| You swapped the Strategy but the behavior did not change | The Context holds a **copy** of the Strategy. Point to it with a pointer |
| It crashed when returning from a function that returns a Context | The Strategy object was a local variable. The lifetime problem of 10.2 |
| `error: no viable conversion from '(lambda ...)' to 'double (*)(double)'` | A lambda with captures does not become a function pointer. Use `std::function` or a template |
| The microcontroller hangs sometimes / gets slower and slower | The heap allocation of `std::function`. Use a template or a function pointer |
| You cannot put `StaticCommander<ClampPolicy>` and `<SlewRatePolicy>` in the same variable | They are different types. **That is the limit of the template version.** If you want to switch at run time, use the virtual function version |
| The destructor is not called on release | `Strategy` has no virtual destructor |
| The learning results of 2 Contexts get mixed | The Strategy has state and is shared. Put the state on the Context side |

## 10.12 Matching exercise

```bash
./drill run dp10
```

The subject is a **filter for velocity commands**. It compares the raw command coming from above
with the value output last time, rounds it, and then passes it to the motor.
The way of rounding is the Strategy, and there are 2 kinds.

- `clamp`: cap by absolute value (put `raw` in `[-max_abs, +max_abs]`)
- `slew rate`: limit the change from the previous value (put `raw` in `[previous - d, previous + d]`)

In `exercises/dp10_strategy/src/velocity_filter.cpp` you implement **the same algorithm in 3 ways**.

1. **Virtual function version**: `ClampFilter::apply` / `SlewRateFilter::apply`, and
   `VirtualCommander`, which **does not own the Strategy and points to it with a pointer**
2. **`std::function` version**: `FunctionCommander`, and `make_clamp_fn`, which returns a lambda
3. **Template version**: `ClampPolicy::apply` / `SlewRatePolicy::apply` (do not write `virtual`)

The tests check the following 4 points.

- **All 3 return the same output for the same input** (only the way is different, and the algorithm is the same)
- **The behavior changes at run time** with `set_filter()`
- `VirtualCommander` **points to the Strategy by address without copying it** (address comparison of `filter()`)
- The template version **has no virtual functions**
  (`static_assert(!std::is_polymorphic_v<ClampPolicy>)` and a `sizeof` comparison)
- You can **pass a lambda with captures directly** to `FunctionCommander`

## 10.13 Summary of this chapter

- **If there is only one implementation, do not add Strategy.** Reading this chapter does not change that
- Against the one way of Java's `interface`, C++ has 4 ways: **virtual function / `std::function` / function pointer / template**
- The differences are "**whether swapping is at compile time or run time**" and "**whether it uses the heap**"
- **How the Context holds the Strategy** is the core of the design. `unique_ptr` (owns) / raw pointer (does not own) /
  reference (cannot swap). **A reference member is not suitable for Strategy**
- **Put the state on the Context, not on the Strategy.** Then you can share the Strategy as `static const`
- The SBO of `std::function` is **not guaranteed by the standard**. If the captures grow, an allocation happens
- A lambda without captures **can be converted to a function pointer**. C APIs have this form because it is the smallest
- The Strategies of the standard library (the comparison of `sort`, the Deleter of `unique_ptr`, the Allocator) are **templates by default**
- **On microcontrollers the template version is the first choice.** Only when you switch at run time,
  use the virtual function version that swaps a pointer to `static` objects. Do not use `std::function`

---

Previous: [9. Bridge](09_Bridge.md) / Next: [11. Composite](11_Composite.md)
