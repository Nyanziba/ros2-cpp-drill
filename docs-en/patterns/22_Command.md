# 22. Command

> **Matches chapter 22 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep `Command` / `MacroCommand` / `DrawCommand`, and
> the `MacroCommand.undo()` part, open next to you.
>
> **Goal of this chapter**: C++ already has a Command. **It is `std::function<void()>`.**
> You need no class and no inheritance. But with `std::function`, **you cannot write undo.**
> You need 2 operations, "execute" and "cancel", and that is the first time you make it a class.
> After setting up this decision rule, you write undo / redo and a macro command.
> Finally, you do the same thing **on a microcontroller where you cannot use `std::function` or `std::vector`.**

## 22.1 Porting the Java version to C++ as is

This is the `Command` interface in the book.

```java
public interface Command {
    public abstract void execute();
}
```

If you port it to C++ in a straightforward way, you get this.

```cpp
class Command
{
public:
  virtual ~Command() = default;
  virtual void execute() = 0;
};
```

There is only one change: the **virtual destructor.** It is the same since chapter 1.
You hold and destroy it with `std::unique_ptr<Command>`, so without it the derived destructor is not called.

The problem is the next one, `MacroCommand`. The book writes it like this.

```java
public class MacroCommand implements Command {
    private Deque<Command> commands = new ArrayDeque<>();
    public void append(Command cmd) { commands.push(cmd); }
}
```

The decisions you make when porting to C++ concentrate here.

### Change 1: `Deque<Command>` becomes `std::vector<std::unique_ptr<Command>>`

```cpp
std::vector<Command> commands_;                    // compile error. You cannot hold an abstract class by value
std::vector<Command *> commands_;                  // who calls delete?
std::vector<std::unique_ptr<Command>> commands_;   // this one
```

The first line does not compile at all, because `Command` is pure virtual. Even if you made it a concrete type,
putting a derived class into `std::vector<Command>` causes **slicing** and the derived part disappears.

The second line works, but **the ownership is not written in the type.**
The book's `MacroCommand` keeps every `Command` that was `append`ed.
In C++, "keep" means "own", so write it with `unique_ptr`.

**The Command pattern has almost the same structure as Composite (chapter 11).**
`MacroCommand` implements `Command` and also holds `Command`s.
The discussion about ownership is exactly the same as in chapter 11.

### Change 2: `append(Command cmd)` becomes `add(std::unique_ptr<Command> command)`

There are 3 ways to receive the argument, and only one is right.

```cpp
void add(std::unique_ptr<Command> command);          // take by value. This one
void add(const std::unique_ptr<Command> & command);  // it cannot be copied, so you can neither move nor push inside
void add(std::unique_ptr<Command> && command);       // it works, but the caller must always write std::move
```

**Take it by value and `std::move` it into the container.** This is the way to use `unique_ptr`.
Then "the caller gave up ownership" shows in the look of the call site (`std::move(cmd)`).

### Change 3: The receiver does not own

`RotateCommand` operates a robot arm, but it **does not own the arm.**
The arm belongs to someone else.

```cpp
class RotateCommand : public Command
{
public:
  RotateCommand(RobotArm & arm, double delta_deg) : arm_(&arm), delta_deg_(delta_deg) {}
private:
  RobotArm * arm_;      // does not own. A raw pointer is a statement of "does not own"
  double delta_deg_;
};
```

If you hold a `unique_ptr`, you own it, and if you hold a `shared_ptr`, you extend its lifetime.
**If you do not own it, use a raw pointer or a reference.** But there is a cost (22.5).

## 22.2 Doubt first — if you do not need undo, `std::function` is enough

This is the biggest difference in C++.

In Java, the biggest reason to use the Command pattern was
"**to carry a method call around as an object.**"
Before Java 8, a class was the only way to do that.

C++ has had a way from the start.

```cpp
std::vector<std::function<void()>> queue;

queue.push_back([&arm]() { arm.rotate(30.0); });
queue.push_back([&arm]() { arm.set_gripper(true); });

for (const auto & action : queue) {
  action();
}
```

**The Command pattern is already complete.** You wrote no `Command` base class,
no `RotateCommand`, and no `GripperCommand`.
The purpose of Command, "turn an operation into a value and run it later", is fully achieved.

So when do you make it a class?

| Requirement | Means |
| --- | --- |
| Only execute | `std::function<void()>` |
| Execute + a name for logging | `struct { std::string name; std::function<void()> action; }` |
| **Execute + cancel (undo)** | **A class.** You need 2 things, `execute()` and `undo()` |
| Execute + cancel + serialization (save, communication) | A class. Or a POD struct (22.8) |

The decision rule is one line.

> **`std::function` can wrap only one operation. If you need two or more, use a class.**

You probably thought: can't I just hold 2 `std::function<void()>`s?

```cpp
struct Action
{
  std::function<void()> execute;
  std::function<void()> undo;      // it works, but...
};
```

It works. But the lambda on the `undo` side must capture "the state before execution", and
where and when that capture is taken is hidden inside `execute`.
If the 2 functions share the same state, that is **state that should be in a class.**
A class with `execute()` and `undo()` is shorter than 2 `std::function`s side by side.

**"Always consider the option of not making a class" is the response to section 0.5 (before you use them) of this course.**
Before you write 5 Command classes in a team library, ask whether you really need undo.

## 22.3 The command queue, and who owns it

Once you decide you need undo, the queue looks like this.

```cpp
class CommandHistory
{
public:
  void run(std::unique_ptr<Command> command)
  {
    command->execute();
    undone_.clear();                        // the history branched, so discard redo
    done_.push_back(std::move(command));
  }

private:
  std::vector<std::unique_ptr<Command>> done_;
  std::vector<std::unique_ptr<Command>> undone_;
};
```

**The history owns the commands.** You cannot delete a command after it finishes running.
You need to keep it in order to undo.
This is the structural difference from a `std::function` queue, where "it is over once it runs".

A macro command has the same shape.

```cpp
class MacroCommand : public Command
{
public:
  void add(std::unique_ptr<Command> command) { commands_.push_back(std::move(command)); }

  void execute() override
  {
    for (const auto & command : commands_) { command->execute(); }
  }

  void undo() override
  {
    for (std::size_t i = commands_.size(); i > 0; --i) { commands_[i - 1]->undo(); }
  }

private:
  std::vector<std::unique_ptr<Command>> commands_;
};
```

The key point is that `undo()` goes in **reverse order.** To cancel "grab, then lift",
you put it down first and then release. If you undo in forward order, the state breaks for operations that depend on each other.

`MacroCommand` itself is also a `Command`, so **you can put a macro inside a macro.**
This is exactly Composite (chapter 11), and from the view of `CommandHistory`,
a macro is one command. One undo brings everything back.

## 22.4 Undo and redo — how to choose between this and Memento (chapter 18)

In chapter 18, I wrote that "there are 2 ways to do Undo". This is the second one.

| | Memento (chapter 18) | Command (this chapter) |
| --- | --- | --- |
| What it holds | A **copy of the state** after the operation | The operation and its **inverse operation** |
| How to go back | Write the state back | Run the inverse operation |
| Cost of one history step | The size of the whole state | The size of a command (usually a few bytes) |
| Can you jump to any point? | Yes | You can go back only one step at a time |
| When the state is huge | Hard | Good at it |

**If you can write the inverse operation, use Command. If you cannot, use Memento.**

### What to do when you cannot write the inverse operation

There are 3 cases.

**1. The inverse operation is obvious** — a relative operation. You save nothing.

```cpp
void RotateCommand::execute() { arm_->rotate(delta_deg_); }
void RotateCommand::undo()    { arm_->rotate(-delta_deg_); }
```

**2. You can write the inverse operation, but you need the state before execution** — an operation that sets an absolute value.
You save the previous value inside `execute()`. This is **a small Memento inside a Command.**

```cpp
void GripperCommand::execute()
{
  previous_ = arm_->gripper_closed();   // save the state before execution
  arm_->set_gripper(closed_);
}
void GripperCommand::undo() { arm_->set_gripper(previous_); }
```

Note that the inverse of "close" is not "open".
**If it was already closed, staying closed after undo is the right answer.**
If you write `set_gripper(!closed_)` in `undo()`, that is a bug.

**3. It cannot be cancelled at all** — the motor turned and the robot moved, you wrote to EEPROM,
you sent a frame on CAN. You cannot undo these. **Do not write what you cannot write.**
The most dangerous way is to implement `undo()` as "do nothing" and let it pass silently. There are 2 choices.

- Do not push that command to the history (call `execute()` directly, not `run()`)
- Add `bool is_undoable() const`, and `CommandHistory::undo()` stops there

For a team library, the former is enough. If you decide at the start, as a design rule, that
**"a command that moves real hardware cannot be undone"**, you will not agonize later.

Also, **when the state is huge and the inverse operation is a pain**, the standard way is to use both.
Take a Memento once every N times, and fill the gaps with Commands.

## 22.5 Dangers specific to C++

### The command outlives the receiver

It is exactly the same problem as Observer in chapter 17.

```cpp
CommandHistory history;
{
  RobotArm arm;
  history.run(std::make_unique<RotateCommand>(arm, 30.0));
}                                  // arm dies here
history.undo();                    // touches a dead arm. Undefined behavior
```

`RotateCommand` holds a `RobotArm *`.
**In Java, the GC keeps the arm alive, so it does not crash. In C++, it crashes.**
And the history "keeps holding after the run is over", so
it holds the reference **for a longer time** than Observer, which makes it more dangerous.

The remedy is the same 3 choices as in chapters 1 and 17.

| Method | When to use |
| --- | --- |
| Make the receiver outlive the command (a convention) | When the receiver is long-lived, like a single `RobotArm`. **Usually this one** |
| Hold `std::weak_ptr<RobotArm>` and use it after `lock()` | When you cannot predict the receiver's lifetime |
| Empty the history when the receiver dies | When the same class can hold both the receiver and the history |

**The first one is the first choice.** A robot's arm and motors are created at startup and live until the end.
If your design makes only the command history outlive them, the design is wrong.

### 3 pitfalls of `std::function`

If you choose `std::function`, 3 C++-specific problems come with it.

**1. It may allocate on the heap.** If the capture is small, it fits in the internal buffer
(Small Object Optimization), but if it is large, an allocation happens. How many bytes fit is **not fixed by the standard.**
I measure it in 22.7.

**2. It cannot wrap something that is not copy-constructible.** `std::function` is a copyable type,
so it requires the content to be copyable too. A lambda that captures a `unique_ptr` does not fit.

```cpp
std::function<void()> action = [p = std::make_unique<int>(3)]() { (void)p; };
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// move_only_capture.cpp
#include <functional>
#include <memory>

int main()
{
  std::function<void()> action = [p = std::make_unique<int>(3)]() { (void)p; };
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic move_only_capture.cpp -o move_only_capture
```

</details>

```
In file included from /usr/include/c++/13/functional:59,
                 from move_only_capture.cpp:2:
/usr/include/c++/13/bits/std_function.h: In instantiation of ‘std::function<_Res(_ArgTypes ...)>::function(_Functor&&) [with _Functor = main()::<lambda()>; _Constraints = void; _Res = void; _ArgTypes = {}]’:
move_only_capture.cpp:7:78:   required from here
/usr/include/c++/13/bits/std_function.h:439:69: error: static assertion failed: std::function target must be copy-constructible
  439 |           static_assert(is_copy_constructible<__decay_t<_Functor>>::value,
      |                                                                     ^~~~~
/usr/include/c++/13/bits/std_function.h:439:69: note: ‘std::integral_constant<bool, false>::value’ evaluates to false
```

`std::move_only_function` in C++23 solves this, but **you cannot use it in C++17.**
The workaround is to switch to `shared_ptr`.

**3. A reference capture does not extend the lifetime.** This is the one you step on most often.

```cpp
ActionQueue queue;
{
  RobotArm arm;
  queue.push([&arm]() { arm.rotate(30.0); });   // reference capture
}
queue.run_all();                                 // touches a dead arm
```

**`[&]` in a lambda only holds a pointer.** There is no GC and no reference counting.
The moment you write `[&]` in something that "runs later", you need a promise about lifetime.
Make a lambda that you put in a queue `[=]` or an explicit capture, and
if it touches an external object, decide the lifetime with the 3 choices at the start of 22.5.

## 22.6 Is there the same thing in the standard library / language?

**Yes. And there are several.**

| Standard tool | Which aspect of Command |
| --- | --- |
| `std::function<void()>` | Carries an operation around as a value. **No undo** |
| Lambda expression | Creates a command on the spot (the counterpart of a Java anonymous class) |
| `std::bind` / `std::invoke` | Bundle arguments and then call. But `std::bind` can be written with a lambda |
| `std::packaged_task<void()>` | A command + a place to receive the result (`std::future`). For passing to a thread |
| Arguments of `std::thread` / `std::async` | "An operation to run later on another thread" = Command |

So **in C++, you write a "Command class" only when you need undo or serialization.**
In every other case, a lambda and `std::function` are enough.
This is the typical example of item 4 in the chapter 0 checklist, "is there the same thing in the standard library?"

## 22.7 Try it yourself

You measure when `std::function` allocates on the heap.
You replace `operator new` and count. **Predict first, and then run it.**

```cpp
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <new>

std::size_t g_alloc_count = 0;

void * operator new(std::size_t size)
{
  ++g_alloc_count;
  void * p = std::malloc(size);
  if (p == nullptr) {
    throw std::bad_alloc();
  }
  return p;
}

void operator delete(void * p) noexcept { std::free(p); }
void operator delete(void * p, std::size_t) noexcept { std::free(p); }

struct Big
{
  double a, b, c, d, e, f;
};

int main()
{
  int x = 1;
  Big big{1, 2, 3, 4, 5, 6};

  g_alloc_count = 0;
  std::function<void()> small = [x]() { (void)x; };
  std::cout << "small capture : " << g_alloc_count << " allocation(s)\n";

  g_alloc_count = 0;
  std::function<void()> large = [big]() { (void)big; };
  std::cout << "large capture : " << g_alloc_count << " allocation(s)\n";

  std::cout << "sizeof(std::function<void()>) = "
            << sizeof(std::function<void()>) << "\n";
  std::cout << "sizeof(Big) = " << sizeof(Big) << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: which one calls <code>new</code>, and how many times?</summary>

With Apple clang 17 (libc++), the result was this.

```
small capture : 0 allocation(s)
large capture : 1 allocation(s)
sizeof(std::function<void()>) = 32
sizeof(Big) = 48
```

A capture of one `int` fits in the internal buffer of `std::function`, so there are **zero allocations.**
A 48-byte capture does not fit, so there was **one heap allocation.**

The numbers are not what matters. What matters is that **this boundary is not fixed by the standard.**
It differs between libstdc++ and libc++, and it changes with versions too.
So "a small lambda is not allocated, so it is fine" is **not a portable basis.**

Code that assigns to a `std::function` inside a control loop (that is, rebuilds it every cycle)
always has the possibility of an allocation. You can accept this in ROS 2,
but you cannot write it on a microcontroller. The next section covers it.
</details>

## 22.8 Conclusion for microcontrollers

**Do not use `std::function`, `std::vector`, or `std::unique_ptr`.** The reason is in 22.7.
Besides, Command is most effective on a microcontroller.

> **An interrupt handler (ISR) must not do the work. Turn the work into a "command" and pass it to the main loop.**

If you turn a motor or hit I2C inside an ISR, you block other interrupts.
All the ISR does is **push "what should be done" into a fixed-length ring buffer.**
This is the Command that is written most often in embedded work.

```cpp
#include <cstddef>
#include <cstdint>

enum class MotorCommandKind : std::uint8_t
{
  kNone = 0,
  kRotate,
  kGrip,
  kRelease,
};

// No vtable. No heap. 4 bytes. It can be pushed from an ISR
struct MotorCommand
{
  MotorCommandKind kind = MotorCommandKind::kNone;
  std::int16_t argument = 0;
};

template <std::size_t Capacity>
class CommandRing
{
public:
  // If full, drop the oldest one and insert. Returns false if something was dropped
  bool push(const MotorCommand & command)
  {
    const bool was_full = (size_ == Capacity);
    buffer_[head_] = command;
    head_ = (head_ + 1) % Capacity;
    if (!was_full) {
      ++size_;
    }
    return !was_full;
  }

  bool pop(MotorCommand & out)
  {
    if (size_ == 0) {
      return false;
    }
    const std::size_t oldest = (head_ + Capacity - size_) % Capacity;
    out = buffer_[oldest];
    --size_;
    return true;
  }

  bool empty() const { return size_ == 0; }

private:
  MotorCommand buffer_[Capacity] = {};
  std::size_t head_ = 0;   // the position to write next
  std::size_t size_ = 0;
};
```

The user side looks like this.

```cpp
CommandRing<8> g_queue;

extern "C" void EXTI0_IRQHandler()      // interrupt. Only pushes
{
  g_queue.push(MotorCommand{MotorCommandKind::kGrip, 0});
}

int main()
{
  RobotArm arm;
  MotorCommand command;
  for (;;) {
    while (g_queue.pop(command)) {      // run in the main loop
      apply(arm, command);
    }
  }
}
```

`apply()` uses a `switch` instead of `virtual`.

```cpp
void apply(RobotArm & arm, const MotorCommand & command)
{
  switch (command.kind) {
    case MotorCommandKind::kRotate:
      arm.rotate(static_cast<double>(command.argument));
      break;
    case MotorCommandKind::kGrip:
      arm.set_gripper(true);
      break;
    case MotorCommandKind::kRelease:
      arm.set_gripper(false);
      break;
    case MotorCommandKind::kNone:
      break;
  }
}
```

Here is what you gave up and what you gained, compared with the Java version and the C++ class version.

| | Class version | POD + ring buffer version |
| --- | --- | --- |
| Heap allocation | Once per command | **Zero.** The buffer is allocated statically |
| Virtual function calls | Once per execution | Zero (`switch`) |
| Adding a kind of command | Add one class. Do not touch existing code | Fix both the `enum` and the `switch` |
| Argument type | Free | You must squeeze it into one `std::int16_t` |
| Can you push from an ISR? | **No** (an allocation happens) | Yes |

**You gave up "just add a class" and bought "zero allocation".**
On a microcontroller, the kinds of commands are fixed at 5 to 10, so this trade works.

**Make 2 design decisions explicitly.**

1. **What to do when it is full.** The code above "drops the oldest one".
   For teleoperation commands, the newest is right, and keeping an old command is more dangerous.
   On the other hand, if you "cannot drop any" (the commands are a procedure),
   `push` should not insert and should return `false`, and the caller should resend. **Do not drop silently.**
2. **A variable shared by the ISR and the main loop cannot be protected by `volatile`.**
   `size_` / `head_` above are updated with multiple instructions.
   In practice, you surround them with interrupt disabling, or assuming a single producer / single consumer,
   you use `std::atomic`. This is about mutual exclusion, not the Command pattern,
   so the exercise does not cover it. **But always think about it before you put it on real hardware.**

If you need undo, add one "value before execution" to the POD command as
`std::int16_t previous`, and you can do the same thing.
It is overwhelmingly cheaper than holding a whole Memento, so **Undo on a microcontroller is done on the Command side** as a rule.

## 22.9 Conclusion for ROS 2 (supplement)

In ROS 2 you may use `std::function`, so you rarely get a chance to write a Command class.
Even so, Command appears as is in 2 places in rclcpp.

- **The Executor is a command queue.** Subscriber callbacks, timers, and services
  enter a queue as "operations to run later", and `spin()` takes them out and runs them.
  The lambda you pass to `create_subscription` is a command as it is.
- **An action goal is a command.** `send_goal` turns the operation "run this motion" into a value
  and passes it over the network. `cancel_goal` also looks like Command,
  but it is an **interruption**, not an undo. You cannot undo the motion of real hardware (case 3 in 22.4).

When you want to write your own Command queue in ROS 2, first
doubt "isn't an Executor enough?" and "isn't a timer enough?"

## 22.10 Common pitfalls

| Symptom | Cause |
| --- | --- |
| The state is strange after undoing a macro | `undo()` runs in forward order. Go back **in reverse order from the end** |
| After undo, you did another operation, and redo broke the state | `run()` does not `clear()` the redo history |
| It crashes after `undo()` | You `move` after `pop_back()` of `done_.back()`. The order is reversed |
| `add(cmd)` does not compile | It takes a `unique_ptr` by value, but you did not `std::move` |
| `std::vector<Command>` does not compile | You cannot hold an abstract class by value. Even a concrete one slices. Use `unique_ptr` |
| It crashes when you undo the history | The receiver (`RobotArm`) held by the command died first (22.5) |
| A lambda does not fit in `std::function` | It captures a `unique_ptr`. It cannot wrap something that is not copy-constructible |
| The control period sometimes gets longer | You rebuild a `std::function` in the loop and a heap allocation happens |
| Undoing "close" opens it | You wrote `!closed_` in `undo()`. Save the value before execution and restore it |
| Commands pushed in an ISR are lost | The ring buffer is full. Review the capacity, or the policy for when it is full |

## 22.11 Matching exercise

```bash
./drill run dp22
```

In `exercises/dp22_command/src/robot_console.cpp`, you implement

1. `RotateCommand` — a command whose inverse operation is obvious
2. `GripperCommand` — a command that saves the state before execution
3. `MacroCommand` — treats several commands as one (`undo()` is in reverse order)
4. `CommandHistory` — undo / redo. `run()` discards the redo history
5. `ActionQueue` — the `std::function` version (no undo)
6. `MotorCommandRing` / `apply()` — POD + fixed-length ring buffer

The tests check that **commands run in the order they were pushed**, that **undo is in reverse order**,
that a macro is pushed to the history as one item,
that the execution logs of the `std::function` version and the class version **match**,
and that the ring buffer drops the oldest ones when it exceeds the capacity.

## 22.12 Summary of this chapter

- **`std::function<void()>` is the C++ version of Command.** Always consider it before you write a class
- **`std::function` can wrap only one operation. If you need undo, use a class**
- Java's `Deque<Command>` is `std::vector<std::unique_ptr<Command>>`.
  If you keep it, you own it
- `MacroCommand` is Composite (chapter 11). **`undo()` is always in reverse order**
- There are 3 ways to implement undo. **The inverse operation is obvious / save the state before execution / it cannot be cancelled at all.**
  Do not cover up the third with an "`undo()` that does nothing"
- Holding the whole state is Memento (chapter 18), and holding the inverse operation is this chapter.
  For a huge state, Command is overwhelmingly cheaper
- **A command can outlive its receiver.** It holds the reference for longer than Observer (chapter 17)
- `std::function` **may allocate on the heap / cannot wrap something that is not copy-constructible /
  `[&]` does not extend the lifetime**
- On microcontrollers, use **a POD command + a fixed-length ring buffer.**
  The ISR only pushes, and the main loop runs. State the policy for when it is full

---

Previous: [21. Proxy](21_Proxy.md) / Next: [23. Interpreter](23_Interpreter.md)
