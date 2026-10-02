# 19. State

> **Matches chapter 19 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep the `State` interface, `DayState` / `NightState`,
> and `SafeFrame` (the `Context`) open next to you.
>
> **Goal of this chapter**: In the Java version, `State` calls
> `context.changeState(NightState.getInstance())` inside `doClock(Context context, int hour)`.
> **If you write this the same way in C++, the rest of the member function runs after `this` has been destroyed.**
> We will check it by actually getting a SIGSEGV. After that, C++ has **three** ways to write a state machine:
> `enum` + `switch`, State classes, and `std::variant`.
> We write all three, confirm that they produce the same sequence of transitions, and decide which one to choose.
> The conclusion first: **for a robot state machine, `enum` + `switch` is often enough.**

## 19.1 What happens if you port the Java version as is

The `State` interface in the book looks like this.

```java
public interface State {
    public abstract void doClock(Context context, int hour);
    public abstract void doUse(Context context);
}
```

A straightforward port to C++ looks like this.

```cpp
class Context;

class State
{
public:
  virtual ~State() = default;                         // Change 1
  virtual void do_clock(Context & context, int hour) = 0;
  virtual void do_use(Context & context) = 0;
};
```

### Change 1: virtual destructor

Same as in chapter 1. If you destroy objects through a base pointer, it is undefined behavior without one.
We follow this rule in every chapter of this course.

### Change 2: `getInstance()` is a **function-local static**, not a Java static field

`DayState` in the book is a singleton.

```java
public class DayState implements State {
    private static DayState singleton = new DayState();
    public static State getInstance() { return singleton; }
}
```

If you write `static DayState singleton;` as a static data member of the class in C++,
you step on the chapter 5 trap: **the initialization order across translation units is undefined**.
Use a function-local static instead.

```cpp
const State * day_state()
{
  static const DayStateObject instance;   // initialized only once, the first time it is called
  return &instance;
}
```

### Change 3: **if a state object has no state, you do not need `unique_ptr`**

This is the biggest design difference from the Java version. We cover it in 19.2.

### Change 4: **remove** `context` from `doClock(context, hour)`

In the Java version, the state object receives `context` and calls `context.changeState(...)` itself.
**We do not do this in C++.** 19.3 is the core of this chapter.

## 19.2 Who owns the state objects

The Java `Context` just switches which `State` it holds. The GC takes care of the rest.
In C++, you must decide what "switch" means. Only one point decides it.

> **Does the state object have member variables?**

| If the state object | How to hold it | Heap allocation |
| --- | --- | --- |
| has no members at all | point a `const State *` at a `static` instance | **none** |
| has its own data per state | swap a `std::unique_ptr<State>` | once per transition |

`DayState` / `NightState` in the book have **no members at all**. That is why
the Java version makes them singletons. The same decision works in C++.

```cpp
class ClassStateMachine
{
public:
  MachineState state() const { return current_->id(); }

private:
  const State * current_ = state_object(MachineState::Stopped);   // just a pointer
};
```

`state_object()` only returns one of four `static` instances,
so **a transition is "one pointer assignment"**. That `make_unique` never runs
inside a robot control loop is worth a lot by itself.

On the other hand, once you want each state to hold data (for example, only the running state has a duty),
you must swap a `unique_ptr`. **When that happens, consider `std::variant`** (19.5).

## 19.3 The most important pitfall in this chapter — replacing yourself during a transition

In the Java version, `doClock` lets the state object itself rewrite the `Context`.

```java
public void doClock(Context context, int hour) {
    if (hour < 9 || 17 <= hour) {
        context.changeState(NightState.getInstance());
    }
}
```

This is fine in Java. `DayState` is a singleton, so nobody destroys it,
and even if it were destroyed, the GC keeps it alive while a reference remains.

**In C++, if you hold state objects in a `unique_ptr`, this line means the same as `delete this`.**

```cpp
void Running::handle(Context & context) override
{
  context.set_state(std::make_unique<Faulted>());  // this is deleted here
  std::cout << "exit: " << name() << "\n";         // member function of an already dead this
  ticks_ = ticks_ + 1;                             // write to already freed memory
  std::cout << "ticks=" << ticks_ << "\n";
}
```

Inside `context.set_state()`, the old `unique_ptr` is destroyed and `this` is gone.
The next 3 lines run **on freed memory**.

This is the result of putting it in one file and running it.

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic try_uaf.cpp -o uaf && ./uaf
echo $?
```

```
139
```

**There is not a single warning.** `-Wall -Wextra -Wpedantic` lets it pass.
It crashes with SIGSEGV (128 + 11 = 139) without printing even one line.
This is because the program dies before the characters pushed into `std::cout` are flushed.

The nasty part is that **if the state object is small and has no members, it may happen to work**.
If you remove `ticks_` from the example above, it becomes code that "looks like it works" on many environments.
The day you add one member, it crashes in a place that looks unrelated.

### Fix: return the transition as a return value, and let the caller do the replacement

Remove `Context` from `handle()`. **If you do not pass the Context to the state object,
the operation "replace myself" cannot even be written.**

```cpp
class State
{
public:
  virtual ~State() = default;
  virtual MachineState id() const = 0;

  /// Returns the State to move to. Return this if there is no transition. Does not touch the Context.
  virtual const State * handle(MachineEvent event) const = 0;

  virtual void on_enter(TransitionLog & log) const;
  virtual void on_exit(TransitionLog & log) const;
};
```

The Context side looks like this.

```cpp
bool ClassStateMachine::handle(MachineEvent event)
{
  const State * const next = current_->handle(event);   // receive it first
  if (next == current_) {
    return false;                                       // no transition, so do nothing
  }

  current_->on_exit(log_);    // the old state is still alive
  current_ = next;            // only now do we replace it
  current_->on_enter(log_);
  return true;
}
```

We expressed the rule "only the caller replaces the state" in a **type**, not as a convention.
This is also why `handle()` is a `const` member function. It does not rewrite its own contents either.

Even if you hold states in a `unique_ptr`, this form is safe.

```cpp
std::unique_ptr<State> next = current_->handle(event);  // create and return a separate object
current_->on_exit(log_);
current_ = std::move(next);   // the old object is destroyed on this line. Nobody uses it anymore
current_->on_enter(log_);
```

**This is the core of the C++ design of State.** If you copy the Java version, this part is the other way around.

## 19.4 Enter and exit actions — required for robots

`on_enter` / `on_exit` do not appear in the book, but on a real robot it is **dangerous without them**.

```
Running --emergency stop--> Faulted
```

In this transition, you must **stop the motor before leaving the state**.
If you write "stop the motor here too" for each target state, you will forget one as transitions grow.
Write **"always stop when leaving Running"** in one place.

```cpp
class RunningStateObject : public State
{
public:
  void on_exit(TransitionLog & log) const override
  {
    State::on_exit(log);        // default behavior of the base class
    log.record("motor:stop");   // on a real robot, set the motor output to 0 here
  }
};
```

Fix the order: **exit first, enter after**. If you reverse it,
you get the sequence "the old state's motor stop runs right after the new state's brake is applied".

There is one more thing to decide. **Do not run actions for an input that causes no transition.**

```cpp
if (next == current_) {
  return false;      // call neither on_exit nor on_enter
}
```

If `on_exit` -> `on_enter` runs when `Running` receives `Start` again,
**the motor stops for a moment and restarts**. On a real robot this causes an obvious accident.
Only if you want to use a "self transition" on purpose, use a separate API (such as `force_reenter()`).

## 19.5 Does the standard library or the language already have this

The State pattern itself is not in the standard library.
But **`std::variant` + `std::visit` meets State's requirement that "each state has a different type"
without inheritance.**

```cpp
struct StoppedState {};
struct IdleState {};
struct RunningState { std::uint8_t duty_percent = 60; };   // data specific to the state
struct FaultedState { MachineEvent cause; };

using StateVariant = std::variant<StoppedState, IdleState, RunningState, FaultedState>;
```

The transition is also written in the "return it as a return value" form.

```cpp
std::optional<StateVariant> VariantStateMachine::next_state(
  const StateVariant & current, MachineEvent event)
{
  return std::visit(
    [event](const auto & concrete) -> std::optional<StateVariant> {
      using T = std::decay_t<decltype(concrete)>;
      if constexpr (std::is_same_v<T, IdleState>) {
        if (event == MachineEvent::Start) {
          return StateVariant{RunningState{kCruiseDutyPercent}};
        }
      }
      // ... other states
      return std::nullopt;      // no transition
    },
    current);
}
```

What you get:

- **Zero heap allocation**. `variant` keeps its contents inside itself
- **Zero vtable**. `std::is_polymorphic_v<StoppedState>` is `false`
- **Each state can have its own data**. The cases where the State class version needs `unique_ptr` go away
- **The compiler checks for missing cases**. If you add a state type, the `visit` lambda fails to compile because it cannot handle every type

What you lose:

- **The set of state kinds is fixed at compile time**. You cannot add states at run time, for example by a plugin
- When the type list of the `variant` gets long, error messages become hard to read

### Check transitions at compile time

Once you have reached the `variant` version, you can go one step further and **reject forbidden transitions with types**.

```cpp
template <typename From, Ev E>
struct Transition;                                          // declaration only. Not defined

template <> struct Transition<Stopped, Ev::PowerOn> { using To = Idle; };
template <> struct Transition<Idle,    Ev::Start>   { using To = Running; };
template <> struct Transition<Running, Ev::Stop>    { using To = Idle; };
template <> struct Transition<Faulted, Ev::Reset>   { using To = Stopped; };
template <typename From> struct Transition<From, Ev::EStop> { using To = Faulted; };

template <Ev E, typename From>
typename Transition<From, E>::To go(const From &)
{
  return typename Transition<From, E>::To{};
}
```

Only the allowed transitions can be written.

```cpp
Stopped stopped;
auto idle    = go<Ev::PowerOn>(stopped);
auto running = go<Ev::Start>(idle);
auto faulted = go<Ev::EStop>(running);
auto back    = go<Ev::Reset>(faulted);

auto bad = go<Ev::Start>(faulted);   // from the faulted state to running. Cannot be written
```

The compile error for the last line (actual output):

```
error: no matching function for call to 'go'
   43 |   auto bad = go<Ev::Start>(faulted);
      |              ^~~~~~~~~~~~~
note: candidate template ignored: substitution failure [with E = Ev::Start,
      From = typename Transition<Running, (Ev)3>::To]:
      implicit instantiation of undefined template 'Transition<Faulted, Ev::Start>'
```

We achieved **"code that goes from the faulted state to running does not compile"**.
But in a real state machine, the trigger of a transition is decided **at run time** by sensor values or communication.
Then the caller of `go<...>` needs a `switch`, and you are back to a run-time decision.
This technique works for **procedures where the order of transitions is written directly in the code**
(a calibration procedure, a startup sequence, and so on). There it is powerful.
Do not try to write the whole state machine with it.

## 19.6 Comparing the three ways

We write the same transition rules in three ways, feed the same event sequence, and check in the exercise that **the same log comes out**.

| Way | How a state is represented | Heap | vtable | Per-state data | Can add kinds at run time |
| --- | --- | --- | --- | --- | --- |
| `enum` + `switch` | a value (1 byte) | none | none | cannot hold | no |
| State class (GoF) | a derived class | none if the state is empty | yes | can hold | **yes** |
| `std::variant` + `visit` | a sum type | none | none | can hold | no |

These are the sizes measured with the exercise header (Apple clang, arm64).

```
MachineState=1  StateVariant=8
```

**One question decides the choice.**

> **Does each state have its own data?**

- **No** -> `enum` + `switch`. Robot state machines often end up here
- **Yes, and the kinds are fixed** -> `std::variant`
- **Yes, and you want to swap kinds at run time** (plugins, loading a scenario) -> State classes

"I learned the State pattern, so I write it with State classes" is the move this chapter most wants you to avoid.
The same story as "abstracting when there is only one implementation" in [0. Before you use them](00_before_you_use_them.md)
is multiplied by the number of states. **Before you add 4 files, 4 classes, and a vtable for 4 states,
check whether one `switch` is enough.**

When you write the `switch`, do not write `default:`.
If you list every `enum` value, **the compiler tells you about a missing case the day you add a state**.
Writing `default:` removes that warning. This is a big strength of the `switch` version.

## 19.7 Try it yourself

Get the SIGSEGV of 19.3 with your own hands. **This is the main part of this chapter.**

```cpp
#include <iostream>
#include <memory>

class Context;

class State
{
public:
  virtual ~State() = default;
  virtual const char * name() const = 0;
  virtual void handle(Context & context) = 0;
};

class Context
{
public:
  Context();
  void set_state(std::unique_ptr<State> next) { state_ = std::move(next); }
  void request() { state_->handle(*this); }

private:
  std::unique_ptr<State> state_;
};

class Faulted : public State
{
public:
  const char * name() const override { return "Faulted"; }
  void handle(Context &) override {}
};

class Running : public State
{
public:
  const char * name() const override { return "Running"; }

  void handle(Context & context) override
  {
    context.set_state(std::make_unique<Faulted>());  // this is deleted here
    std::cout << "exit: " << name() << "\n";
    ticks_ = ticks_ + 1;
    std::cout << "ticks=" << ticks_ << "\n";
  }

private:
  int ticks_ = 0;
};

Context::Context()
: state_(std::make_unique<Running>())
{
}

int main()
{
  Context context;
  context.request();
  std::cout << "done\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try; echo $?
```

<details>
<summary>Predict: what is printed, and how many warnings appear</summary>

**There are 0 warnings.** And it crashes with 0 lines of output.

```
139
```

(`139` is the value of `echo $?`. It is 128 + SIGSEGV(11). With Apple clang, arm64 on my machine, it was the same all 3 times.)

The "exit: ..." line pushed into `std::cout` is not printed because the program dies before the flush.
**It is not "the output is cut off midway" but "nothing is printed"**. This is how this kind of bug looks.
If you look for the cause using logs, you cannot even tell that `handle()` was entered, so you get lost.

If you remove `ticks_ = ticks_ + 1;`, it may **look like it works** depending on the environment.
That is the most dangerous state. The fact that `this` is dead does not change.

The fix is "return the next state as a return value" from 19.3. If you remove `Context &` from `handle()`,
this code **cannot even be written**.
</details>

## 19.8 Conclusion for microcontrollers

Use **`enum` + a table-driven design**. Zero heap, zero vtable, no exceptions.
If you hold the transition table as a `constexpr` array, the state machine becomes 1 byte of RAM.

```cpp
#include <cstddef>
#include <cstdint>
#include <cstdio>

enum class St : std::uint8_t { Stopped, Idle, Running, Faulted, Count };
enum class Ev : std::uint8_t { PowerOn, Start, Stop, EStop, Reset, Count };

constexpr std::size_t kStates = static_cast<std::size_t>(St::Count);
constexpr std::size_t kEvents = static_cast<std::size_t>(Ev::Count);

// [current state][event] = next state. If it is itself, it means "ignore".
constexpr St kTable[kStates][kEvents] = {
  //            PowerOn      Start        Stop         EStop        Reset
  /* Stopped */ {St::Idle,    St::Stopped, St::Stopped, St::Faulted, St::Stopped},
  /* Idle    */ {St::Idle,    St::Running, St::Idle,    St::Faulted, St::Idle},
  /* Running */ {St::Running, St::Running, St::Idle,    St::Faulted, St::Running},
  /* Faulted */ {St::Faulted, St::Faulted, St::Faulted, St::Faulted, St::Stopped},
};

// Enter/exit actions can also be held in a table. Function pointers are the same tool as in chapter 9 of the C track.
using Action = void (*)();

void motor_stop() { std::printf("motor:stop\n"); }
void brake_engage() { std::printf("brake:engage\n"); }
void nothing() {}

constexpr Action kOnExit[kStates]  = {nothing, nothing, motor_stop, nothing};
constexpr Action kOnEnter[kStates] = {nothing, nothing, nothing, brake_engage};

class Machine
{
public:
  bool handle(Ev event)
  {
    const St next = kTable[static_cast<std::size_t>(state_)][static_cast<std::size_t>(event)];
    if (next == state_) {
      return false;
    }
    kOnExit[static_cast<std::size_t>(state_)]();
    state_ = next;
    kOnEnter[static_cast<std::size_t>(state_)]();
    return true;
  }

  St state() const { return state_; }

private:
  St state_ = St::Stopped;
};

// The table itself can be checked at compile time.
static_assert(kTable[static_cast<std::size_t>(St::Faulted)][static_cast<std::size_t>(Ev::Start)] ==
                St::Faulted,
              "the faulted state must not be left by anything other than Reset");
static_assert(sizeof(Machine) == 1, "the state machine is 1 byte");
```

This is the actual output of building with `-fno-exceptions -fno-rtti` and feeding `Start / PowerOn / Start / EStop / Start / Reset`.

```
--- Start
(ignored)
--- PowerOn
exit:Stopped
enter:Idle
--- Start
exit:Idle
enter:Running
--- EStop
exit:Running
motor:stop
enter:Faulted
brake:engage
--- Start
(ignored)
--- Reset
exit:Faulted
enter:Stopped
sizeof(Machine)=1 sizeof(kTable)=20
```

**1 byte of RAM, 20 bytes of ROM (+ the function pointer tables).** There is no dynamic allocation and no vtable.

Another benefit of the table-driven design is that **you can see the transition rules as one "table"**.
You can check by eye that "the whole emergency stop column is `Faulted`".
With the State class version, the rules are scattered over 4 files, so you have to visit 4 places to check.

Holding the enter/exit actions in a table of function pointers is
exactly the same tool as "9. Function pointers" in the C track.
If you think of **the GoF State as a C "table of function pointers" that was given the name vtable**,
it becomes easier to decide which to choose on a microcontroller.

Use the State class version on a microcontroller **only when you really need to swap states at run time**.
Even then, make the state objects `static` instances and do not use `unique_ptr`.

## 19.9 Conclusion for ROS 2 (supplement)

The ROS 2 lifecycle node (`rclcpp_lifecycle::LifecycleNode`) comes with
a **finished State pattern** as standard.

```
Unconfigured -> Inactive -> Active -> Inactive -> Finalized
```

You write transitions as **enter/exit action callbacks**: `on_configure()` / `on_activate()` / `on_deactivate()` / `on_cleanup()`.
This is exactly what we did in 19.4.
Returning success or failure through the return value (`CallbackReturn::SUCCESS` / `FAILURE`)
follows the same idea as "the caller does the replacement" in 19.3.

**Do not try to manage the start and stop state of the node itself with your own state machine.**
It is already provided. What you build yourself is the robot's "behavior state".

For the behavior state inside a node, writing it with `enum` + `switch` and
publishing it with `std_msgs::msg::String` or a custom message is the most readable in practice.
On the ROS 2 side, dynamic allocation and exceptions are free to use, so there is no cost problem even if you choose the State class version.
**I still recommend `enum` because the transition rules stay in one place.**

## 19.10 Common pitfalls

| Symptom | Cause |
| --- | --- |
| It crashes right after a transition. No log is printed | The state replaces itself inside `handle()` (19.3) |
| It started crashing the day you added a member to the state object | Same as above. While there were no members, it "happened to" work |
| The motor stops for a moment and restarts | `on_exit` -> `on_enter` runs for an input that causes no transition (19.4) |
| The motor stop runs after the brake is applied | Enter and exit are in the wrong order. **Exit first** |
| You added one state and shipped without noticing a missing case | You wrote `default:` in the `switch`. Remove it and a warning appears |
| Allocation runs in the control loop | States are held in a `unique_ptr`. If the state is empty, a `static` instance is enough (19.2) |
| The `std::visit` lambda gives a compile error | The return type differs per branch. Write the return type explicitly (`-> std::optional<StateVariant>`) |
| States grew and now there are 10 State classes | If states have no data, go back to `enum` (19.6) |

## 19.11 Matching exercise

```bash
./drill run dp19
```

The subject is the behavior state of a robot.

```
Stopped --PowerOn--> Idle --Start--> Running --Stop--> Idle
From any state, EmergencyStop --> Faulted
Faulted --Reset--> Stopped (it can be left only by a manual reset)
Combinations not in the table are ignored. The state does not change, and no enter/exit action runs
```

In `exercises/dp19_state/src/state_machine.cpp`, you implement **the same rules in three ways**.

1. `EnumStateMachine::next_state()` / `handle()` — `enum` + `switch`
2. The `handle()` of the four `State` derived classes, `state_object()`, and `ClassStateMachine::handle()` — the GoF version.
   Override `RunningStateObject::on_exit()` and `FaultedStateObject::on_enter()`
3. `id_of()` / `VariantStateMachine::next_state()` / `handle()` — `std::variant` + `std::visit`

What the tests check (13 tests):

- All 4 states x all 5 events (20 combinations) follow the transition table
- For a forbidden input, the state does not change, `handle()` returns `false`, and **no action runs**
- Enter and exit run in the right order (`exit:Running` -> `motor:stop` -> `enter:Faulted` -> `brake:engage`)
- **After a transition, the old state object is still alive** (verifying the safe design of 19.3).
  It also checks with `static_assert` that the type of `State::handle` is `const State * (State::*)(MachineEvent) const`
- The `variant` version is non-polymorphic (`static_assert(!std::is_polymorphic_v<...>)`)
- **The three implementations return the same transition sequence, the same return values, and the same log**

## 19.12 Summary of this chapter

- Calling `context.changeState(...)` from inside `handle()`, as in the Java version, **becomes `delete this` in C++**.
  Measured: SIGSEGV. 0 warnings
- The fix is to **return the next state as a return value, and let only the caller do the replacement**.
  If you remove `Context` from `handle()`, the dangerous code **cannot even be written**
- If the state object **has no members, a pointer to a `static` instance** is enough. Zero heap allocation
- Enter and exit actions are required for robots. **Exit first, enter after.**
  Run neither for an input that causes no transition
- C++ has three ways to build a state machine. **The criterion is "does each state have its own data?"**
- **If not, use `enum` + `switch`. For a robot state machine this is usually enough.**
  If you do not write `default:`, the compiler tells you about a missing case when you add a state
- `std::variant` is the best answer when "each state has data and the kinds are fixed". Zero heap and zero vtable
- On a microcontroller, use a `constexpr` transition table. 1 byte of RAM. The same thing as a C table of function pointers
- The ROS 2 lifecycle node is a finished State pattern. Do not build the node's startup state yourself

---

Previous: [18. Memento](18_Memento.md) / Next: 20. Flyweight (coming soon)
