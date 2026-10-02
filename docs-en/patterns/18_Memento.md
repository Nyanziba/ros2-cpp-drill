# 18. Memento

> **Matches Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 18.** Open `Gamer` / `Memento` / `Main` next to you.
> Look especially at the `Memento` class, where "`getMoney()` is public, `getFruits()` is package private".
> This split is the narrow interface / wide interface.
>
> **Goal of this chapter**: **This chapter is easier than the Java version.** It is one of the few such chapters out of the 23.
> The Java Memento also exists to prevent a problem: passing a reference to an object shares its contents.
> C++ has value semantics, so **the snapshot is complete the moment you return the state by value**.
> Instead, there are two C++-specific jobs: **access control with `friend`**, and
> **checking that you really hold the state by value**.

## 18.1 What happens if you port the Java version to C++ as is

The `Memento` in Yuki's book looks like this (key points only).

```java
public class Memento {
    int money;
    ArrayList<String> fruits;

    public int getMoney() { return money; }          // narrow interface
    Memento(int money) { ... }                       // wide interface (package private)
    void addFruit(String fruit) { ... }              // wide interface
    List<String> getFruits() { ... }                 // wide interface
}
```

Moved to C++, it looks like this (excerpt from the exercise header).

```cpp
class GainTuner;   // forward declaration

class GainSnapshot
{
public:
  const std::string & label() const { return label_; }   // narrow interface

private:
  friend class GainTuner;                                // <- replaces Java's package private

  GainSnapshot(double kp, double ki, double kd, std::string label)
  : kp_(kp), ki_(ki), kd_(kd), label_(std::move(label))
  {
  }

  double kp_;
  double ki_;
  double kd_;
  std::string label_;
};
```

There are three changes from the Java version.

### Change 1: There is no package private, so we used `friend class GainTuner;`

Yuki's book is designed to **depend on** Java's access control: "the wide interface of Memento is visible only from inside the `game` package".
**C++ has nothing equivalent to package private.** A namespace does not control access.
Even if you put everything in the same `namespace game`, all members are still visible.

The only tool C++ gives you to write this intent is `friend`.

```cpp
private:
  friend class GainTuner;   // ONLY GainTuner can see the contents
```

`friend` is often introduced as "a dangerous feature that breaks encapsulation". It is the opposite.

- Without `friend` -> you have to make the contents `public`. **The whole world can see them**
- With `friend` -> **only one class can see them**. And that one class is **named in the header**

`friend` is **a tool that widens the scope of encapsulation explicitly**, not a tool that punches a hole in it.
Memento is one of the few patterns where `friend` works correctly
(after `operator<<`, it is the most typical use).

Note that `friend` is **one-way**. `GainTuner` can see inside `GainSnapshot`, but
`GainSnapshot` cannot see inside `GainTuner`. Java's package private is two-way, so this is a difference.

### Change 2: We did not make anything like `getFruits()` / `addFruit()`

The Java version adds contents after creating the Memento (`addFruit`). That is natural if you assume a GC.
In C++, **pass everything in the constructor and never change it afterwards**.

```cpp
GainSnapshot create_snapshot() const
{
  return GainSnapshot{kp_, ki_, kd_, label_};   // Complete here. Immutable from now on
}
```

A Memento is "the state at one point in time". **If it changes after you create it, it is not a Memento.**
The Java version has `addFruit` because the state of `Gamer` is an `ArrayList`,
and it was hard to pass everything to the constructor. In C++, you only write one copy.

### Change 3: The return value is a **value** of `Memento` (not a pointer, not a reference)

`createMemento()` in the Java version returns `new Memento(...)`. A reference is returned.
In C++ you may want to return `std::unique_ptr<GainSnapshot>`, but **you do not need it in this chapter.**

```cpp
GainSnapshot create_snapshot() const;                     // <- this is enough
std::unique_ptr<GainSnapshot> create_snapshot() const;    // <- too much
```

The reason is in 18.2.

## 18.2 Why Memento becomes easy in C++

In Java, writing this causes an accident.

```java
// Java (bad example)
public State createMemento() {
    return this.state;      // A reference is returned. If the caller changes state, the contents of Gamer change too
}
```

**In Java, "return" means "pass a reference", so it is not a snapshot.**
That is why a Java Memento needs "a dedicated Memento class that you copy into".

In C++,

```cpp
GainSnapshot create_snapshot() const
{
  return GainSnapshot{kp_, ki_, kd_, label_};
}
```

both `kp_` and `label_` are **copied by value**. The moment you write it, it is a snapshot.
`std::string` and `std::vector` are also duplicated with all their contents when copied (a deep copy is the default).

> **This is the biggest difference in this chapter.**
> Half of the work of a Java Memento is "keeping references from leaking".
> In C++, that half is **already done by the language from the start**.
> What remains in C++ is only "who may see the contents of the Memento" (= `friend`).

## 18.3 A C++-specific danger: you think you hold it by value, but you do not

18.2 said "if you hold it by value, you get a deep copy". The reverse is that **if you do not hold it by value, nothing happens.**
Here are three common mistakes.

| Member of the Memento | Is it a snapshot? |
| --- | --- |
| `double` / `std::string` / `std::vector<double>` | **Yes** (copying duplicates the contents) |
| `const State &` / `State *` | **No**. If the original changes, what you see changes. If the original dies, it dangles |
| `std::shared_ptr<State>` | **No**. Of course: a smart pointer exists to "share" |

The third row is the real trap. The intuition "I used a smart pointer, so it is safe" works the wrong way here.

```cpp
class ByShared
{
public:
  explicit ByShared(std::shared_ptr<Trajectory> t) : t_(std::move(t)) {}
private:
  std::shared_ptr<Trajectory> t_;   // points to the same object as the original
};
```

What `shared_ptr` prevents is a **lifetime** problem, not **sharing**. It actually shares.
In a Memento, you may hold a `shared_ptr` only when you can guarantee that the target is **immutable**.
We measure this in 18.5.

One more thing. **Do not forbid copying a Memento.**

```cpp
class GainSnapshot
{
  GainSnapshot(const GainSnapshot &) = delete;   // <- do not do this
};
```

You stack the undo history in a `std::vector<GainSnapshot>`. When a `vector` reallocates, it
copies or moves its elements. If you remove both copy and move, even `push_back` does not compile.
The exercise test checks `std::is_copy_constructible<GainSnapshot>` with `static_assert`.

## 18.4 Restore cheaply with move: provide two `restore` functions

If the state contains `std::string` or `std::vector`, every `restore` runs a copy (= an allocation).
For the last use only, you can steal the contents.

```cpp
void restore(const GainSnapshot & snapshot);   // Copy. snapshot can be reused
void restore(GainSnapshot && snapshot);        // Steal. snapshot can no longer be used
```

The caller decides which one to call.

```cpp
tuner.restore(undo_stack[1]);                       // we want to keep it in the history, so the copy version
tuner.restore(std::move(undo_stack.back()));        // we throw it away, so the move version
undo_stack.pop_back();
```

There is one thing to watch in the move version.

```cpp
void GainTuner::restore(GainSnapshot && snapshot)
{
  label_ = std::move(snapshot.label_);
  snapshot.label_.clear();      // <- this is needed
}
```

**After a move, the contents of a `std::string` are "valid but unspecified" by the standard.**
Most implementations leave it empty, but a short string may keep its contents because of SSO (small string optimization).
If you **promise the caller "it is empty after the move", call `clear()` yourself**.
If you do not promise, just write in the header "a Memento cannot be reused after the move".
The exercise takes the former, writes it in the header, and checks it in a test.

This is not only about Memento. It is **a decision you make every time you write a function that receives a move**.

## 18.5 Try it yourself

We put "hold by value" and "hold by `shared_ptr`" side by side. **Predict the output first**, then run it.

```cpp
#include <iostream>
#include <memory>
#include <string>

struct Trajectory
{
  std::string name;
};

class ByValue
{
public:
  explicit ByValue(Trajectory t) : t_(std::move(t)) {}
  const std::string & name() const { return t_.name; }

private:
  Trajectory t_;
};

class ByShared
{
public:
  explicit ByShared(std::shared_ptr<Trajectory> t) : t_(std::move(t)) {}
  const std::string & name() const { return t_->name; }

private:
  std::shared_ptr<Trajectory> t_;
};

int main()
{
  Trajectory live{"startup trajectory"};
  auto live_shared = std::make_shared<Trajectory>(Trajectory{"startup trajectory"});

  const ByValue snapshot_value{live};
  const ByShared snapshot_shared{live_shared};

  // After saving, change the original.
  live.name = "tuned trajectory";
  live_shared->name = "tuned trajectory";

  std::cout << "Memento held by value      : " << snapshot_value.name() << "\n";
  std::cout << "Memento held by shared_ptr: " << snapshot_shared.name() << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: what do the two lines print?</summary>

```
Memento held by value      : startup trajectory
Memento held by shared_ptr: tuned trajectory
```

`ByShared` is **not a snapshot**.
Even if you make it `const ByShared`, the `const` applies to the `shared_ptr` itself,
not to the object it points to. `t_->name` can be modified.

Both are written with `const` and `explicit`, and look equally careful.
**The compiler gives no warning.** Only someone who checks "is the Memento member a value?" every time will notice.
</details>

You can also confirm that `friend` is working. Include the exercise header and write

```cpp
return saved.kp_ > 0.0 ? 0 : 1;    // touch the wide interface from outside GainTuner
```

and the build stops with this (actual output).

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// private_access.cpp
#include <string>
#include <utility>

class GainTuner;   // forward declaration

class GainSnapshot
{
public:
  const std::string & label() const { return label_; }   // narrow interface

private:
  friend class GainTuner;                                // <- replaces Java's package private

  GainSnapshot(double kp, double ki, double kd, std::string label)
  : kp_(kp), ki_(ki), kd_(kd), label_(std::move(label))
  {
  }

  double kp_;
  double ki_;
  double kd_;
  std::string label_;
};

class GainTuner
{
public:
  GainSnapshot create_snapshot() const
  {
    return GainSnapshot{kp_, ki_, kd_, label_};   // Complete here. Immutable from now on
  }

private:
  double kp_ = 1.0;
  double ki_ = 0.0;
  double kd_ = 0.0;
  std::string label_ = "initial";
};

int main()
{
  const GainTuner tuner;
  const GainSnapshot saved = tuner.create_snapshot();
  return saved.kp_ > 0.0 ? 0 : 1;    // touch the wide interface from outside GainTuner
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic private_access.cpp -o private_access
```

</details>

```
private_access.cpp: In function ‘int main()’:
private_access.cpp:45:16: error: ‘double GainSnapshot::kp_’ is private within this context
   45 |   return saved.kp_ > 0.0 ? 0 : 1;    // touch the wide interface from outside GainTuner
      |                ^~~
private_access.cpp:20:10: note: declared private here
   20 |   double kp_;
      |          ^~~
```

What corresponds to "`getFruits()` cannot be called from outside the package" in the Java version is
this error in C++.

## 18.6 Does the standard library or the language already have this?

**Memento itself is not in the standard library.** But all the ingredients are there.

| What you want to do | Standard tool |
| --- | --- |
| Copy the state and save it | **Copy constructor** (generated automatically). This is the core of Memento |
| The state can be of several kinds | `std::variant` |
| Represent the "not saved" state | `std::optional<Snapshot>` |
| Stack the history | `std::vector` / `std::deque` |

Look at the first row in particular. **If the state is gathered in a single class, you do not need a `Memento` class.**

```cpp
class GainTuner
{
public:
  GainTuner create_snapshot() const { return *this; }   // a copy of itself is the Memento
  void restore(const GainTuner & snapshot) { *this = snapshot; }
};
```

This works. Judged by the criterion of chapter 0 ("if I delete this abstraction, which changes become harder?"),
you need a dedicated Memento type only in one of these two cases:

1. **You want to save only part of the state** (you do not want to save communication handles or mutexes)
2. **You hand the saved data to the outside, but do not want to show its contents** (= you need `friend`)

The exercise is case 2. If neither 1 nor 2 applies, a copy of `*this` is enough.
"I learned the pattern, so I add a type" is the way to break things that chapter 0 talked about.

## 18.7 Conclusion for microcontrollers

If you keep the undo history in a `std::vector<Memento>`, this happens:

- Every `push_back` may allocate. **Allocation inside a loop is forbidden**
- The history keeps growing. There is no upper limit written, so you cannot tell when memory runs out
- If the state contains `std::string`, each saved item allocates even more

On a microcontroller, you redo all three.

1. Make the state a **POD** (trivially copyable)
2. Make the history a **fixed-length ring buffer**. When it exceeds the capacity, drop the oldest first
3. Save with a single `memcpy`

```cpp
#include <cstddef>
#include <cstring>
#include <type_traits>

// 1. The state is a POD. Do not put std::string in it
struct GainState
{
  double kp;
  double ki;
  double kd;
};

static_assert(
  std::is_trivially_copyable<GainState>::value,
  "GainState is saved with memcpy, so it must be trivially copyable");

// 2. Fixed-length ring buffer. Zero dynamic allocation
class GainHistory
{
public:
  static constexpr std::size_t kCapacity = 4;

  void push(const GainState & state)
  {
    std::memcpy(&buffer_[head_], &state, sizeof(GainState));   // 3. one memcpy
    head_ = (head_ + 1) % kCapacity;
    if (size_ < kCapacity) {
      ++size_;
    }
  }

  std::size_t size() const { return size_; }

  // back_index = 0 is the newest, 1 is one before
  GainState recent(std::size_t back_index) const
  {
    const std::size_t index = (head_ + kCapacity - 1 - back_index) % kCapacity;
    return buffer_[index];
  }

private:
  GainState buffer_[kCapacity] = {};
  std::size_t head_ = 0;   // position to write next
  std::size_t size_ = 0;
};
```

Writing the `static_assert` matters a lot. If someone later writes

```cpp
struct GainState
{
  double kp, ki, kd;
  std::string label;    // <- added
};
```

then **the build stops before `memcpy` breaks things**.
If you carry a `std::string` with `memcpy`, two `string` objects point to the same heap block,
and it is freed twice. Without the `static_assert`, this is a bug that passes the tests and crashes in the field.

This version **drops** access control with `friend`. The contents of `GainState` are public.
We chose POD so that `memcpy` can carry it, and that does not go together with encapsulation.
The decision is: **the microcontroller version puts "zero allocation and a save that cannot break" before "encapsulation"**.
If you do not write the decision down, the next reader will wonder "why is this public?".

We also fixed `kCapacity` to a value like `4` so that the RAM usage is
**fixed at compile time**. You can see how many bytes it uses from `sizeof(GainHistory)`.

## 18.8 Conclusion for ROS 2 (supplement)

On the ROS 2 side, there are almost no cases where you write your own Memento.

- To save and restore parameters, it is enough to copy and hold the values of `rclcpp::Parameter`.
  `get_parameters()` returns values, so that is already a snapshot
- If you want to roll back a node's state transitions, first check whether the state machine of a lifecycle node
  (the topic of chapter 19, State) can express it
- If the goal is "I want to reproduce it later", **recording with rosbag** is more practical than Memento

If you do write one, it is for "try, and go back if it is bad" with calibration values or gains.
That is essentially the same story as on the microcontroller side, and on the ROS 2 side a `std::vector<Snapshot>` is fine.

## 18.9 Common pitfalls

| Symptom | Cause |
| --- | --- |
| A Memento you saved changes together with the original when you change the original | The Memento holds a `shared_ptr` / reference / pointer. Make it a value |
| `error: 'double GainSnapshot::kp_' is private within this context` | As intended. `friend class GainTuner;` is working. Go through the Originator |
| You wrote `friend` but it is not visible | Wrong spelling of the class name, or no forward declaration. Write `class GainTuner;` first |
| You cannot `push_back` into `std::vector<Memento>` | You wrote `= delete` for copy / move of the Memento |
| After restoring with the move version, the contents of the Memento are sometimes left and sometimes empty, so it is not stable | A `std::string` after a move is unspecified. If you promise it, call `clear()` explicitly |
| Undo gets slower as it grows | You copy the whole state. Keep only the difference (-> chapter 22 Command) |
| You saved with `memcpy` on a microcontroller and got a double free | `std::string` or similar got into the state. Put `static_assert(is_trivially_copyable)` |
| `recent()` of the ring buffer is off by one | `head_` is "the position to write next". The newest is `head_ - 1` |

## 18.10 When the state is big, do not use Memento (a bridge to chapter 22)

Memento **copies the whole state**. If the state is 10 KB and you keep 100 undo levels, that is 1 MB.
On a microcontroller that is fatal, and on ROS 2 it is not pleasant either.

Instead, record "**what was done**".

| Method | What it records | How it restores | Cost |
| --- | --- | --- | --- |
| Memento | Snapshot of the state | Overwrite | State size x number of levels |
| Command (chapter 22) | The operation and its inverse | Run the inverse | Operation size x number of levels |

The difference "kp changed from 1.0 to 2.0" is two `double` values. Much smaller than the whole state.
Real-world undo (editors, CAD, parameter tuning GUIs) is almost all on the Command side.

Use Command **when you can define the inverse operation**, and Memento **when you cannot write the inverse, or it is a pain**.
Many implementations use both
(take a Memento once every N steps and fill the gaps with Command).
Chapter 22 writes the Command side.

## 18.11 Matching exercise

```bash
./drill run dp18
```

In `exercises/dp18_memento/src/gain_tuner.cpp`, implement:

1. **`GainTuner::create_snapshot()`**: return the current state as a `GainSnapshot` **by value**
2. **`GainTuner::restore(const GainSnapshot &)`**: restore by copying (the Memento can be reused)
3. **`GainTuner::restore(GainSnapshot &&)`**: restore by stealing. Call `clear()` after stealing
4. **`GainTuner::capture_state()` / `restore_state()`**: the POD path for microcontrollers
5. **`GainHistory::push()` / `size()` / `recent()`**: fixed-length ring buffer

The tests check that the **Memento does not change** even if you change the Originator after taking the snapshot,
that you can go back to any point with `std::vector<GainSnapshot>`,
that the `GainSnapshot` constructor cannot be called from outside (`static_assert` with `std::is_constructible`),
and that the ring buffer drops the oldest items first when it exceeds the capacity.

## 18.12 Summary of this chapter

- **In C++, Memento is easier than in Java.** The snapshot is complete the moment you copy by value
- C++ has nothing equivalent to Java's package private. **Write it with `friend class Originator;`**
- `friend` is not a tool that breaks encapsulation, but **a tool that widens the scope explicitly, by exactly one class**
- If a Memento member is a `shared_ptr` / reference / pointer, **it is not a snapshot**.
  What `shared_ptr` prevents is a lifetime problem, not sharing
- Do not forbid copying a Memento. You could no longer stack it in a `std::vector`
- Provide both a copy version and a move version of `restore`. **If you promise the state after the move, call `clear()` yourself**
- If the state is gathered in one class, **a copy of `*this` is enough**. Create a dedicated type only when you can state the reason
- On a microcontroller: POD + fixed-length ring buffer + `memcpy`.
  **Always put `static_assert(std::is_trivially_copyable<...>)`**
- If the state is big, keep differences with Command (chapter 22), not Memento

---

Previous: [17. Observer](17_Observer.md) / Next: [19. State](19_State.md)
