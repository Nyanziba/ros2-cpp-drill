# 13. Visitor

> **Matches chapter 13 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep `Visitor` / `Element` / `File` / `Directory` / `ListVisitor` open next to you.
>
> **Goal of this chapter**: The roundabout structure where `accept()` calls `visitor.visit(this)`.
> I explain, from the C++ overload resolution rules, **why one call is not enough**.
> On top of that, C++17 has **another solution that needs no inheritance, no virtual functions, and no `accept`**:
> `std::variant` + `std::visit`. You implement both the GoF version and the variant version, and
> check that **the same tree gives the same result**.
> Of the 23 chapters, this is the one where **the Java version and the C++ version look the most different**.

## 13.1 Porting the Java version to C++ as it is

Yuki's book defines `Visitor` and `Element` like this.

```java
public abstract class Visitor {
    public abstract void visit(File file);
    public abstract void visit(Directory directory);
}

public interface Element {
    public abstract void accept(Visitor v);
}
```

In C++ it becomes this.

```cpp
class SensorCheck;
class MotorCheck;
class CheckGroup;

class DiagVisitor
{
public:
  virtual ~DiagVisitor() = default;                    // Change 1
  virtual void visit(const SensorCheck & node) = 0;    // Change 2
  virtual void visit(const MotorCheck & node) = 0;
  virtual void visit(const CheckGroup & node) = 0;
};

class DiagNode
{
public:
  virtual ~DiagNode() = default;
  virtual void accept(DiagVisitor & visitor) const = 0;  // Change 3
};
```

### Change 1: virtual destructor

Same as chapter 1. If you write even one `virtual`, write a virtual destructor too.
**You need it in both `DiagVisitor` and `DiagNode`**, because visitors are also passed around through base pointers.

### Change 2: the parameter is `const Derived &`, not a value and not the base type

Java's `visit(File file)` passes a reference. If you write this in C++

```cpp
virtual void visit(SensorCheck node) = 0;    // pass by value. copies every time
```

**the whole element is copied on every visit.**
Worse, if the element is a `CheckGroup` (which holds a `vector` of `unique_ptr`),
it has no copy constructor, so the code does not even compile.

Taking the base type is more dangerous still.

```cpp
virtual void visit(DiagNode node) = 0;       // slicing
```

The derived part is cut off. **Slicing does not exist in Java at all**, so
you cannot notice it if you only copy the book.

`const` is there because a visitor normally only reads the element.
Remove `const` only when you build a visitor that modifies elements (then the `const` on `accept` goes away too).

### Change 3: make `accept` a `const` member function

If the visit does not change the element, `accept` is `const` too.
Java has no such distinction, so **if you forget it, you cannot visit a `const DiagNode &`.**

## 13.2 Why `accept` is needed: overloads are chosen by the static type

This is the main topic of the chapter. You probably thought, "If `visit` is overloaded,
can't I just hand the element to the visitor?" Let's try it.

```cpp
struct Node { virtual ~Node() = default; };
struct Sensor : Node {};
struct Motor : Node {};

void describe(const Sensor &) { std::cout << "sensor\n"; }
void describe(const Motor &) { std::cout << "motor\n"; }
void describe(const Node &) { std::cout << "node (kind lost)\n"; }

const Node * const nodes[] = {&sensor, &motor};
for (const Node * const node : nodes) {
  describe(*node);
}
```

Running it gives this (this is the first half of `try.cpp` in 13.8).

```
node (kind lost)
node (kind lost)
```

**The static type of `*node` is `const Node &`, so only `describe(const Node &)` is chosen.**
It does not matter whether the object is a `Sensor` or a `Motor`.
C++ overload resolution is decided **at compile time, from the static type of the expression only**.
To look at the runtime type, C++ has **only virtual functions**.

So it takes two steps.

| | What it does | What decides it |
| --- | --- | --- |
| 1st call `node->accept(v)` | **Restores** the runtime type to a static type | Virtual function (runtime) |
| 2nd call `v.visit(*this)` | Selects the processing for each kind | Overloading (compile time) |

Inside `SensorCheck::accept`, the static type of `*this` is `SensorCheck`.
**Only once you get there** can `visit(const SensorCheck &)` be selected.
This is **double dispatch**.

### The three `accept` functions look the same, but you cannot merge them

```cpp
void SensorCheck::accept(DiagVisitor & visitor) const { visitor.visit(*this); }
void MotorCheck::accept(DiagVisitor & visitor) const  { visitor.visit(*this); }
void CheckGroup::accept(DiagVisitor & visitor) const  { visitor.visit(*this); }
```

The three are identical, character for character. You will certainly think, "Can't I write one in the base class?" If you do, this happens.

```cpp
struct DiagNode
{
  virtual ~DiagNode() = default;
  void accept(DiagVisitor & visitor) const { visitor.visit(*this); }   // one in the base
};
```

```
error: no matching member function for call to 'visit'
note: candidate function not viable: no known conversion from 'const DiagNode' to 'const SensorCheck' for 1st argument
note: candidate function not viable: no known conversion from 'const DiagNode' to 'const MotorCheck' for 1st argument
```

This is because **inside the base class, `*this` is a `DiagNode`**.
You cannot remove this copy-paste. The moment you try, double dispatch breaks.
Accept it as the **structural cost** of the Visitor pattern.

## 13.3 Who owns what

In Visitor, ownership shows up in three places. **Each one has a different answer.**

| Target | How you hold it | Reason |
| --- | --- | --- |
| Children of a group | `std::vector<std::unique_ptr<DiagNode>>` | The tree owns its nodes. Same as chapter 11 (Composite) |
| The visitor in `accept` | `DiagVisitor &` | The caller owns the visitor's lifetime. Ownership does not move |
| The element in `visit` | `const Derived &` | You look at it only during the visit. The tree keeps ownership |

**If you feel like writing `accept(std::unique_ptr<DiagVisitor>)`, it is wrong.**
You normally put the visitor on the stack and just pass a reference to it.

```cpp
FailureCountVisitor counter;      // on the stack. zero allocations
root->accept(counter);
std::cout << counter.failure_count() << "\n";
```

**The result of the visit accumulates in members of the visitor.** This is the same as in Java,
but in C++ you must make sure yourself that `counter` stays alive until `accept` finishes.
With the code above, the scope guarantees it.

## 13.4 The price of Visitor: the easy direction to extend is turned 90 degrees

Visitor is **not a tool that gives flexibility for free**. It is clear what gets cheaper and what gets more expensive.

| What you want to do | With Visitor |
| --- | --- |
| Add an **operation** (add a count, add JSON output) | **Just add one new visitor class. Existing code does not change** |
| Add a **kind of element** (add `EncoderCheck`) | **Add a `visit` to `DiagVisitor`, then fix every visitor** |

Plain virtual functions (the approach where the element has a `report()`) are exactly the opposite.
Adding a kind is easy; adding an operation means fixing every element.

This situation, "extend one side and the other side breaks everywhere", is called the **expression problem**.
It does not go away whichever pattern you choose. **All you can choose is which side to make cheap.**

> **Rule of thumb**: If the kinds of elements are almost fixed and operations keep growing, use Visitor.
> If the kinds keep growing, do not introduce Visitor.
> It is the same point as "abstracting something that has only one implementation" in [0. Before you use them](00_before_you_use_them.md):
> **if you have only one visitor, you do not need Visitor.** Adding a member function to the elements is enough.

In club code, it looks like this.

- "Self-check results" and "communication frame types": **the kinds are fixed**. Visitor or variant works well
- "Sensor types": **they will keep growing**. With Visitor, every new sensor means fixing every visitor

## 13.5 Can't I just branch with `dynamic_cast`?

As a way to avoid writing `accept`, you will certainly think of this.

```cpp
void report(const DiagNode & node)
{
  if (const SensorCheck * const s = dynamic_cast<const SensorCheck *>(&node)) { /* ... */ }
  else if (const MotorCheck * const m = dynamic_cast<const MotorCheck *>(&node)) { /* ... */ }
  // forgot CheckGroup
}
```

It works. But it has three problems.

1. **It compiles even if you forget a case.** The code above silently ignores `CheckGroup`.
   When you add a kind, **nobody tells you where to fix**
2. **It needs RTTI.** On microcontrollers `-fno-rtti` is normal.

   ```
   error: use of dynamic_cast requires -frtti
   ```

3. **It is not fast.** `dynamic_cast` searches the inheritance relationships at runtime.
   The cost is different from one virtual function call

**Number 1 is the essence.** The one point where Visitor beats a chain of `dynamic_cast` is this:
"when you add a kind, a pure virtual `visit` is added to `DiagVisitor`, and
**every visitor that does not implement it becomes a compile error**".
So Visitor is not a tool that "breaks everything", but one that "**tells you about everything that breaks, without missing any**".

## 13.6 Does the standard library or the language have the same thing? `std::variant` and `std::visit`

**Yes. C++17's `std::variant` + `std::visit` is exactly Visitor.**
And it needs no inheritance, no virtual functions, and no `accept`.

```cpp
struct SensorSample { std::string name; int value_mv; int limit_mv; };
struct MotorSample  { std::string name; unsigned int fault_bits; };

using DiagValue = std::variant<SensorSample, MotorSample>;   // holds exactly one of them
```

`DiagValue` is "a box that holds exactly one of the three types". 
There is **no** common base class. The three types may have no relation to each other.

You branch with `std::visit`.

```cpp
std::visit(
  overloaded{
    [](const SensorSample & s) { std::cout << "sensor " << s.value_mv << "mV\n"; },
    [](const MotorSample & m) { std::cout << "motor fault=" << m.fault_bits << "\n"; }},
  value);
```

### You write the `overloaded` idiom yourself

It is **not in** the C++17 standard library. It is 5 lines, so you write it yourself.

```cpp
template <class ... Ts>
struct overloaded : Ts ...
{
  using Ts::operator() ...;
};

template <class ... Ts>
overloaded(Ts ...) -> overloaded<Ts ...>;      // deduction guide. required in C++17
```

A lambda is "an unnamed class with one `operator()`".
If you **inherit from all of them** and make every `operator()` visible with `using`,
you get "one function object whose overload is resolved by the argument type".
That is what `std::visit` asks for.

**If you forget the deduction guide in the last 2 lines, you get this.**

```
error: no viable constructor or deduction guide for deduction of template arguments of 'overloaded'
note: candidate function template not viable: requires 1 argument, but 2 were provided
```

C++20 added CTAD for aggregates, so the deduction guide is not needed.
**In C++17, write it.** The exercise makes you write it too.

### Exhaustiveness is guaranteed at compile time

This is the decisive difference from a chain of `dynamic_cast`.
If you add a kind `EncoderV` to the variant and forget to add a lambda,

```
error: static assertion failed due to requirement
  'is_invocable_v<overloaded<...>, EncoderV &>':
  `std::visit` requires the visitor to be exhaustive.
```

**it fails with an error that says "the visitor is not exhaustive".**
When you add a kind, **the compiler lists every `std::visit` call you need to fix**.
It is exactly the same effect as when adding a pure virtual `visit` in the GoF version makes every visitor fail.
The difference is that **you did not write a single inheritance hierarchy for it**.

### How to represent a tree

You need to be careful when you build a tree with variant.
If `GroupSample` holds a `std::vector<DiagValue>`,
**`DiagValue` appears inside the definition of `DiagValue`** (a recursive type),
and you would pass an incomplete type to `std::variant`. This is undefined behavior.

The exercise **holds children by index**.

```cpp
struct GroupSample
{
  std::string name;
  std::vector<std::size_t> children;   // positions inside DiagArena
};

class DiagArena          // a place where nodes are laid out flat
{
public:
  std::size_t add(DiagValue value);
  const DiagValue & at(std::size_t index) const;
};
```

The shape of the tree is expressed with indexes. Because the `new` for each node disappears,
**on a microcontroller you can replace it with one fixed-length array** (13.9).

## 13.7 Which one to choose

| Situation | What to choose | Reason |
| --- | --- | --- |
| The kinds of elements are fixed at compile time | **`std::variant`** | Zero inheritance. Exhaustiveness shows up at compile time |
| Users of another library add kinds | **GoF version** | With variant you must write the whole list of kinds in one place |
| Kinds are decided at runtime, such as plug-ins | **GoF version** | Variant cannot express it |
| You do not want to use the heap | **`std::variant`** | It holds the contents directly. No per-node allocation |
| Elements have very different sizes | **GoF version** | With variant, every element has **the size of the largest member** |
| The number of operations (visitors) keeps growing | Either is fine | Both are strong in this direction |
| The kinds of elements keep growing | **Use neither** | Use virtual functions on the elements |

The last 2 rows are the point of 13.4. **First decide which direction will grow**, then choose.

The weak point of `std::variant` is row 5.

```
sizeof(NodeV) = 8      // for SensorV{int} and MotorV{unsigned int}
```

It is a gain if all types are small, but if you mix in just one type with a 1 KB member,
**every node becomes 1 KB**. In that case, move only the large one out with a `unique_ptr`.

## 13.8 Try it yourself

It is complete in one file. **Predict the output** before you run it.

```cpp
#include <iostream>
#include <string>
#include <variant>
#include <vector>

// ---- 1) Check that overloads are chosen by the static type ----
struct Node { virtual ~Node() = default; };
struct Sensor : Node {};
struct Motor : Node {};

void describe(const Sensor &) { std::cout << "sensor\n"; }
void describe(const Motor &) { std::cout << "motor\n"; }
void describe(const Node &) { std::cout << "node (kind lost)\n"; }

// ---- 2) With std::variant, you can choose by the runtime content ----
struct SensorV { int mv; };
struct MotorV { unsigned int fault; };
using NodeV = std::variant<SensorV, MotorV>;

template <class ... Ts>
struct overloaded : Ts ...
{
  using Ts::operator() ...;
};

template <class ... Ts>
overloaded(Ts ...) -> overloaded<Ts ...>;

int main()
{
  Sensor sensor;
  Motor motor;
  const Node * const nodes[] = {&sensor, &motor};

  for (const Node * const node : nodes) {
    describe(*node);
  }

  const std::vector<NodeV> values = {SensorV{11800}, MotorV{3U}};
  for (const NodeV & value : values) {
    std::visit(
      overloaded{
        [](const SensorV & s) { std::cout << "sensor " << s.mv << "mV\n"; },
        [](const MotorV & m) { std::cout << "motor fault=" << m.fault << "\n"; }},
      value);
  }

  std::cout << "sizeof(NodeV) = " << sizeof(NodeV) << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: what do the first 2 lines print? There are 3 overloads of <code>describe</code></summary>

```
node (kind lost)
node (kind lost)
sensor 11800mV
motor fault=3
sizeof(NodeV) = 8
```

In the first half, **both calls go to `describe(const Node &)`**.
The static type of `*node` is `const Node &`, so only that one is chosen.
The fact that the object is a `Sensor` has no effect on overload resolution at all.
**To make the choice by runtime type, the GoF version puts one virtual function, `accept`, in between.**

In the second half, the function is chosen per kind. There is no inheritance and no `accept`.
This is because `std::visit` looks at the discriminator inside the variant and jumps to the matching `operator()`.

The last 8 bytes are the result of aligning "a 4-byte member + a discriminator".
**It uses 0 bytes of heap.**

To go further, delete one lambda in the second half.

```
error: static assertion failed ... `std::visit` requires the visitor to be exhaustive.
```

You can confirm that **forgetting a case becomes a compile error**.
This does not happen with a chain of `dynamic_cast`.
</details>

## 13.9 Conclusion for microcontrollers

**The variant version is the main choice.** The GoF version pays three costs on a microcontroller.

1. Every element carries a vtable pointer (4 bytes on 32-bit)
2. If you build the tree with `unique_ptr`, **a heap allocation runs for every node**
3. Each visit makes two virtual function calls (`accept` and `visit`)

With the variant version, you can **lay the nodes out flat in a fixed-length array**.

```cpp
// Zero dynamic allocation. Zero vtables.
struct SensorSample { const char * name; int value_mv; int limit_mv; };
struct MotorSample  { const char * name; unsigned int fault_bits; };
using DiagValue = std::variant<SensorSample, MotorSample>;

DiagValue g_nodes[16];        // static allocation. you decide the upper limit on the count
```

Do not use `std::string`. Names point to string literals in ROM through `const char *`
([the microcontroller constraints table in the README](README.md#microcontrollers-and-ros-2-give-different-conclusions)).

### With `-fno-exceptions`, do not use `std::get`

When you bring in `std::variant`, **this is the only thing to be careful about**.
`std::get<T>(v)` **throws** `std::bad_variant_access` if the content is not `T`.

```cpp
const SensorSample & s = std::get<SensorSample>(value);   // throws if the content differs
```

In a build with `-fno-exceptions`, it cannot throw, so
**it calls `abort()` the moment the content differs**. It stops on a board with no debugger attached.

Use `std::get_if`. **It only returns a pointer and never throws.**

```cpp
if (const SensorSample * const sensor = std::get_if<SensorSample>(&value)) {
  std::printf("%d\n", sensor->value_mv);
} else {
  std::printf("not a sensor\n");
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti try2.cpp -o try2
```

It compiles even with both `-fno-exceptions -fno-rtti`.
**`std::visit` itself also does not throw as long as the variant holds a valid value**
(it becomes `valueless_by_exception` only when an exception broke it, and it throws there.
With `-fno-exceptions` no exception is thrown, so this state does not occur).

To sum up, on a microcontroller you can use `std::variant` safely if you follow these 3 rules.

| What to follow | Reason |
| --- | --- |
| `std::get_if`, not `std::get` | `std::get` throws |
| Do not put `std::string` / `std::vector` in members | They allocate on the heap |
| Hold the tree by index (not by pointer) | Removes the `new` per node |

## 13.10 Conclusion for ROS 2 (supplement)

ROS 2 has looser constraints, so you can write it either way. In fact rclcpp has both.

- `rclcpp::ParameterValue` is **close to a tagged union** that holds one value per type,
  and branches with `get_type()` (the same idea as `std::variant`)
- For processing by message kind, the usual way is simply to **split the callback per type**,
  and it is rare to set up a Visitor class

If you were to write a Visitor on the ROS 2 side, it would be the case where **you split received frames into kinds with your own parser**, and
then apply counting, log formatting, or recording to the result.
The kinds are fixed there too, so `std::variant` is the first candidate.

## 13.11 Common pitfalls

| Symptom | Cause |
| --- | --- |
| `error: no matching member function for call to 'visit'` | You wrote only one `accept` in the base class. `*this` is a `DiagNode` (13.2) |
| Through a base pointer, everything gets the same processing | `accept` is not `virtual`. Overloads are chosen by the static type |
| The element is copied on every visit | The parameter of `visit` is a value, not `const Derived &` |
| Derived-only members cannot be read or their values are broken | You take a base-type value like `visit(DiagNode node)` (slicing) |
| Every visitor failed after I added one kind | **Normal.** That is the benefit of Visitor (13.4) |
| `error: no viable constructor or deduction guide ... 'overloaded'` | You did not write the deduction guide (required in C++17) |
| ``error: ... `std::visit` requires the visitor to be exhaustive.`` | One lambda is missing. Write all kinds of the variant |
| A lambda cannot call itself recursively | A lambda does not know its own name. Make a named function and call `std::visit` inside it |
| `error: use of dynamic_cast requires -frtti` | You used `dynamic_cast` in a `-fno-rtti` build (13.5) |
| `std::get` caused `abort()` on a microcontroller | It cannot throw with `-fno-exceptions`. Use `std::get_if` (13.9) |
| The variant is oddly large | Everyone is sized to the largest member. Move only the large type to `unique_ptr` |

## 13.12 Matching exercise

```bash
./drill run dp13
```

In `exercises/dp13_visitor/src/diagnostics.cpp`, using the result tree of a startup self-check as the subject, you implement

1. **The GoF `accept` (three of them)**: the core of double dispatch
2. **`FailureCountVisitor`**: a visitor that counts NG results (counting)
3. **`TextReportVisitor`**: a visitor that builds an indented report (formatting)
4. **The `overloaded` idiom**: you write it yourself, including the deduction guide
5. **`count_failures` / `make_report`**: the `std::visit` version. It must return **the same result** as 1 to 3

The tests check that the derived `visit` is chosen through a base pointer
(that double dispatch works), that two kinds of visitors apply to the same tree,
that the variant version returns a string that differs by not even one character from the GoF version,
and that the variant types satisfy `static_assert(!std::is_polymorphic_v<...>)`
(that is, they have no virtual functions).

## 13.13 Summary of this chapter

- **Overloads are chosen by the static type.** The only mechanism that chooses by runtime type is the virtual function
- That is why `accept` is needed. **The first virtual call turns the runtime type back into a static type**, and that is `accept`
- The three `accept` functions look the same but **cannot be merged**. In the base, `*this` has the base type
- The parameter of `visit` is `const Derived &`. A value copies, and a base type slices
- Visitor makes **adding operations cheap and adding kinds of elements expensive** (the expression problem).
  Decide the direction of growth first, then choose
- A chain of `dynamic_cast` **compiles even if you forget a case**. It also needs RTTI. It is worse than Visitor
- **C++17 has `std::variant` + `std::visit`.** It needs no inheritance, no virtual functions, and no `accept`, and
  exhaustiveness is guaranteed at compile time (`requires the visitor to be exhaustive`)
- The `overloaded` idiom is not in the standard. **Write it yourself, including the deduction guide** (C++17)
- If the kinds are fixed, use variant. If you want to extend at runtime, use the GoF version
- **On microcontrollers, variant is the main choice.** It uses no heap and no vtable.
  But with `-fno-exceptions`, use **`std::get_if`**, not `std::get`

---

Previous: [12. Decorator](12_Decorator.md) / Next: [14. Chain of Responsibility](14_ChainOfResponsibility.md)
