# 17. Observer

> **Corresponds to chapter 17 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep the `Observer` interface and `NumberGenerator` open next to you.
>
> **Goal of this chapter**: In the Java version, `addObserver(observer)` returns `void` and that is all.
> **If you write the same thing in C++, it breaks.** If a subscriber dies before the Subject,
> the Subject keeps a dangling pointer. In Java the GC keeps the object alive, so it does not crash.
> In this chapter, we first **run it and watch how it breaks**,
> then implement and compare 3 solutions, and finally land on a form that "returns an RAII token that represents the subscription".
> **This is the longest chapter in this course.** All the topics of ownership and lifetime come together here.

## 17.1 If you port the Java version to C++ as it is

Yuki's book defines `Observer` and `Subject` like this.

```java
public interface Observer {
    public abstract void update(NumberGenerator generator);
}

public abstract class NumberGenerator {
    private ArrayList<Observer> observers = new ArrayList<Observer>();
    public void addObserver(Observer observer)    { observers.add(observer); }
    public void deleteObserver(Observer observer) { observers.remove(observer); }
    public void notifyObservers() {
        for (Observer o : observers) { o.update(this); }
    }
}
```

If you port it to C++ in a straightforward way, you get this. Since this is a club library, let's make it a hub that distributes distance sensor values.

```cpp
class SensorObserver
{
public:
  virtual ~SensorObserver() = default;          // Change 1
  virtual void on_sample(int value_mm) = 0;     // Change 2
};

class SensorHub
{
public:
  void add_observer(SensorObserver * observer)  // Change 3
  {
    observers_.push_back(observer);
  }

  void notify(int value_mm)
  {
    for (SensorObserver * observer : observers_) {
      observer->on_sample(value_mm);
    }
  }

private:
  std::vector<SensorObserver *> observers_;
};
```

### Change 1: Virtual destructor

You have read this in every chapter from 1 to 16, so I keep it short. If you write a pure virtual function, also write a virtual destructor.
(The **only exception** appears in 17.9 "Conclusion for microcontrollers" of this chapter.)

### Change 2: `update(this)` became `on_sample(value_mm)`

Yuki's book uses `update(NumberGenerator generator)`, and the observer calls
`generator.getNumber()` itself. This is the **pull style**. If you use the pull style in C++, you write

```cpp
virtual void update(const SensorHub & hub) = 0;
```

and that means **`SensorObserver` knows `SensorHub`**.
The headers depend on each other, and just changing the type `SensorHub` forces all observers to be recompiled.
If you use the **push style**, which passes the value as an argument, this dependency disappears.

```cpp
virtual void on_sample(int value_mm) = 0;   // does not need to know SensorHub
```

In Java this does not matter, because you only pass a reference.
In C++, **header dependencies and compile time** are a real cost. This chapter uses the push style.

### Change 3: `Observer` became `SensorObserver *`

Java's `ArrayList<Observer>` is an array of references. If you write `std::vector<SensorObserver>` in C++,

- it **does not compile at all**, because the class is abstract (`std::vector` holds its elements by value)
- even if it compiled, copies would be made and **slicing** would happen

so you must choose one of pointer, reference, or `shared_ptr`.
The code above uses a **raw pointer**. **And this is where it breaks.**

## 17.2 The main problem — the subscriber dies first

This is what was announced in 0.4 of [0. Before you use them](00_before_you_use_them.md).

```cpp
{
  Display display;
  hub.add_observer(&display);
}                      // display dies. hub still holds &display
hub.notify(200);       // undefined behavior
```

**In Java, the GC keeps `display` alive, so it does not crash** (instead, it leaks forever.
Even in Java, forgetting to call `deleteObserver` is a classic memory leak).
**In C++, `display` really disappears.**

The problem is that **it usually does not crash**. Let's run it.

```cpp
class Display : public SensorObserver
{
public:
  void on_sample(int value_mm) override { std::printf("display %d\n", value_mm); }
};

class Logger : public SensorObserver
{
public:
  void on_sample(int value_mm) override { std::printf("logger  %d\n", value_mm); }
};

int main()
{
  SensorHub hub;

  Display * display = new Display{};
  hub.add_observer(display);
  std::printf("display addr = %p\n", static_cast<void *>(display));
  hub.notify(100);
  delete display;                     // display dies here

  Logger * logger = new Logger{};     // the same address is reused
  std::printf("logger  addr = %p\n", static_cast<void *>(logger));
  hub.notify(200);                    // calls logger, thinking it is display
  delete logger;
  return 0;
}
```

This is the output (Apple clang / macOS).

```
display addr = 0x1034fa470
display 100
logger  addr = 0x1034fa470
logger  200
```

You meant to notify `display`, but **`logger` was called.**
Another class was placed at the address you `delete`d, and the call followed the vtable pointer found there.

**The program does not crash. No exception. No error message.**
The notification target is silently replaced by another class.
You can imagine what happens when this occurs in the control loop of a robot.

If you can use a sanitizer (`-fsanitize=address`), it reports
`heap-use-after-free` right away. Please use it whenever you can.
**This kind of bug is invisible unless you bring in tools.**

## 17.3 Who owns the subscription — 3 solutions

The root cause is that `add_observer` returns `void`.
**Nowhere in the code does it say "who owns this subscription and when it ends".**
The principle "declare ownership in the type", which we have repeated since chapter 1, works here too.

There are 3 ways to deal with it.

| Method | How to write it | Cost |
| --- | --- | --- |
| Make unsubscribing a duty | `hub.remove_observer(this)` in the observer's destructor | **You forget to write it.** Also, the Subject's lifetime is required |
| Hold with `weak_ptr` | The Subject holds `std::vector<std::weak_ptr<SensorObserver>>` | Observers **must be managed by `shared_ptr`** |
| Token style (RAII) | `subscribe()` returns an RAII object that represents the subscription, and destroying it unsubscribes automatically | You have to implement it. **Safest** |

Let's look at them in order.

### Method 1: Make unsubscribing a duty

```cpp
class Display : public SensorObserver
{
public:
  explicit Display(SensorHub & hub) : hub_(hub) { hub_.add_observer(this); }
  ~Display() override { hub_.remove_observer(this); }   // if you forget this, 17.2 happens

  void on_sample(int value_mm) override { /* ... */ }

private:
  SensorHub & hub_;
};
```

At first glance this solves it. But 2 problems remain.

1. **You forget to write it.** Every time you add an observer, you need to write a destructor,
   and if you forget, the code still compiles. And as 17.2 showed, you cannot notice the mistake even by running it
2. **If `SensorHub` dies first, this time `~Display()` breaks.**
   The `SensorHub` that `hub_` refers to is gone, but you call `remove_observer` on it

The second one matters. **A human has to remember the order of lifetimes.**
You can settle it with a comment like "Always keep the Hub alive longer than the observers",
but that is a convention, and the compiler protects nothing.

### Method 2: Hold with `weak_ptr`

If the Subject holds `weak_ptr`, the Subject can detect that a subscriber has died.

```cpp
class SensorHub
{
public:
  void add_observer(const std::shared_ptr<SensorObserver> & observer)
  {
    observers_.push_back(observer);
  }

  void notify(int value_mm)
  {
    // iterate while cleaning up dead subscribers
    std::vector<std::weak_ptr<SensorObserver>> alive;
    alive.reserve(observers_.size());
    for (const std::weak_ptr<SensorObserver> & weak : observers_) {
      if (const std::shared_ptr<SensorObserver> observer = weak.lock()) {
        observer->on_sample(value_mm);
        alive.push_back(weak);
      }
    }
    observers_.swap(alive);
  }

private:
  std::vector<std::weak_ptr<SensorObserver>> observers_;
};
```

When you run it, you get this.

```
display 100
生き残った購読 = 0
```

After the subscriber dies, `notify(200)` **does nothing. It does not crash.** This is the correct behavior.

`lock()` is the key. It is promoted to `shared_ptr` only while notifying,
so it is **safe even if another thread releases the observer while `on_sample` is running**.
In chapter 16 (Mediator) we used `weak_ptr` to break a `shared_ptr` cycle,
but here we use it for **detecting lifetime**. It is the same tool used in a different way.

The costs are clear.

- **Observers must be managed by `shared_ptr`.** A `Display display;` placed on the stack
  cannot be registered. `std::make_shared<Display>()` becomes mandatory
- **Every notification adds the reference count operation of `lock()`** (an atomic operation)
- On microcontrollers, `shared_ptr` itself often cannot be used (the control block needs a heap allocation)

If you can **enforce the policy "create every observer with `shared_ptr`" across the whole library**,
this is a perfectly practical choice. For ROS 2 code it is hardly a problem.

### Method 3: Token style (the main choice of this chapter)

`subscribe()` returns **a value that represents the subscription**. When that value dies, the subscription ends.

```cpp
{
  Subscription sub = hub.subscribe(&display);   // the subscription starts
  hub.publish(100);                             // reaches display
}                                               // sub dies → the subscription ends
hub.publish(200);                               // does not reach display
```

It is the same idea as `std::unique_ptr` and `std::lock_guard`.
**"If an operation needs cleanup, return an object that does the cleanup."**

This has 3 advantages.

1. **You cannot forget to write it.** If you throw away the return value, the subscription does not start (that is, it ends right there).
   Observers do not need a destructor
2. **Observers do not need to be `shared_ptr`.** Objects placed on the stack work too
3. **It can be safe even if the Subject dies first.** If the token watches the subscription list through a `weak_ptr`,
   `~Subscription()` does nothing after the Subject has died

For the third point, we do not put the subscription list directly inside `SensorHub`.
**We put it in a separate object (`Registry`) held by `shared_ptr`.**
The token watches it through a `weak_ptr`. The header of the exercise has this form.

```cpp
class SensorHub
{
  std::shared_ptr<detail::Registry> registry_;   // the real thing is here
};

class Subscription
{
  std::weak_ptr<detail::Registry> registry_;     // only peeks
  std::size_t id_ = 0;
};
```

| Who dies | What happens |
| --- | --- |
| The observer (together with its token) dies first | `~Subscription()` removes its own subscription from the `Registry` |
| `SensorHub` dies first | `Registry` also dies → the token's `lock()` returns empty → **does nothing** |

**It is safe in both directions.** This is what Method 1 could not guarantee.

### Give the token the same properties as `unique_ptr`

There is only one subscription. If it were copied, it would be unclear which destructor should unsubscribe.

```cpp
Subscription(const Subscription &) = delete;
Subscription & operator=(const Subscription &) = delete;
Subscription(Subscription && other) noexcept;
Subscription & operator=(Subscription && other) noexcept;
```

**In the move constructor, always make the moved-from object empty.**

```cpp
Subscription::Subscription(Subscription && other) noexcept
: registry_(std::move(other.registry_)),
  id_(other.id_)
{
  other.registry_.reset();
  other.id_ = 0;          // ← if you forget this, the subscription ends the moment the moved-from object dies
}
```

It is the same as setting the pointer to `nullptr` in a `unique_ptr` move.
`std::move` **only has a verb for a name, and moves nothing**. Emptying the object is your job.

## 17.4 The subscription list changes during notification

Here is another problem. It also happens in Java, but it breaks differently in C++.

```cpp
void notify(int value_mm)
{
  for (SensorObserver * observer : observers_) {   // ← here
    observer->on_sample(value_mm);                 // ← what if remove_observer is called inside this?
  }
}
```

In Java, `ConcurrentModificationException` is thrown. **It is an exception, so you notice it.**
C++ says nothing. Let's run it. Three observers unsubscribe themselves when they receive a notification.

```cpp
class SelfRemoving : public SensorObserver
{
public:
  void on_sample(int value_mm) override
  {
    std::printf("observer %d got %d\n", id_, value_mm);
    hub_->remove_observer(this);   // changes the list in the middle of notification
  }
  // ...
};
```

This is the output.

```
observer 1 got 42
observer 3 got 42
observer 3 got 42
returned from notify
```

Observers 1, 2, and 3 should each be called once, but **number 2 is skipped and number 3 is called twice.**
And the last call reads **out of the range** of the `vector`, which has already shrunk by `erase`.
This does not crash either.

The cause is that `erase` invalidates iterators.
`std::vector::erase` moves the following elements forward,
so the iterator held by the range-based for points to the element one step ahead.

**There are 2 ways to deal with it.**

| Method | How | Cost |
| --- | --- | --- |
| Copy, then iterate | `auto snapshot = observers_; for (auto * o : snapshot) { ... }` | **A copy for every notification** (heap allocation). Observers that were already removed are still notified |
| Deferred removal | Unsubscribing only "marks" the entry. The real removal happens after the notification loop ends | One more flag. **Zero allocations** |

The exercise implements **deferred removal**. We choose the one that works on microcontrollers too (the one without allocation).

```cpp
void Registry::remove(std::size_t id)
{
  for (Entry & entry : entries) {
    if (entry.id == id) {
      entry.observer = nullptr;   // only mark it. do not erase
      break;
    }
  }
  if (!notifying) { compact(); }  // if not notifying, close the gaps right now
}
```

The notification loop also **iterates by index**.

```cpp
const std::size_t count = entries.size();     // fix the count before the loop
for (std::size_t i = 0; i < count; ++i) {
  SensorObserver * observer = entries[i].observer;
  if (observer != nullptr) { observer->on_sample(value_mm); }
}
```

We do not keep references or iterators because **if `subscribe()` is called during notification,
`std::vector` is reallocated**. An index keeps working correctly even after reallocation.
We fix the count first so that **a subscription added during notification is not called in that round**
(if it were called, the notification would reach an observer that is not ready yet).

## 17.5 Notifications loop

This is the other problem announced in 0.4. A's target notifies B, and B's target notifies A.

```cpp
a.add_observer(&to_b);   // when a is updated, notify b
b.add_observer(&to_a);   // when b is updated, notify a
a.notify(1);             // never returns
```

It is an infinite loop, so you cannot observe it if you just run it.
We measured it with a mechanism that counts the depth and forcibly stops at 8 levels.

```
depth 1
depth 1
depth 2
depth 2
depth 3
depth 3
...
depth 8
depth 8
depth 9 に到達。止めます
```

**Without the stopping mechanism, it keeps going until the stack is used up.**
The same happens in Java, but the difference is that in C++ a stack overflow
is (in many environments) **sudden death with no error message**.

The fix is one re-entrancy guard flag.

```cpp
void SensorHub::publish(int value_mm)
{
  if (registry_->notifying) { return; }   // ignore re-entry during notification
  // ...
}
```

**Forgetting to reset the flag is the next accident.** If `on_sample` throws an exception, `notifying` stays
true, and that Subject **never notifies again**.
Write a small RAII class on the spot that resets it in the destructor.

```cpp
struct NotifyingGuard
{
  explicit NotifyingGuard(detail::Registry & target) : registry(target)
  {
    registry.notifying = true;
  }
  ~NotifyingGuard() { registry.notifying = false; }

  NotifyingGuard(const NotifyingGuard &) = delete;
  NotifyingGuard & operator=(const NotifyingGuard &) = delete;

  detail::Registry & registry;
};
```

What Java does with `try` / `finally`, C++ does with a destructor.
**Instead of `finally`, we have RAII.** This is a part where C++ is stronger.

There is also a design other than "ignore" for re-entry: "push it to a queue and process it after the notification".
It takes more implementation, but no notification is lost. For a club library, ignoring is enough at first.
**If you drop it, make sure the drop is written to the log.**

## 17.6 Comparison with the callback style using `std::function`

You probably thought, "Writing an observer class every time is a pain. Why not let users register lambdas?"

```cpp
class SensorHub
{
public:
  void add_callback(std::function<void(int)> callback)
  {
    callbacks_.push_back(std::move(callback));
  }

private:
  std::vector<std::function<void(int)>> callbacks_;
};

hub.add_callback([&display](int mm) { display.show(mm); });   // certainly easy
```

Registering is overwhelmingly easier. You do not need inheritance. **But you cannot unsubscribe.**

```cpp
auto cb = [](int v) { (void)v; };
callbacks_.push_back(cb);
for (auto it = callbacks_.begin(); it != callbacks_.end(); ++it) {
  if (*it == cb) { callbacks_.erase(it); break; }   // I want to write this
}
```

```
error: invalid operands to binary expression
      ('std::function<void (int)>' and '(lambda at fn.cpp:7:13)')
```

**`std::function` cannot be compared for equality.**
Only comparison with `nullptr` is defined
(the standard does not know what is inside, so there is no way to compare it).
So the moment you store `std::function` objects, **"who registered this" disappears from the code.**

Also, nobody watches the lifetime of the references that the lambda captures.

```cpp
{
  Display display;
  hub.add_callback([&display](int mm) { display.show(mm); });
}                     // display dies. the lambda still holds the dead display
hub.publish(200);     // exactly the same undefined behavior as 17.2
```

**The problem is not solved by even 1 millimeter.** The dangling reference has only moved
from a class member to a lambda capture, and **it just became harder to see**.

That is why **you need a token**. If `add_callback` also returns a token,
the `std::function` style becomes safe too. In fact, decent signal/slot
libraries (such as Boost.Signals2) always return an object that represents the "connection".

| | Virtual function (`SensorObserver`) | `std::function` |
| --- | --- | --- |
| Ease of registering | You must write a class | One line of lambda |
| Who registered | Known by pointer | **Unknown** (cannot compare) |
| Heap allocation | None | **Happens if the capture is large** |
| Microcontroller | Usable | Practically unusable |
| Unsubscribing | Can remove by pointer or id | **Impossible without a token** |

The exercise is written with the virtual function version. The `std::function` version appears again in chapter 22 (Command).

## 17.7 Does the standard library have the same thing?

**The C++ standard library has no Observer.**
`std::observer_ptr` has a similar name but is **completely unrelated** (it is only a type that
represents a non-owning pointer, and it is not in C++17 either).

Close things are:

- **`std::weak_ptr`** — a part for "check that the other side is alive before touching it". It is exactly Method 2 of 17.3
- **`std::function`** — a part that holds the notification target with type erasure. As in 17.6, you must handle unsubscribing yourself
- **`std::condition_variable`** — "wait / wake" between threads. The purpose is different,
  but it is the same in that it "tells others that the state has changed"

Since the standard has no such thing, **using an external library for signals/slots is normal in practice**.
Boost.Signals2, Qt's `signals` / `slots`, and `sigslot` are examples.
All of them are designed to **return a connection object (= a token)**.

If your club library needs an Observer in only one place, **writing it yourself is faster**
(about 150 lines in the exercise code). If it grows to 3 or more places and you also want thread safety,
consider a library. **To make that decision, write it yourself once.**

## 17.8 Try it yourself

Before solving the exercise, compile this and **predict the output** before you run it.
It is the smallest form of 17.4, "the list changes during notification".

```cpp
#include <algorithm>
#include <cstdio>
#include <vector>

class SensorObserver
{
public:
  virtual ~SensorObserver() = default;
  virtual void on_sample(int value_mm) = 0;
};

class SensorHub
{
public:
  void add_observer(SensorObserver * observer) { observers_.push_back(observer); }

  void remove_observer(SensorObserver * observer)
  {
    observers_.erase(
      std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
  }

  void notify(int value_mm)
  {
    for (SensorObserver * observer : observers_) {
      observer->on_sample(value_mm);
    }
  }

private:
  std::vector<SensorObserver *> observers_;
};

class SelfRemoving : public SensorObserver
{
public:
  SelfRemoving(SensorHub * hub, int id)
  : hub_(hub),
    id_(id)
  {
  }

  void on_sample(int value_mm) override
  {
    std::printf("observer %d got %d\n", id_, value_mm);
    hub_->remove_observer(this);
  }

private:
  SensorHub * hub_;
  int id_;
};

int main()
{
  SensorHub hub;
  SelfRemoving a{&hub, 1};
  SelfRemoving b{&hub, 2};
  SelfRemoving c{&hub, 3};
  hub.add_observer(&a);
  hub.add_observer(&b);
  hub.add_observer(&c);

  hub.notify(42);
  std::puts("returned from notify");
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: how many lines are printed, and which observers are called how many times</summary>

It is not 3 lines with 1, 2, 3 once each. This is what happened on my machine.

```
observer 1 got 42
observer 3 got 42
observer 3 got 42
returned from notify
```

**Number 2 is skipped and number 3 is called twice.**

When observer 1 removes itself, the `vector` shrinks to `{2, 3}`,
and the loop iterator moves on while still pointing at the second element (= 3).
In the third iteration, it reads **out of the range** of the already shrunk `vector`.

The compiler gives no warning. The program also exits normally.
**You will never find this kind of breakage unless you write a test.**
The exercise test "does not crash even if another observer is removed during notification" checks this.

Note that this output changes depending on the environment (of course, since it reads out of range).
**"I got a different result on my machine" is evidence of undefined behavior, not a counterargument.**
</details>

## 17.9 Conclusion for microcontrollers

The exercise code **cannot run on a microcontroller as it is**. There are 3 things you cannot use.

| What is used | Why it does not fit |
| --- | --- |
| `std::vector` | Heap allocation on every subscription. It fragments memory |
| `std::shared_ptr` / `std::weak_ptr` | Heap allocation for the control block. Atomic counter operations are also added |
| `std::function` | Heap allocation if the capture is large |

We replace them. **We line up raw pointers in a fixed-length array.**

```cpp
class SensorObserver
{
public:
  virtual void on_sample(int value_mm) = 0;

protected:
  // Non-virtual protected destructor.
  // We decided "never delete through a base class pointer", so we do not add
  // a destructor slot to the vtable.
  ~SensorObserver() = default;
};

template <std::size_t Capacity>
class SensorHub
{
public:
  bool subscribe(SensorObserver * observer)
  {
    if (observer == nullptr) {
      return false;
    }
    for (std::size_t i = 0; i < Capacity; ++i) {
      if (observers_[i] == nullptr) {
        observers_[i] = observer;
        return true;
      }
    }
    return false;                 // full. we cannot throw an exception, so return a bool
  }

  void unsubscribe(SensorObserver * observer)
  {
    for (std::size_t i = 0; i < Capacity; ++i) {
      if (observers_[i] == observer) {
        observers_[i] = nullptr;  // do not close the gap. safe even during the notification loop
        return;
      }
    }
  }

  void publish(int value_mm)
  {
    if (notifying_) {
      return;                     // re-entrancy guard
    }
    notifying_ = true;
    for (std::size_t i = 0; i < Capacity; ++i) {
      SensorObserver * observer = observers_[i];
      if (observer != nullptr) {
        observer->on_sample(value_mm);
      }
    }
    notifying_ = false;
  }

private:
  SensorObserver * observers_[Capacity] = {};
  bool notifying_ = false;
};
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti micro.cpp -o micro
```

This compiles with zero warnings under `-fno-exceptions -fno-rtti`. **There are zero allocations.**

There are 4 points.

**1. Do not write a virtual destructor (the only exception in this chapter)**

We use a `protected` non-virtual destructor.
Since we decided in the design "never `delete` through a base class pointer",
we saved one vtable slot. Because it is `protected`,

```cpp
void f(SensorObserver * p) { delete p; }
```

becomes a compile error.

```
error: calling a protected destructor of class 'SensorObserver'
note: declared protected here
```

If **a convention violation becomes a compile error**, it is a mechanism, not a convention.
Trying to create a `unique_ptr<SensorObserver>` is also an error.
On a microcontroller, observers are placed statically, so this is not a problem.

**Do not bring this form into ROS 2 code.** There are normal cases where objects are created dynamically
and released through a base pointer, so a virtual destructor is the right answer there.

**2. Removing only "makes a hole". Do not close the gap**

Since the array has a fixed length, there is no reason to close the gap. We just skip over the `nullptr` holes.
The deferred removal implementation becomes unnecessary in the first place.

**3. Return "full" as a `bool`**

Because of `-fno-exceptions`, we cannot `throw`. We return it as a return value.
It is even better to add `[[nodiscard]]` so that ignoring it gives a warning.

**4. Do not call `publish()` from an ISR**

This is the biggest cause of accidents. **Do not run the notification loop inside an interrupt handler.**

- You do not know what the notification targets do. If they do `printf` or I2C transmission, the interrupt becomes long
- It causes a data race with `subscribe` / `unsubscribe`.
  If an interrupt comes while the main loop is writing `observers_[i]`,
  you read a half-written pointer
- The `notifying_` flag also cannot be protected against interrupts

Let the interrupt **only store a value and set a flag**, and do the notification in the main loop.

```cpp
static volatile int g_latest_mm = 0;
static volatile bool g_has_new = false;

extern "C" void EXTI0_IRQHandler(void)
{
  g_latest_mm = read_distance_mm();
  g_has_new = true;               // 32-bit aligned access, so a single assignment is not split
}

int main()
{
  SensorHub<4> hub;
  Display display;
  hub.subscribe(&display);

  for (;;) {
    if (g_has_new) {
      g_has_new = false;
      const int value_mm = g_latest_mm;   // copy to a local variable before using it
      hub.publish(value_mm);              // notify on the main loop side
    }
  }
}
```

**One more reminder about `volatile`.**

`volatile` only tells the compiler "do not remove or reorder this by optimization".
**It does not guarantee atomicity. It cannot be used for synchronization.**
The code above works because

- the roles are **separated**: only the ISR writes, and only the main loop reads
- it only does **single reads and writes** of `int` and `bool` (aligned
  32-bit / 8-bit accesses on Cortex-M are not split)

both hold. The 3 steps of read, add, and write, as in `g_counter++`,
**can be split by an interrupt**.
There, surround it with interrupt disabling, or use `std::atomic`.

If there is a chance that the subscription list (`observers_`) is touched during an interrupt,
you need to surround `subscribe` / `unsubscribe` with interrupt disabling.
**To avoid needing that, finish all subscriptions at startup.**
This is the safest and fastest design.

## 17.10 Conclusion for ROS 2 (supplement)

**ROS 2 pub/sub is Observer itself.**
Before you write your own Observer, first think about whether it is enough.

| Observer pattern | ROS 2 |
| --- | --- |
| Subject | Publisher (plus the middleware) |
| Observer | Subscription callback |
| `add_observer` | `create_subscription` |
| Unsubscribing | Discard the Subscription object |

What `create_subscription` returns is `rclcpp::Subscription<T>::SharedPtr`.
**This is exactly the token style of 17.3**, and the `shared_ptr` reference count is the lifetime of the subscription.

```cpp
sub_ = this->create_subscription<sensor_msgs::msg::Range>(
  "range", 10, std::bind(&MyNode::on_range, this, std::placeholders::_1));
```

If you do not keep the return value in a member, the subscription ends right there.
**If you throw away the return value, the subscription does not start**, exactly like `Subscription` in the exercise.

If you only want to "distribute sensor values to several internal classes" inside a node,
your own Observer can be lighter than adding one more topic
(because it does not go through serialization or the middleware).
**Topics across nodes, your own Observer inside a node** is a good rule of thumb.

Also, if you call an API of the same node from inside an `rclcpp` callback,
the Executor may get stuck because of re-entry. **The re-entry problem is the same kind as 17.5.**
Whether you write it yourself or use a framework, you trip at the same place.

## 17.11 Common pitfalls

| Symptom | Cause |
| --- | --- |
| After removing an observer, a function of a different class was called on notification | 17.2. A raw pointer is dangling. Use the token style |
| A notification skipped one observer and called another one twice | 17.4. You `erase` during the notification loop. Use deferred removal |
| The program does not return after a notification | 17.5. Notifications form a loop. Add a re-entrancy guard flag |
| After an exception happened once, notifications never came again | The exception left before the re-entrancy flag was reset. Reset it with RAII |
| The program crashed when `subscribe` was called during a notification | The `vector` was reallocated. Do not keep references or iterators. Iterate by index |
| An observer registered during a notification received the notification of that round | The count was not fixed before the loop |
| After moving a `Subscription`, the subscription ended | The moved-from object was not emptied. Same as `unique_ptr` |
| After `SensorHub` was destroyed, destroying a token crashed | The token holds the Subject by a raw pointer. Use `weak_ptr` |
| Something registered with a lambda cannot be unsubscribed | 17.6. `std::function` cannot be compared. Design it to return a token |
| `std::vector<SensorObserver>` does not compile | An abstract class cannot be held by value. Use a pointer or `shared_ptr` |
| A subscriber receives a notification twice | The same observer was `subscribe`d twice. Count by id |

## 17.12 Matching exercise

```bash
./drill run dp17
```

In `exercises/dp17_observer/src/sensor_hub.cpp`, you implement

1. `detail::Registry::remove()` / `compact()` — **deferred removal**
2. The destructor, move operations, and `reset()` / `active()` of `Subscription` — **the RAII token**
3. `SensorHub::subscribe()` / `publish()` / `observer_count()` — notification with a re-entrancy guard

There are 10 tests. They check not only that notifications arrive, but also that

- **the Subject does not break even if an observer dies first** (the token unsubscribes automatically)
- **it does not crash even if you unsubscribe during a notification** (a removed observer gets no notification)
- **a notification loop does not cause an infinite loop**
- **destroying a token is safe even if the Subject dies first**

**The breakages that we observed in 17.2, 17.4, and 17.5 are, as they are, the tests.**

## 17.13 Summary of this chapter

- If you port Java's `addObserver()` as it is, **it breaks the moment the subscriber dies first**.
  And since it **does not crash**, you cannot notice it
- The cause is that `add_observer` returns `void`.
  **"Who owns this subscription and when it ends" is not written in the type**
- There are 3 solutions. Unsubscribe in the destructor (you forget it, and the Subject's lifetime is required),
  hold with `weak_ptr` (observers must be `shared_ptr`),
  and the **token style (RAII) is the safest**
- Give the token the same properties as `unique_ptr`. **No copy, movable, and the moved-from object is empty**
- Hold the real subscription list with `shared_ptr`, and let the token watch it with `weak_ptr`.
  Then it is **safe whichever dies first**
- **Do not `erase` from the subscription list during the notification loop.** Mark it, and remove it afterwards
- **Do not keep references or iterators.** Iterate by index. Fix the count before the loop
- **Reset the re-entrancy guard flag with RAII.** So that it is reset even when an exception leaves the function
- **`std::function` cannot be compared**, so you cannot identify and remove what you registered.
  That is why you need a token
- On microcontrollers, use raw pointers in a fixed-length array. **Zero allocations**.
  **Do not notify from an ISR**. Only store a value and set a flag
- **`volatile` is not atomic.** It only stops optimization
- ROS 2 pub/sub is Observer itself. The return value of `create_subscription` is the **token**

---

Previous: [16. Mediator](16_Mediator.md) / Next: 18. Memento (coming soon)
