# 14. Chain of Responsibility

> **Matches chapter 14 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep `Support` / `NoSupport` / `LimitSupport` / `Trouble` open next to you.
>
> **Goal of this chapter**: The structure is almost the same as the Java version. Call `resolve()`, and
> if it fails, pass the request to `next`. That is all.
> **The whole problem comes down to one point: what you use to hold `next`.**
> In Java, `private Support next;` is taken care of by the GC,
> but in C++, if you write `Support * next_;`, **the whole chain becomes a minefield the moment `next` dies first**.
> I line up three choices (raw pointer, `unique_ptr`, "do not build a chain") and
> decide which one to use in practice.

## 14.1 Porting the Java version to C++ as it is

Yuki's book defines `Support` like this.

```java
public abstract class Support {
    private String name;
    private Support next;

    public Support setNext(Support next) {
        this.next = next;
        return next;
    }

    public final void support(Trouble trouble) {
        if (resolve(trouble)) {
            done(trouble);
        } else if (next != null) {
            next.support(trouble);
        } else {
            fail(trouble);
        }
    }

    protected abstract boolean resolve(Trouble trouble);
}
```

In C++ it becomes this.

```cpp
class FaultHandler
{
public:
  explicit FaultHandler(std::string name);
  virtual ~FaultHandler();

  FaultHandler(const FaultHandler &) = delete;
  FaultHandler & operator=(const FaultHandler &) = delete;

  FaultHandler & set_next(std::unique_ptr<FaultHandler> next);
  std::optional<FaultAction> support(const Fault & fault) const;

protected:
  virtual std::optional<FaultAction> resolve(const Fault & fault) const = 0;

private:
  std::string name_;
  std::unique_ptr<FaultHandler> next_;
};
```

There are four changes.

### Change 1: added `virtual ~FaultHandler();`

The usual story. Because we hold it with `std::unique_ptr<FaultHandler>`,
without a virtual destructor the derived destructors are not called.
In this chapter, **the whole chain is destroyed through the same path**, so if even one is missing,
memory leaks from the middle of the chain.

### Change 2: replaced `Support next` with `std::unique_ptr<FaultHandler> next_`

**This is the main topic of the chapter.** In 14.3 I line up all three choices.

### Change 3: forbade copying

Java's `Support` has no concept of copying. In C++, if you do not write anything,
a copy constructor **appears on its own**. If you cannot answer what should happen when a chain node is copied,
forbidding it is the right answer.

Note that because `next_` is a `unique_ptr`, the copy constructor is already implicitly deleted,
but **write `= delete` explicitly.** "Forbidden by chance because of a member" and
"forbidden by design" are different things, and the difference shows up when you replace the member later.

### Change 4: changed the return of `support()` from `void` to `std::optional<FaultAction>`

The Java version calls `done()` / `fail()` and ends.
I explain in 14.4 why the C++ version returns "who handled it" as a return value.

### What does not change: NVI

`support()` is `public` non-virtual, and `resolve()` is `protected` pure virtual.
This has **exactly the same intent** as the Java version's `public final void support` / `protected abstract boolean resolve`
(the shape of chapter 3, Template Method).

The key is **not to make derived classes write** the "pass it along" logic.
If you open that to derived classes, there will always be one handler that forgets to pass to `next`.

## 14.2 Who owns the chain

The Java version builds it like this.

```java
Support alice = new NoSupport("Alice");
Support bob = new LimitSupport("Bob", 100);
alice.setNext(bob).setNext(charlie);
```

`alice`, `bob`, and `charlie` are all kept alive by the GC as long as a reference is alive.
**Nowhere is it written who owns whom.** Java works anyway.

In C++ you write it down. Because `set_next()` takes a `unique_ptr`,

> **Each handler owns "its next". The head holds the lifetime of the whole chain.**

is now written in the type. The caller looks like this.

```cpp
auto head = make_low_voltage_handler("low_voltage", 11000);
head->set_next(make_over_current_handler("over_current", 20000))
  .set_next(make_comm_timeout_handler("comm_timeout", 500));
// when you drop head, all 3 links of the chain disappear
```

### Why `set_next()` returns `*next_`

The Java `setNext` does `return next;`. Do the same in C++.

```cpp
FaultHandler & FaultHandler::set_next(std::unique_ptr<FaultHandler> next)
{
  next_ = std::move(next);
  return *next_;      // not *this
}
```

After `std::move`, `next` is empty, so **you must not return `*next`**.
Return `*next_`, **after** you put it into `next_`.

If you return `*this`, the meaning of the method chain changes.

```cpp
head->set_next(std::move(b)).set_next(std::move(c));
// return *next_ -> head -> b -> c (correct)
// return *this  -> head -> c (b is thrown away)
```

In C++, the `*this` version is not just "a different order".
At the second `set_next`, `head`'s `next_` is overwritten, and **b is released right there**.
The exercise test "replacing the middle of a chain destroys the old rest" checks this.

## 14.3 A C++-specific danger: the lifetime of the chain

There are three choices for how to hold `next`. **I rule them out one by one.**

### (a) Raw pointer + the convention "the caller guarantees the lifetime"

This is the closest to the Java version.

```cpp
class Handler
{
public:
  void set_next(Handler * next) { next_ = next; }
  // ...
private:
  Handler * next_ = nullptr;
};
```

If you write this, the following code **can be written**.

```cpp
int main()
{
  Handler head{"head"};
  {
    Handler tail{"tail"};      // dies when it leaves the block
    head.set_next(&tail);
  }
  std::cout << head.support() << "\n";   // head still points to the dead tail
  return 0;
}
```

**It compiles, and there is no warning.** When you run it, it touches a dead object.
If you are lucky it crashes; if you are unlucky it only "sometimes returns a strange value".
You can get it reported by running with `-fsanitize=address`,
but the fact that **you cannot notice until you apply a sanitizer** makes this a weak design.

In Java, the reference to `tail` remains inside `head`, so the GC does not collect it and nothing crashes.
**This difference is the extra work that exists only on the C++ side in this chapter.**

You may choose the raw-pointer version only when **all handlers are in static storage**
(the microcontroller version in 14.7 is exactly that). A convention such as "the caller guarantees the lifetime"
written only in a comment will not be kept in team development.

### (b) Own the next with `unique_ptr`

This is what you implement in the exercise.

```cpp
FaultHandler & set_next(std::unique_ptr<FaultHandler> next);
private:
  std::unique_ptr<FaultHandler> next_;
```

- As long as the head is alive, the whole chain is alive
- If you drop the head, the whole chain disappears
- "The next dies first" **cannot structurally happen**

There are two costs.

1. **Heap allocations run once per handler** (this matters on a microcontroller)
2. **You cannot share a handler elsewhere.** You cannot put the same handler into two chains

The second looks like a restriction, but it actually makes the specification clear. If you want to share,
first consider **making the handler stateless and creating two of them**, instead of using
`shared_ptr`.

### (c) Do not build a chain: loop over a `vector<unique_ptr<Handler>>`

```cpp
std::vector<std::unique_ptr<FaultHandler>> handlers;
handlers.push_back(make_low_voltage_handler("low_voltage", 11000));
handlers.push_back(make_over_current_handler("over_current", 20000));

for (const auto & handler : handlers) {
  if (auto action = handler->support_alone(fault)) {
    return action;
  }
}
return std::nullopt;
```

**In practice this is the most straightforward.** There are four reasons.

| Point | (b) Chain | (c) Array |
| --- | --- | --- |
| Seeing the order | Follow the source and trace `set_next` | The order of `push_back` is the order |
| Changing the order | Relink. The source becomes empty after the move, so it is easy to get the steps wrong | Just swap elements |
| Lifetime | The head holds everything (implicit) | The vector holds everything (explicit) |
| Cycles | Some shapes can be built (14.5) | **Structurally impossible** |
| Deep chains | Recursion. Uses stack in proportion to the depth | Loop. Constant |

It is also big that the `next_` member disappears from `Handler`.
**A handler only needs to know "whether it can handle this",
and does not need to know "who is next".** (c) even does that separation of responsibility for you.

Does that mean the GoF version (the chain) has no meaning? No.
When **"the next" changes dynamically per handler**, for example
when you want a branch like "only for a voltage fault, send it to another route",
you need the shape of a chain. If there is no branching, use (c).

**The exercise implements both (b) and (c).** If you write them side by side, you feel the simplicity of (c) with your own hands.

## 14.4 When nobody handled it: `bool` or `optional`, and why we do not throw an exception

The Java version calls `fail()` and ends, and `support()` is `void`.
In C++ there are three choices.

| Return type | What you learn | What is a problem |
| --- | --- | --- |
| `void` + `fail()` inside | Nothing | The caller cannot tell whether it was handled |
| `bool` | Whether it was handled | You cannot tell **who** handled it. You cannot write it to a log |
| `std::optional<FaultAction>` | Whether it was handled + who did what | The type is a bit heavy |

This exercise uses `std::optional<FaultAction>`.
In a chain for fault handling, "which handler did what" is the main content of the log,
so with `bool` you would end up fetching that information through another path.

### Do not throw an exception

You probably thought, "Nobody being able to handle it is abnormal, so shouldn't we `throw`?" **We do not throw.**

1. **On microcontrollers `-fno-exceptions` is normal.** Code that contains `throw` cannot even be built
2. "Nobody handled it" is **expected**. Exceptions are for the unexpected
3. If you throw from the middle of a chain, the caller cannot tell how far the processing went

Instead we return `std::nullopt` and **let the caller decide**.

```cpp
if (const auto action = chain->support(fault)) {
  logger.info(action->handler_name + ": " + action->action);
} else {
  logger.warn("unhandled fault");     // only here do we decide "what to do"
}
```

The claim of this form is that "how to treat an unhandled case" should be decided **outside the chain**, not inside it.
Yuki's book puts `fail()` inside the chain, but in C++ it is more natural to put it outside.

Note that `std::optional` itself works under `-fno-exceptions`,
but `value()` `throw`s on failure. Use `*action` or `has_value()`.
The conclusion for microcontrollers comes separately in 14.7.

## 14.5 The danger of an infinite loop: the chain becomes a ring

If the chain becomes a ring, `support()` loops forever. This is the same in Java, but
in C++ the symptom of a stack overflow tends to be just "it restarts for some reason", so
you reach the cause late. On a microcontroller, you get a watchdog reset right away.

With the raw-pointer version ((a)), a ring is easy to make.

```cpp
a.set_next(&b);
b.set_next(&c);
c.set_next(&a);      // it became a ring
```

**Check just once, right after assembly.** Floyd's cycle detection is enough.

```cpp
bool has_cycle(const Node * head)
{
  const Node * slow = head;
  const Node * fast = head;
  while (fast != nullptr && fast->next() != nullptr) {
    slow = slow->next();
    fast = fast->next()->next();
    if (slow == fast) {
      return true;
    }
  }
  return false;
}
```

Running it gives this (`straight` is a straight line, and `cyclic` is after adding `c -> a`).

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// cycle.cpp
#include <cstdio>

class Node
{
public:
  void set_next(Node * next) { next_ = next; }
  const Node * next() const { return next_; }

private:
  Node * next_ = nullptr;
};

bool has_cycle(const Node * head)
{
  const Node * slow = head;
  const Node * fast = head;
  while (fast != nullptr && fast->next() != nullptr) {
    slow = slow->next();
    fast = fast->next()->next();
    if (slow == fast) {
      return true;
    }
  }
  return false;
}

int main()
{
  Node a;
  Node b;
  Node c;
  a.set_next(&b);
  b.set_next(&c);
  std::printf("straight: %d\n", has_cycle(&a) ? 1 : 0);

  c.set_next(&a);      // it became a ring
  std::printf("cyclic:   %d\n", has_cycle(&a) ? 1 : 0);
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic cycle.cpp -o cycle && ./cycle
```

</details>

<!-- measure: files=cycle.cpp -->
```
straight: 0
cyclic:   1
```

The extra memory is two pointers. Assembly happens once at startup, so the cost is negligible.

### What about the `unique_ptr` version

**You cannot make a ring out of separate nodes.** Each node has only one owner, and
to put `a` into `c`'s `next_`, you have to pass `a`'s `unique_ptr`,
and that `a` is then no longer the head.
If ownership is unique, the shape can only be a tree.

However, **self-ownership** can be written.

```cpp
auto head = std::make_unique<Handler>("head");
head->set_next(std::move(head));    // make it own itself
std::cout << "head is " << (head ? "alive" : "null") << "\n";
std::cout << "--- leaving main ---\n";
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// self_owned.cpp
#include <iostream>
#include <memory>
#include <string>
#include <utility>

class Handler
{
public:
  explicit Handler(std::string name) : name_(std::move(name)) {}
  virtual ~Handler() { std::cout << "dtor " << name_ << "\n"; }

  Handler & set_next(std::unique_ptr<Handler> next)
  {
    next_ = std::move(next);
    return *next_;
  }

private:
  std::string name_;
  std::unique_ptr<Handler> next_;
};

int main()
{
  auto head = std::make_unique<Handler>("head");
  head->set_next(std::move(head));    // make it own itself
  std::cout << "head is " << (head ? "alive" : "null") << "\n";
  std::cout << "--- leaving main ---\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic self_owned.cpp -o self_owned && ./self_owned
```

</details>

<!-- measure: files=self_owned.cpp -->
```
head is null
--- leaving main ---
```

**`dtor head` does not appear.** `head` is owned by its own `next_`,
so nobody releases it. It is not an infinite loop but a **silent leak**.
If you call `support()`, that one recurses infinitely.

You can stop it by putting

```cpp
if (next.get() == this) {
  return *this;       // or assert
}
```

at the top of `set_next`. **If you check, check at assembly time.** Do not check on every call at runtime.

## 14.6 Does the standard library or the language have the same thing

The answer is **no as a class**. The standard library has no
container or utility that corresponds to the GoF Chain of Responsibility.

However, **there is one language feature: the exception `catch`.**

```cpp
try {
  do_something();
} catch (const LowVoltage & e) {      // can I handle it?
} catch (const OverCurrent & e) {     // if not, go to the next
} catch (...) {                       // the last resort
}
```

It tries the `catch` clauses from the top, the one whose type matches handles it, and if none matches it passes on to the next.
Furthermore, if no `catch` accepts it, **it propagates to the outer `try`**.
This is Chain of Responsibility itself.
And the language builds the chain for you, so there is no lifetime problem.

So the decision is as follows.

- In **an environment where exceptions can be used (ROS 2 / PC), and the goal is to propagate a fault**, first think about whether a chain of `catch` is enough
- If **exceptions cannot be used, or you want to treat "handled / not handled" as a value**, write this pattern yourself

Microcontrollers are the latter. That is why this chapter matters for your project.

`std::variant` + `std::visit` is "branching by type", but **it does not pass requests along**
(overload resolution picks exactly one). The use is different.

## 14.7 Try it yourself

Before you solve the exercise, compile this one file and **predict the output** before you run it.
In particular, guess the **order** of the `dtor` lines that appear at the final `head.reset()`.

```cpp
// try.cpp
#include <iostream>
#include <memory>
#include <optional>
#include <string>

struct Fault
{
  int code;
};

class Handler
{
public:
  explicit Handler(std::string name, int mine)
  : name_(std::move(name)), mine_(mine)
  {
    std::cout << "ctor " << name_ << "\n";
  }

  virtual ~Handler() { std::cout << "dtor " << name_ << "\n"; }

  Handler & set_next(std::unique_ptr<Handler> next)
  {
    next_ = std::move(next);
    return *next_;
  }

  std::optional<std::string> support(const Fault & fault) const
  {
    if (fault.code == mine_) {
      return name_;
    }
    if (next_ != nullptr) {
      return next_->support(fault);
    }
    return std::nullopt;
  }

private:
  std::string name_;
  int mine_;
  std::unique_ptr<Handler> next_;
};

int main()
{
  auto head = std::make_unique<Handler>("low_voltage", 1);
  head->set_next(std::make_unique<Handler>("over_current", 2))
    .set_next(std::make_unique<Handler>("comm_timeout", 3));

  const auto hit = head->support(Fault{3});
  std::cout << "hit=" << (hit ? *hit : std::string{"(none)"}) << "\n";

  const auto miss = head->support(Fault{9});
  std::cout << "miss=" << (miss ? *miss : std::string{"(none)"}) << "\n";

  std::cout << "--- dropping head ---\n";
  head.reset();
  std::cout << "--- after dropping ---\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: does <code>dtor</code> run from the head or from the tail</summary>

<!-- measure: files=try.cpp -->
```
ctor low_voltage
ctor over_current
ctor comm_timeout
hit=comm_timeout
miss=(none)
--- dropping head ---
dtor low_voltage
dtor over_current
dtor comm_timeout
--- after dropping ---
```

**From the head.** After the body of `~Handler()` (the `dtor` output) runs,
the member `next_` is destroyed. Members are destroyed **after** the destructor body, so the order is
`low_voltage` -> `over_current` -> `comm_timeout`.

If you predicted "from the tail", that is probably your feeling from writing list release with recursion.
Remember the order of the destructor body and member destruction.

One more thing. If you change `return *next_;` in `set_next` to `return *this;`, you get this.

<!-- measure: files=try.cpp cmd="sed 's/return \*next_;/return *this;/' try.cpp > try_this.cpp && g++ -std=c++17 -Wall -Wextra -Wpedantic try_this.cpp -o try_this && ./try_this" -->
```
ctor low_voltage
ctor over_current
ctor comm_timeout
dtor over_current
hit=comm_timeout
miss=(none)
--- dropping head ---
dtor low_voltage
dtor comm_timeout
--- after dropping ---
```

**`dtor over_current` appears in the middle of assembly.** At the second `set_next`,
`head`'s `next_` is overwritten with `comm_timeout`, and `over_current` is released there.
You meant a 3-link chain, but it is 2 links. And `hit=comm_timeout` does not change, so
**depending on how you write the test, you cannot notice**.
What the method chain returns directly affects lifetime.

Also, this chain **recurses as deep as its length**. With about 5 handlers it is fine,
but if it will be hundreds of links, use the loop version, (c) in 14.3.
</details>

## 14.8 Conclusion for microcontrollers

**Do not use a chain of `unique_ptr`.** There are two reasons.

1. Heap allocations run once per handler. Even if it is once at startup, there is no point in adding a seed of fragmentation
2. `std::string` / `std::optional` / `std::function` come along as a chain of dependencies

We use the **static version of (c)** in 14.3.
**Put handlers in static storage, give them no `next`, and express the order by the order in a fixed-length array.**
Allocation is zero, and the very problem of chain lifetime disappears.

```cpp
#include <cstddef>
#include <cstdio>

enum class FaultKind
{
  kLowVoltage,
  kOverCurrent,
  kCommTimeout,
};

struct Fault
{
  FaultKind kind;
  int magnitude;
};

/// Do not use std::optional (value() can throw).
/// When handled == false, the promise is that handler_name / action are not read.
struct FaultAction
{
  bool handled = false;
  const char * handler_name = nullptr;
  const char * action = nullptr;
};

/// It has no next. The shape of the chain is held by the array side.
class FaultHandler
{
public:
  virtual ~FaultHandler() = default;
  virtual FaultAction resolve(const Fault & fault) const = 0;
};

class LowVoltageHandler : public FaultHandler
{
public:
  constexpr explicit LowVoltageHandler(int threshold_mv)
  : threshold_mv_(threshold_mv)
  {
  }

  FaultAction resolve(const Fault & fault) const override
  {
    if (fault.kind == FaultKind::kLowVoltage && fault.magnitude < threshold_mv_) {
      return FaultAction{true, "low_voltage", "reduce_duty"};
    }
    return FaultAction{};
  }

private:
  int threshold_mv_;
};

class OverCurrentHandler : public FaultHandler
{
public:
  constexpr explicit OverCurrentHandler(int limit_ma)
  : limit_ma_(limit_ma)
  {
  }

  FaultAction resolve(const Fault & fault) const override
  {
    if (fault.kind == FaultKind::kOverCurrent && fault.magnitude >= limit_ma_) {
      return FaultAction{true, "over_current", "cut_output"};
    }
    return FaultAction{};
  }

private:
  int limit_ma_;
};

// Static storage. Uses neither the heap nor new.
LowVoltageHandler low_voltage{11000};
OverCurrentHandler over_current{20000};

// The "order" of the chain is the order of this array itself.
FaultHandler * const kChain[] = {&low_voltage, &over_current};
constexpr std::size_t kChainSize = sizeof(kChain) / sizeof(kChain[0]);

FaultAction dispatch(const Fault & fault)
{
  for (std::size_t i = 0; i < kChainSize; ++i) {
    const FaultAction action = kChain[i]->resolve(fault);
    if (action.handled) {
      return action;
    }
  }
  return FaultAction{};   // nobody handled it
}
```

This is the result of passing 3 faults from `main`.

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti mcu.cpp -o mcu && ./mcu
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// mcu.cpp
#include <cstddef>
#include <cstdio>

enum class FaultKind
{
  kLowVoltage,
  kOverCurrent,
  kCommTimeout,
};

struct Fault
{
  FaultKind kind;
  int magnitude;
};

/// Do not use std::optional (value() can throw).
/// When handled == false, the promise is that handler_name / action are not read.
struct FaultAction
{
  bool handled = false;
  const char * handler_name = nullptr;
  const char * action = nullptr;
};

/// It has no next. The shape of the chain is held by the array side.
class FaultHandler
{
public:
  virtual ~FaultHandler() = default;
  virtual FaultAction resolve(const Fault & fault) const = 0;
};

class LowVoltageHandler : public FaultHandler
{
public:
  constexpr explicit LowVoltageHandler(int threshold_mv)
  : threshold_mv_(threshold_mv)
  {
  }

  FaultAction resolve(const Fault & fault) const override
  {
    if (fault.kind == FaultKind::kLowVoltage && fault.magnitude < threshold_mv_) {
      return FaultAction{true, "low_voltage", "reduce_duty"};
    }
    return FaultAction{};
  }

private:
  int threshold_mv_;
};

class OverCurrentHandler : public FaultHandler
{
public:
  constexpr explicit OverCurrentHandler(int limit_ma)
  : limit_ma_(limit_ma)
  {
  }

  FaultAction resolve(const Fault & fault) const override
  {
    if (fault.kind == FaultKind::kOverCurrent && fault.magnitude >= limit_ma_) {
      return FaultAction{true, "over_current", "cut_output"};
    }
    return FaultAction{};
  }

private:
  int limit_ma_;
};

// Static storage. Uses neither the heap nor new.
LowVoltageHandler low_voltage{11000};
OverCurrentHandler over_current{20000};

// The "order" of the chain is the order of this array itself.
FaultHandler * const kChain[] = {&low_voltage, &over_current};
constexpr std::size_t kChainSize = sizeof(kChain) / sizeof(kChain[0]);

FaultAction dispatch(const Fault & fault)
{
  for (std::size_t i = 0; i < kChainSize; ++i) {
    const FaultAction action = kChain[i]->resolve(fault);
    if (action.handled) {
      return action;
    }
  }
  return FaultAction{};   // nobody handled it
}

int main()
{
  const Fault faults[] = {
    Fault{FaultKind::kLowVoltage, 10500},
    Fault{FaultKind::kOverCurrent, 25000},
    Fault{FaultKind::kCommTimeout, 0},
  };

  for (const Fault & fault : faults) {
    const FaultAction action = dispatch(fault);
    if (action.handled) {
      std::printf("%s -> %s\n", action.handler_name, action.action);
    } else {
      std::printf("unhandled\n");
    }
  }
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti mcu.cpp -o mcu && ./mcu
```

</details>

<!-- measure: files=mcu.cpp -->
```
low_voltage -> reduce_duty
over_current -> cut_output
unhandled
```

Four points.

- **We do not use `std::optional`; we use a POD that has a `bool`.** This removes, along with the type,
  the accident of calling `value()` by mistake under `-fno-exceptions`
- **We use `const char *`, not `std::string`.** Names are literals, so no allocation is needed
- **Handlers are variables at namespace scope.** There is no dynamic allocation and no relinking of `next`.
  But watch out for the **static initialization order** (the topic of chapter 5, Singleton. Do not make it
  depend on other static objects)
- **`kChain` is `FaultHandler * const []`.** The order is decided at compile time and
  cannot break at runtime

Virtual functions need one vtable per handler type, but
there is only one `resolve` each, so it is on the order of tens of bytes. This is a cost you may pay.
If you really do not want to pay it, and the handlers are fixed at compile time,
drop inheritance and use an array of function pointers.

```cpp
using ResolveFn = FaultAction (*)(const Fault &);
constexpr ResolveFn kChain[] = {&resolve_low_voltage, &resolve_over_current};
```

The vtable becomes zero, but handlers can no longer hold state (the threshold), so
you either make the threshold a global constant or pass it as an argument. **Write the virtual-function version first.**

## 14.9 Conclusion for ROS 2 (supplement)

No GoF Chain of Responsibility class appears in rclcpp.
Similar structures appear in two places.

- **Parameter callbacks**: the validation functions registered with `add_on_set_parameters_callback()`
  are called in order, and if even one rejects, the whole thing is rejected. It has the shape of "try in order", but
  the stop condition is the opposite of CoR, which stops at the first success (here everything must pass)
- **Lifecycle node transition callbacks**: the responsibility is split per state

If you write your own, on the ROS 2 side you can use exceptions and `std::function`, so
`std::vector<std::function<std::optional<FaultAction>(const Fault &)>>` is enough.
**You almost never need to build a class hierarchy.**

```cpp
std::vector<std::function<std::optional<FaultAction>(const Fault &)>> handlers;
handlers.push_back([](const Fault & f) -> std::optional<FaultAction> {
  if (f.kind == FaultKind::kLowVoltage && f.magnitude < 11000) {
    return FaultAction{"low_voltage", "reduce_duty"};
  }
  return std::nullopt;
});
```

If handlers have no state and each is a few lines, this is the shortest.
Bring out inheritance only when a handler **has state** or **you want to test it alone**.

## 14.10 Common pitfalls

| Symptom | Cause |
| --- | --- |
| With `a.set_next(b).set_next(c)`, b has disappeared | `set_next` returns `*this` instead of `*next_` |
| It crashes when you try to return `*next` inside `set_next` | After `std::move(next)`, `next` is empty. Return `*next_` |
| The program freezes right after building the chain | It is a ring. Add the check from 14.5 at assembly time |
| Even if you drop the head, the 2nd and later links are not released | `next_` is a raw pointer. Make it a `unique_ptr` |
| The destructor of the 2nd link is not called | The base has no virtual destructor |
| It does not compile when I try to put a handler into 2 chains | A `unique_ptr` cannot be shared. Create 2 handlers |
| I want to call `next_->support()` inside the derived `resolve()` | NVI is broken. Passing along is the job of `support()` |
| `optional::value()` cannot be linked with `-fno-exceptions` | Use `*opt` or `has_value()` |
| The stack ran out after adding handlers | The chain recurses. If there are many links, switch to the loop of (c) |

## 14.11 Matching exercise

```bash
./drill run dp14
```

In `exercises/dp14_chain_of_responsibility/src/fault_chain.cpp`, you implement
the handling system for robot fault detection (low voltage, over-current, communication loss).

1. **`FaultHandler::~FaultHandler()`**: leave its own name in the destruction log
2. **`FaultHandler::set_next()`**: own the next with a `unique_ptr` and return **a reference to the next**
3. **`FaultHandler::support()`**: itself -> next -> ... If nobody handles it, `std::nullopt`
4. **`FaultHandler::support_alone()`**: do not pass to the next; ask only itself
5. **`dispatch()`**: the array approach of 14.3 (c)
6. **`resolve()` of the 3 concrete handlers**

The tests check

- the appropriate handler handles it, and the others pass it through
- **changing the order of the chain changes which handler handles it**
- the case where nobody handles it is `std::nullopt`
- **destroying the head destroys the whole chain** (checked by the order of the destructor log)
- `set_next()` returns a reference to the next handler, not `*this` (address comparison)
- the chain version and the array version give the same answer

## 14.12 Summary of this chapter

- The structure is almost the same as the Java version. **The only thing that changes is what you use to hold `next`**
- The raw-pointer version cannot stop "the next dies first". This is **work that Java does not have**
- If you own the next with a `unique_ptr`, **the lifetime of the head is the lifetime of the whole chain**
- `set_next()` returns **`*next_`**, not `*this`. If you get it wrong, the 2nd link is silently released
- If nobody handles it, return **`std::nullopt`**. Do not throw an exception (because of `-fno-exceptions`)
- If you do not need branching, **not building a chain and looping over a `vector`, (c), is the most straightforward in practice**
- A ring never stops. **Check once at assembly time.** With `unique_ptr`, you cannot make a ring out of different nodes
- The standard library has no class, but **a chain of exception `catch` is the language-feature version**
- On microcontrollers, use static handlers + a fixed-length array. **Zero allocation, zero lifetime problems**

---

Previous: [13. Visitor](13_Visitor.md) / Next: [15. Facade](15_Facade.md)
