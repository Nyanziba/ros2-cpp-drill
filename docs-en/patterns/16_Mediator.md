# 16. Mediator

> **Matches chapter 16 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep the `Mediator` / `Colleague` interfaces and
> `LoginFrame`, `ColleagueCheckbox`, `ColleagueButton` open next to you.
>
> **Goal of this chapter**: Yuki's `LoginFrame` holds `Colleague`s in an array, and
> each `Colleague` points back to the `LoginFrame` with `setMediator()`. It is a structure where **they point at each other**.
> In Java, nothing happens because of this. If you write the same shape in C++ with `std::shared_ptr`,
> **the reference count never reaches 0, and neither side is ever released**.
> This chapter starts by actually leaking it and seeing that the destructor log does not appear.
> In addition, the Mediator pattern has a structural side effect: **adding it always leads to two-phase initialization**.

## 16.1 Porting the Java version to C++ as it is

Yuki's two interfaces look like this.

```java
public interface Mediator {
    public abstract void createColleagues();
    public abstract void colleagueChanged();
}

public interface Colleague {
    public abstract void setMediator(Mediator mediator);
    public abstract void setColleagueEnabled(boolean enabled);
}
```

In C++ it becomes this.

```cpp
class PanelWidget;                       // forward declaration

class PanelMediator
{
public:
  virtual ~PanelMediator() = default;
  virtual void widget_changed(PanelWidget * widget) = 0;
};
```

There are 3 changes from the Java version.

### Change 1: removed `createColleagues()`

The Java `Mediator` has `createColleagues()`. In C++ we **do not put it in the interface**.
Creation is the job of the constructor of `ControlPanel`, and it is not something to call from outside.

The Java version puts it in the interface because it wanted to state in the type the flow
of calling `createColleagues()` from the constructor of `LoginFrame`,
but in C++ doing this means **calling a virtual function of an object under construction**.
While the constructor of the base class runs, the derived vtable is not in place yet, so
**the base implementation is called, not the derived one** (Java is the opposite: the derived one is called. The behavior is different here).

**Rule**: Do not call virtual functions from constructors and destructors. This chapter is a typical example.

### Change 2: added "who" to `colleagueChanged()`

The Java version has no argument. `LoginFrame` looks directly at its own fields and decides.
You can do the same in C++, but this exercise uses `widget_changed(PanelWidget * widget)`.
There are two reasons.

- A test can observe "whether it went through the Mediator" and "how many times it did"
- When you want to branch on the sender, you can decide by **name, not by a cast**

You will want to use `dynamic_cast` to find the sender's type, but **on microcontrollers `-fno-rtti` makes it unavailable**.
If you build it to distinguish by name or ID, you can take it over as it is.

### Change 3: made the `Colleague` parameter `PanelMediator *` (not `shared_ptr`)

This is the main topic of the chapter. **You must not make it `std::shared_ptr<PanelMediator>`.**
I measure the reason in 16.3.

## 16.2 Who owns whom

With the Mediator pattern, **it always breaks unless you decide the direction of ownership**. There is only one way to decide.

```
ControlPanel --(owns with std::unique_ptr)--> PanelWidget
PanelWidget  --(only points with a raw pointer)--> PanelMediator
```

**Ownership goes one way. The reverse direction only "points".** That is the answer.

```cpp
class ControlPanel : public PanelMediator
{
private:
  std::unique_ptr<ToggleWidget> emergency_stop_;   // owns
  std::unique_ptr<ToggleWidget> auto_mode_;
  // ...
};

class PanelWidget
{
private:
  PanelMediator * mediator_ = nullptr;             // does not own
};
```

You may be wary when a raw pointer shows up, but here **the raw pointer is the correct statement**.
The way to say "does not own" in the type is a raw pointer (or `std::weak_ptr`).
`unique_ptr` and `shared_ptr` mean ownership, so putting them here would be a lie.

You also need a **lifetime promise**: "the Mediator outlives the Colleagues".
This is kept automatically. The Mediator owns the Colleagues, so
when the Mediator dies, the Colleagues die with it. **The reverse order cannot happen.**

## 16.3 A C++-specific danger: if both sides hold `shared_ptr`, nothing is released

"I can't tell who owns what, so let's use `shared_ptr` for now" is the worst move.
Run it and see for yourself.

```cpp
#include <iostream>
#include <memory>
#include <vector>

struct Colleague;

// --- Bad example: Mediator and Colleague hold each other with shared_ptr -------
struct BadMediator
{
  ~BadMediator() { std::cout << "~BadMediator\n"; }
  std::vector<std::shared_ptr<Colleague>> members;
};

struct Colleague
{
  ~Colleague() { std::cout << "~Colleague\n"; }
  std::shared_ptr<BadMediator> mediator;   // the source of the cycle
};

// --- Good example: ownership goes only one way, Mediator -> Colleague ----------
struct GoodMediator;

struct GoodColleague
{
  ~GoodColleague() { std::cout << "~GoodColleague\n"; }
  GoodMediator * mediator = nullptr;       // only points. does not own
};

struct GoodMediator
{
  ~GoodMediator() { std::cout << "~GoodMediator\n"; }
  std::vector<std::unique_ptr<GoodColleague>> members;
};

int main()
{
  std::cout << "--- bad ---\n";
  {
    auto mediator = std::make_shared<BadMediator>();
    auto colleague = std::make_shared<Colleague>();
    mediator->members.push_back(colleague);
    colleague->mediator = mediator;
    std::cout << "use_count: mediator=" << mediator.use_count()
              << " colleague=" << colleague.use_count() << "\n";
  }
  std::cout << "--- end of bad ---\n";

  std::cout << "--- good ---\n";
  {
    auto mediator = std::make_unique<GoodMediator>();
    mediator->members.push_back(std::make_unique<GoodColleague>());
    mediator->members.back()->mediator = mediator.get();
  }
  std::cout << "--- end of good ---\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: do <code>~BadMediator</code> and <code>~Colleague</code> appear</summary>

**They do not.** The result of running it is this.

```
--- bad ---
use_count: mediator=2 colleague=2
--- end of bad ---
--- good ---
~GoodMediator
~GoodColleague
--- end of good ---
```

In the `bad` block, **not one destructor is called** even after leaving the scope.
The cause is that `use_count` is 2. Even when the local `shared_ptr` variables die,
the count stays at 1 (because they hold each other), so it never reaches 0 and nothing is released.
**This is the leak.**

And there is no error or warning. `-Wall -Wextra -Wpedantic` pass in silence.
It does not crash when you run it either. **The only ways to notice are the destructor log or monitoring memory usage.**

In the `good` block, `unique_ptr` holds in one direction only, so both die properly.
</details>

### The form with `weak_ptr`

If the Mediator itself is managed by `shared_ptr` and a raw pointer cannot guarantee the lifetime,
make the Colleague side a `std::weak_ptr`.

```cpp
class PanelWidget
{
public:
  void set_mediator(std::weak_ptr<PanelMediator> mediator) { mediator_ = std::move(mediator); }

protected:
  void notify_changed()
  {
    // lock() temporarily promotes to a shared_ptr. If it is already dead, nullptr.
    if (const std::shared_ptr<PanelMediator> mediator = mediator_.lock()) {
      mediator->widget_changed(this);
    }
  }

private:
  std::weak_ptr<PanelMediator> mediator_;
};
```

`weak_ptr` **does not increase the count**, so there is no cycle. In addition,
"do nothing if the Mediator has already died" can be written safely.

But there is a cost. `lock()` does an **atomic counter operation** every time
and constructs one `shared_ptr`. If it runs only when a button is pressed, it is within the noise, but
if you hit it on every cycle of a control loop, measure it. **Do not use it on microcontrollers** (16.7).

| Means | Does it cycle | Cost | When to use |
| --- | --- | --- | --- |
| Raw pointer | No | Zero | When **the Mediator owns the Colleagues** (= almost always) |
| `std::weak_ptr` | No | A counter operation on every `lock()` | When the Mediator's lifetime is managed externally and cannot be read |
| `std::shared_ptr` | **Yes** | - | Do not use |

The exercise uses number 1. **If the Mediator owns the Colleagues, a raw pointer is correct.**

## 16.4 Adding a Mediator always leads to two-phase initialization

This is the same in Java, but in C++ it **hurts more**.

The Colleague needs the Mediator, and the Mediator needs the Colleague.
You cannot connect both directions in the constructor's initializer list. **One side must be complete before you can hand it to the other.**

```cpp
ControlPanel::ControlPanel()
: emergency_stop_(std::make_unique<ToggleWidget>("emergency_stop")),
  auto_mode_(std::make_unique<ToggleWidget>("auto_mode"))
{
  // Only here do the Colleagues exist for the first time. From here, connect the reverse direction.
  emergency_stop_->set_mediator(this);
  auto_mode_->set_mediator(this);
  update_enabled_states();
}
```

**There are 2 dangers.**

**Danger 1: a Colleague with no Mediator set can exist.**
From right after `std::make_unique<ToggleWidget>(...)` returns until `set_mediator(this)` is called,
that Colleague is in a state where "it cannot report to anyone". If something calls `notify_changed()` during this time,
`mediator_` is `nullptr`. That is why `notify_changed()` must have a nullptr check.

```cpp
void PanelWidget::notify_changed()
{
  if (mediator_ == nullptr) {
    return;          // not wired yet. Do not crash
  }
  mediator_->widget_changed(this);
}
```

You cannot choose "throw an exception if not wired". On microcontrollers it is `-fno-exceptions`.
**Design "not wired" as a state of "not doing anything yet", not as a "broken state".**

**Danger 2: you are handing out `this` while it is under construction.**
At the time of `set_mediator(this)`, `ControlPanel` is still under construction.
It is safe only because `set_mediator()` **just stores the pointer**.
If the Colleague called back `mediator->widget_changed(...)` here,
it would touch members of `ControlPanel` that are not yet initialized.

**Rule**: The `this` you hand out in the first phase of two-phase initialization must only be **stored**.
For the same reason, the order of calling `update_enabled_states()` after that cannot be swapped.

Also, you **cannot make `notify_changed()` an inline definition in the header**.
This is because `PanelMediator` has only a forward declaration.

```cpp
class PanelMediator;

class PanelWidget
{
public:
  void notify_changed() { mediator_->widget_changed(this); }
  // ...
};
```

```
error: member access into incomplete type 'PanelMediator'
note: forward declaration of 'PanelMediator'
```

For two classes that refer to each other, **you must push the implementation of one of them out to the `.cpp`**.
This is also a structural side effect that comes with the Mediator pattern.

## 16.5 Beware of infinite recursion: separate the "calling entry" from the "called entry"

A Colleague has 2 kinds of methods. **If you mix them, you get infinite recursion immediately.**

| Kind | Example | Does it report to the Mediator |
| --- | --- | --- |
| Entry for user operations (Colleague -> Mediator) | `set_checked()` / `press()` | **Yes** |
| Entry for instructions from the Mediator (Mediator -> Colleague) | `set_enabled()` | **No** |

If `set_enabled()` calls `notify_changed()`,
`widget_changed()` -> `update_enabled_states()` -> `set_enabled()` -> `widget_changed()` ...
and it never comes back. In Yuki's Java version too, `setColleagueEnabled()` does not notify. It is for the same reason.

Another thing you need is **not to report if the value did not change**.

```cpp
void ToggleWidget::set_checked(bool checked)
{
  if (!is_enabled()) { return; }
  if (checked == is_checked_) { return; }   // no change. Do not report
  is_checked_ = checked;
  notify_changed();
}
```

Without this, just calling `set_checked(true)` twice runs the Mediator twice.
If the state is the same it is harmless, but if the Mediator "writes a log on every change" or "sends a command to the motor", it does harm.

## 16.6 Does the standard library or the language have the same thing

**No.** The C++ standard library has nothing that corresponds to Mediator.

Something close is "signals and slots", but it is not standard.

| Mechanism | Where it is | Relation |
| --- | --- | --- |
| Qt signal/slot | Qt (not standard) | Writes the notification wiring declaratively. Closer to Observer than to Mediator |
| `boost::signals2` | Boost (not standard) | Same as above |
| A callback table of `std::function` | Standard | A way to implement the inside of a Mediator with a table of functions. Not the pattern itself |

**Just lining up `std::function`s does not make a Mediator.** The essence of a Mediator is
"the mediation logic is gathered in one place", not the notification wiring.
If all you want is the wiring, that is chapter 17, Observer. **Do not confuse these two.**

## 16.7 A Mediator grows fat: a rule of thumb before adding one

As in the attitude of `00_before_you_use_them.md`: **a Mediator turns into a God object if you leave it alone.**

The reason is structural. Every time a Colleague is added, mediation logic is added, and
it all gathers in the one `widget_changed()`.
If there are 8 Colleagues, there is one function that knows the circumstances of all 8.
**That has not "removed complexity"; it has only "gathered it in one place".**

Gathering is worth it **only if N x N lines were being drawn in the first place**.

| Number of Colleagues | Lines if connected directly | With a Mediator | Decision |
| --- | --- | --- | --- |
| 2 | 1 line | 2 lines + 1 Mediator class | **Do not add. It increases** |
| 3 | 3 lines | 3 lines + 1 Mediator class | **Do not add. Almost the same** |
| 5 | 10 lines | 5 lines + 1 Mediator class | Consider it |
| 8 | 28 lines | 8 lines + 1 Mediator class | Worth adding |

**Rule of thumb: if there are 3 Colleagues or fewer, do not add one.**
If it is only "press a button and a lamp turns on", the button may know the lamp directly.

Ask two more things.

1. **Is the relationship among Colleagues really mutual?** If it is one-way (A changes and it only reaches B),
   Observer is enough. A Mediator is for when "A and B affect each other"
2. **Can you write the mediation rules down in one function?** If you cannot,
   it is not a Mediator but a **state machine**. See chapter 19, State

There is one way out when it starts to grow fat. **Do not branch on the sender; "decide everything again from the current state".**
The exercise's `update_enabled_states()` is that.

```cpp
// Bad: branches per sender. Every new Colleague adds an if
void widget_changed(PanelWidget * widget)
{
  if (widget == auto_mode_.get()) { /* ... */ }
  else if (widget == emergency_stop_.get()) { /* ... */ }
  // with 8 it is 8 branches. And combinations get missed
}

// Good: derive everything from the current state, regardless of who it came from
void widget_changed(PanelWidget *) { update_enabled_states(); }
```

The branches disappear, and **the state cannot get out of sync**.
A bug such as "only when you turn off auto mode during an emergency stop, the manual button becomes enabled again"
appears only when you write it with branches.

## 16.8 Conclusion for microcontrollers

**Do not use the heap.** Use none of `unique_ptr`, `shared_ptr`, `vector`, or `string`;
hold the Mediator and the Colleagues **statically and wire them just once at startup**.

```cpp
#include <cstdint>

class PanelMediator;

// Colleague. Does not use the heap. Only points to the Mediator with a raw pointer.
class Widget
{
public:
  constexpr explicit Widget(std::uint8_t id) : id_(id) {}

  std::uint8_t id() const { return id_; }
  bool is_on() const { return is_on_; }
  bool is_enabled() const { return is_enabled_; }
  void set_enabled(bool enabled) { is_enabled_ = enabled; }
  void set_mediator(PanelMediator * mediator) { mediator_ = mediator; }

  void set_on(bool on);

private:
  std::uint8_t id_;
  PanelMediator * mediator_ = nullptr;
  bool is_on_ = false;
  bool is_enabled_ = true;
};

// Mediator. Limit virtual functions to one (one vtable is enough).
class PanelMediator
{
public:
  virtual ~PanelMediator() = default;
  virtual void widget_changed(Widget * widget) = 0;
};

void Widget::set_on(bool on)
{
  if (!is_enabled_ || on == is_on_) {
    return;
  }
  is_on_ = on;
  if (mediator_ != nullptr) {
    mediator_->widget_changed(this);
  }
}

constexpr std::uint8_t kEmergencyStop = 0;
constexpr std::uint8_t kAutoMode = 1;
constexpr std::uint8_t kManualForward = 2;

// ConcreteMediator. Holds members by value. Uses none of new, vector, or string.
class ControlPanel : public PanelMediator
{
public:
  ControlPanel()
  {
    // Two-phase initialization. Wire just once at startup.
    for (Widget & widget : widgets_) {
      widget.set_mediator(this);
    }
    update_enabled_states();
  }

  Widget & widget(std::uint8_t id) { return widgets_[id]; }

  void widget_changed(Widget *) override { update_enabled_states(); }

private:
  void update_enabled_states()
  {
    const bool is_emergency = widgets_[kEmergencyStop].is_on();
    widgets_[kEmergencyStop].set_enabled(true);
    widgets_[kAutoMode].set_enabled(!is_emergency);
    widgets_[kManualForward].set_enabled(!is_emergency && !widgets_[kAutoMode].is_on());
  }

  // Fixed-length array. The number of elements is decided at compile time.
  Widget widgets_[3] = {Widget{kEmergencyStop}, Widget{kAutoMode}, Widget{kManualForward}};
};

// Allocated statically. Constructed once at startup, and after that never touches the heap.
ControlPanel g_panel;

int main()
{
  g_panel.widget(kAutoMode).set_on(true);
  return g_panel.widget(kManualForward).is_enabled() ? 1 : 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti micro.cpp -o micro
./micro; echo "exit=$?"
```

You get `exit=0`, because turning on auto mode disabled `manual_forward`.
**It compiles with zero warnings under `-fno-exceptions -fno-rtti`.** Here are the changes side by side.

| Host version | Microcontroller version | Reason |
| --- | --- | --- |
| `std::unique_ptr<Widget>` x 4 | `Widget widgets_[3]` | No dynamic allocation. The number of elements is fixed at compile time |
| `std::string name_` | `std::uint8_t id_` | `std::string` allocates |
| `std::vector<std::string> change_log_` | Not held | Use a fixed-length ring buffer for logs, or do not keep a log at all |
| `dynamic_cast` on the sender | Distinguish by ID | `dynamic_cast` is unavailable under `-fno-rtti` |
| Exception when not wired | Do nothing when not wired | `-fno-exceptions` |

**I put `ControlPanel g_panel;` as a global.** The static initialization order problem seen in chapter 5, Singleton,
does not occur, because `ControlPanel` **does not depend on globals of other translation units**.
If you make it depend on them, use a Meyers Singleton (a function-local static).

Let's also look at the **vtable cost**. The virtual functions are only `PanelMediator::widget_changed()` and
the virtual destructor, 2 in all. `Widget` has no virtual functions at all.
If you make the Colleague virtual, **a vtable pointer per Colleague goes into RAM**.
Unless you really need kinds, hold Colleagues as concrete types.

## 16.9 Conclusion for ROS 2 (supplement)

In ROS 2 `shared_ptr` appears everywhere, so **circular references become real accidents**.

If a `rclcpp::Node` captures `shared_from_this()` instead of `this` in a callback lambda and
stores it in its own member, **the node is never released**.
The standard remedy is to use `std::weak_ptr` and call `lock()` at the top of the callback.

```cpp
std::weak_ptr<MyNode> weak_self = shared_from_this();
timer_ = create_wall_timer(100ms, [weak_self]() {
  if (const auto self = weak_self.lock()) {
    self->on_timer();
  }
});
```

But **you do not normally write the mediation between nodes as a Mediator class**.
In ROS 2, topics, services, and parameters are already "a mechanism that keeps them from connecting directly".
You write a Mediator only **inside one node**, or **inside a library**.

## 16.10 Common pitfalls

| Symptom | Cause |
| --- | --- |
| Not one destructor is called. It does not crash either | Mutual `shared_ptr`. The circular reference of 16.3 |
| It crashes with a stack overflow | `set_enabled()` calls `notify_changed()`. 16.5 |
| `error: member access into incomplete type 'PanelMediator'` | You use `mediator_->` in a header that has only a forward declaration. Move the implementation to the `.cpp` |
| It crashes right after `set_mediator(this)` in the constructor | The place you handed `this` to in the first phase calls back immediately. Danger 2 of 16.4 |
| The initial state of a Colleague disagrees with the mediation rules | You forgot to call `update_enabled_states()` at the end of the constructor |
| Adding one Colleague broke other combinations | You branch on the sender. Switch to the form that "decides everything again from the current state". 16.7 |
| nullptr reference at `mediator_->` | In the middle of two-phase initialization. You missed the nullptr check in `notify_changed()` |
| The derived `createColleagues()` is not called | You call a virtual function from the base constructor. The behavior differs from Java. 16.1 |

## 16.11 Matching exercise

```bash
./drill run dp16
```

In `exercises/dp16_mediator/src/control_panel.cpp`, you implement the robot's control panel.

1. `PanelWidget::set_mediator()`: only store it with a raw pointer. **Do not own it**
2. `PanelWidget::notify_changed()`: nullptr check, then `widget_changed(this)`
3. `ToggleWidget::set_checked()` / `ButtonWidget::press()`: do nothing if disabled. Report only when it changed
4. The constructor of `ControlPanel`: wire the 4 Colleagues with two-phase initialization
5. `ControlPanel::widget_changed()` / `update_enabled_states()`: the mediation rules

In addition to the mediation rules, the tests check that **removing the Mediator stops the effect from reaching other Colleagues**
(= they are not connected directly), that **`use_count()` stays 1**
(= they do not own it), and that
**destroying the `ControlPanel` calls the destructors of all 4 Colleagues**.

## 16.12 Summary of this chapter

- A Mediator and its Colleagues **point at each other**. If you make both `shared_ptr` in C++, **a circular reference leaks**
- Even when it leaks, **there is no warning and no exception**. The only way to notice is the destructor log
- Ownership goes **one way, Mediator -> Colleague**. The reverse is a raw pointer, and `weak_ptr` only when the lifetime cannot be read
- Here **the raw pointer is the correct statement of "does not own"**. Making it a `shared_ptr` would be a lie
- A Mediator always leads to **two-phase initialization**. Design "a Colleague with no Mediator set" as a state that does not crash
- For classes that refer to each other, you have no choice but to **push the implementation of one into the `.cpp`** (incomplete type)
- Do not notify from `set_enabled()`. **Separate the calling entry from the called entry**
- The standard library has **no** Mediator. Signals and slots are Qt / Boost, not standard
- **If there are 3 Colleagues or fewer, do not add one.** Gathering is worth it only once the lines become N x N
- Do not branch on the sender; "decide everything again from the current state", and both the bloat and the bugs decrease
- On microcontrollers, **hold things statically and wire just once at startup**. It compiles with `-fno-exceptions -fno-rtti`

---

Previous: [15. Facade](15_Facade.md) / Next: 17. Observer (coming soon)
