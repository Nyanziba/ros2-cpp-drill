# 11. Composite

> **Matches chapter 11 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep `Entry` / `File` / `Directory` open next to you.
>
> **Goal of this chapter**: The structure itself is almost the same as the Java version. **Only the ownership changes.**
> If you bring Java's `ArrayList<Entry> directory` to C++ and make it
> `std::vector<Entry>`, **it slices and the derived part disappears**, and if you make it `std::vector<Entry *>`,
> **nobody knows who releases it.**
> The right answer is `std::vector<std::unique_ptr<Entry>>`,
> but if you choose this, **that class can no longer be copied.**
> Also, if you add a link to the parent as in Yuki's book, with `shared_ptr` **it becomes a cycle and is never released.**
> This chapter is almost entirely about lifetime.

## 11.1 Porting the Java version to C++ as is

Yuki's `Entry` looks like this (excerpt).

```java
public abstract class Entry {
    public abstract String getName();
    public abstract int getSize();
    public Entry add(Entry entry) throws FileTreatmentException {
        throw new FileTreatmentException();
    }
    public void printList() { printList(""); }
    protected abstract void printList(String prefix);
}
```

If you move it to C++, it looks like this.

```cpp
class Entry
{
public:
  virtual ~Entry() = default;
  const std::string & name() const { return name_; }
  virtual std::size_t size() const = 0;
  virtual void print_list(const std::string & prefix, std::vector<std::string> & out) const = 0;

protected:
  explicit Entry(std::string name) : name_(std::move(name)) {}

private:
  std::string name_;
};
```

There are 3 changes from the Java version.

### Change 1: Added `virtual ~Entry() = default;`

This is the same story as chapter 1, but **in this chapter the danger is higher than in other chapters.**
In Composite, the children are held with `std::unique_ptr<Entry>`. So
**it is structurally certain that `delete` runs through a pointer to the base class.**
Without a virtual destructor, every time you throw away the tree the destructors of the derived classes are not called,
and neither `std::string` nor `std::vector` is released.

### Change 2: **Removed** `add()` from the base

Yuki's `Entry` has `add()`, and if you call it on a `File`,
a `FileTreatmentException` is thrown. It is a design where "leaves also have `add()`, but calling it fails at run time".

In C++ we put `add()` only in `Directory`.

```cpp
File file{"imu_whoami", 100};
file.add(...);       // error: no member named 'add' in 'File'
```

**A run-time exception became a compile error.** On microcontrollers `-fno-exceptions` is normal,
so `throw` is not even an option. All you can do is return a value and make the caller check it,
and nobody checks it.

The GoF book also has this discussion ("transparency vs safety"). The Java version takes transparency,
puts `add()` in the base, and fails on the leaf. **C++ takes safety.**
The reason is that C++ has few ways to safely ask "is this a leaf?" without using `dynamic_cast`,
and on microcontrollers `dynamic_cast` itself is often unusable because of `-fno-rtti`.

The case where you really need transparency (you want to call `add` without distinguishing leaves and nodes) almost never comes.
The code that calls `add` always knows "where am I making a group now".

### Change 3: `print_list` does not return a string and pushes onto `out`

The Java version calls `System.out.println` directly.
Avoid having a library class write to standard output, even in club code.
It becomes untestable. Either pass `std::vector<std::string> & out` and push onto it, or
receive a callback. **A microcontroller has no standard output in the first place**, too.

## 11.2 Who owns the children?

The Java version looks like this.

```java
private List<Entry> directory = new ArrayList<>();
public Entry add(Entry entry) { directory.add(entry); return this; }
```

In C++ there are 4 choices, and **only 1 is correct.**

| Way of writing | What happens | Verdict |
| --- | --- | --- |
| `std::vector<Entry>` | **Slicing.** Only the `Entry` part is copied and the derived part disappears. If it is an abstract class, it does not even compile | ✗ |
| `std::vector<Entry *>` | The type does not say who calls `delete`. You end up writing the code to throw away the tree yourself | ✗ |
| `std::vector<std::shared_ptr<Entry>>` | It works. But **there is no reason to share.** It is heavier by the reference count, and if you add a link to the parent it becomes a cycle | △ |
| `std::vector<std::unique_ptr<Entry>>` | The parent owns alone. If the parent dies, the whole tree dies | **○** |

```cpp
class Directory : public Entry
{
public:
  void add(std::unique_ptr<Entry> child);

private:
  std::vector<std::unique_ptr<Entry>> children_;
};
```

**"The owner of the tree is the root" is now written in the type.** You do not write a single `delete`.
If you throw away the `unique_ptr` of the root, it chains: the destructor of `vector` →
the destructor of each `unique_ptr` → the destructor of the child `Directory` → ...

If there is a reason to choose `shared_ptr`, it is when you "want to hang the same subtree from 2 parents",
but that is not a tree, it is a DAG. **The tree of diagnostic items has no such requirement.**
"Somehow `shared_ptr`" is exactly the over-application I mentioned in chapter 0.

## 11.3 Slicing: if you hold by value, the derived part silently disappears

This is the first of the C++-specific dangers. If `Entry` has a pure virtual function,
`std::vector<Entry>` is a compile error, so you notice.
**What is scary is the case where the base has an implementation and it compiles.**

```cpp
class Entry
{
public:
  virtual ~Entry() = default;
  virtual int size() const { return 0; }   // not pure virtual
};

class Check : public Entry
{
public:
  explicit Check(int size) : size_(size) {}
  int size() const override { return size_; }

private:
  int size_;
};

std::vector<Entry> by_value;
by_value.push_back(Check{100});            // it compiles
by_value[0].size();                        // ?
```

Measured result (we run the whole program in 11.9).

```
vector<Entry>            : 0
vector<unique_ptr<Entry>>: 100
```

`push_back` calls the copy constructor of `Entry`, and
**cuts off the `Check` part (both `size_` and the vtable).**
There is no error and no warning. This is the reason you must not hold a `Composite` in a `vector` of values.

Java does not have this phenomenon. `ArrayList<Entry>` always holds references.

## 11.4 A class that has a `vector` of `unique_ptr` cannot be copied

As soon as you make `std::vector<std::unique_ptr<Entry>>` a member,
the **copy constructor of `Directory` is implicitly deleted.**

```cpp
Directory a{"motors"};
Directory b = a;      // error: call to implicitly-deleted copy constructor
```

This is not an accident. It is **correct** behavior. If you could copy a `Directory`,
2 parents would own the same children, and you would get a double free.
If you want to copy, you have to write a `clone()` that duplicates the tree recursively yourself
(this is the story of chapter 6, Prototype).

The form of the argument of `add()` is also decided by this constraint.

```cpp
void add(std::unique_ptr<Entry> child);          // take by value
```

`unique_ptr` cannot be copied, so **the caller must always pass it by moving.**
"I gave up ownership here" appears in the caller's code as `std::move`.

```cpp
auto motors = std::make_unique<Directory>("motors");
motors->add(std::make_unique<Check>("motor_l", true));   // a temporary object is moved automatically
root->add(std::move(motors));                            // a named variable needs std::move
// from here on, motors is nullptr. Using it will crash
```

The receiving side also needs `std::move`.

```cpp
void Directory::add(std::unique_ptr<Entry> child)
{
  children_.push_back(std::move(child));   // if you forget move, it is a compile error
}
```

`child` is a **variable with a name**, so as it is, it is an lvalue. If you forget `std::move`,
you are told "the copy constructor of `unique_ptr` is deleted".
**This error is normal.** The fix is to add `std::move`.

Also, if you write a destructor yourself (we write one in this exercise for logging),
**the move constructor is no longer generated implicitly.** Write out all 5.

```cpp
~Directory() override;
Directory(const Directory &) = delete;
Directory & operator=(const Directory &) = delete;
Directory(Directory &&) noexcept = default;
Directory & operator=(Directory &&) noexcept = default;
```

## 11.5 If you hold a pointer to the parent, it becomes a cycle

The exercises of Yuki's book include the idea of "make it possible to go up to the parent".
If you do this in C++ with `shared_ptr`, **parent and child own each other and are never released.**

```cpp
struct Bad
{
  std::vector<std::shared_ptr<Bad>> children;
  std::shared_ptr<Bad> parent;      // the parent is also shared_ptr = a cycle
};

struct Good
{
  std::vector<std::shared_ptr<Good>> children;
  std::weak_ptr<Good> parent;       // the parent is weak_ptr. Does not own
};
```

This is the result of measuring with logs in the destructors (the code is in the second half of 11.9).

```
Bad  root.use_count = 2
--- left the scope of Bad ---
Good root.use_count = 1
~Good root
~Good leaf
--- left the scope of Good ---
```

**`Bad` does not print a single `~Bad` line.** Both of them leaked.
Even if you throw away `root`, `leaf` holds `root` as `parent`, so the count does not reach 0.
`leaf` is in the `children` of `root`, so that one does not reach 0 either.
In Java the GC collects "cycles that cannot be reached from outside", so this problem does not happen.
**Reference counting cannot collect cycles.** This is the fundamental limit of `shared_ptr`.

The fact that `use_count` is 2 for `Bad` and 1 for `Good` is the cause itself.

### What do we do in this exercise?

**We do not hold a link to the parent.** Because we do not need it.
When you need it, there are 2 choices.

| Way | Condition |
| --- | --- |
| Raw pointer `Entry * parent_` | The child is owned by the parent, so **the parent can never die before the child**. So it is safe |
| `std::weak_ptr<Entry> parent_` | Only when the child is held by `shared_ptr`. `lock()` is needed |

**If you hold the children with `unique_ptr`, a raw pointer for the parent is correct.**
"Raw pointer = dangerous" is not true. It is enough that **the fact that it is a non-owning pointer** can be expressed in the type.
In code that uses `unique_ptr` (owns) and raw pointers (does not own) properly, you can tell the ownership by reading it.

## 11.6 Recursive destructors and the stack

The chain of `unique_ptr` is released automatically, but **that release is a recursive call.**
If you throw away a deep tree, the nesting of destructors uses the stack as it is.

This is a measurement of building a linked list 200,000 levels deep with `unique_ptr` and releasing it.

```cpp
struct N { std::unique_ptr<N> next; };
// connect 200,000 levels, then head.reset();
```

```
built
(SIGSEGV here. exit code 139)
```

**The construction succeeded, and the release overflowed the stack.** `freed` was not printed.
The stack of the main thread on macOS is 8 MB, and this is what happened.
The stack of a microcontroller is **a few KB to a few tens of KB**, so think that even a few hundred levels are dangerous.

If the **depth is 3 to 4 levels** like the tree of diagnostic items, there is no problem at all.
It becomes a problem when you express a linked list with a tree class instead of a tree,
or when the depth depends on the input, like the syntax tree of a parser.
In that case, you write an explicit loop in the destructor (take apart the tree with your own stack).

First check **whether the depth is decided by the structure or by the input.**

## 11.7 Is there something in the standard library / language that does the same?

**No.** The standard library has no general-purpose class template that corresponds to Composite.

Close things are `std::filesystem::directory_entry` and
`std::filesystem::recursive_directory_iterator`, but these are
"tools to walk an existing tree, the file system",
and **not tools to build your own tree.**

As a way to express a tree by type, C++ has one choice that Java does not have: **nested types.**

```cpp
// if the structure is decided at compile time, this is also a kind of Composite
template <typename... Children>
class Group;
```

If you hold the children in a `std::tuple`, you can do recursive aggregation with zero virtual functions and zero dynamic allocation.
It works only when the shape of the tree is decided at compile time, but it is powerful on microcontrollers (11.8).

## 11.8 Try it yourself

Before you solve the exercise, run these 2 programs **after predicting the output.**

### Part 1: Slicing

```cpp
#include <iostream>
#include <memory>
#include <vector>

// We deliberately do not make this an abstract class. If it were abstract, vector<Entry> could not even be created,
// and we could not reproduce the scariest state: "it slices and runs silently".
class Entry
{
public:
  virtual ~Entry() = default;
  virtual int size() const { return 0; }
};

class Check : public Entry
{
public:
  explicit Check(int size) : size_(size) {}
  int size() const override { return size_; }

private:
  int size_;
};

int main()
{
  std::vector<Entry> by_value;
  by_value.push_back(Check{100});
  std::cout << "vector<Entry>            : " << by_value[0].size() << "\n";

  std::vector<std::unique_ptr<Entry>> by_pointer;
  by_pointer.push_back(std::make_unique<Check>(100));
  std::cout << "vector<unique_ptr<Entry>>: " << by_pointer[0]->size() << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: what is printed on the lines above? Is a warning printed?</summary>

```
vector<Entry>            : 0
vector<unique_ptr<Entry>>: 100
```

**Not a single warning is printed.** It passes silently even with `-Wall -Wextra -Wpedantic`.

`push_back(Check{100})` calls the copy constructor of `Entry`,
and throws away the `size_` and the vtable of `Check`. The remaining `Entry` returns 0 from `size()`.

If you make `Entry::size()` `= 0` (pure virtual), then at `push_back` you get
`error: allocating an object of abstract class type 'Entry'`.
**If you put in one pure virtual function, this accident is stopped at compile time.**
This is the practical reason to keep the base class of Composite abstract.
</details>

### Part 2: If you hold the parent with `shared_ptr`, it is not released

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct Bad
{
  explicit Bad(std::string name) : name_(std::move(name)) {}
  ~Bad() { std::cout << "~Bad " << name_ << "\n"; }

  std::vector<std::shared_ptr<Bad>> children;
  std::shared_ptr<Bad> parent;        // the parent is also shared_ptr = a cycle
  std::string name_;
};

struct Good
{
  explicit Good(std::string name) : name_(std::move(name)) {}
  ~Good() { std::cout << "~Good " << name_ << "\n"; }

  std::vector<std::shared_ptr<Good>> children;
  std::weak_ptr<Good> parent;        // the parent is weak_ptr. Does not own
  std::string name_;
};

int main()
{
  {
    auto root = std::make_shared<Bad>("root");
    auto leaf = std::make_shared<Bad>("leaf");
    root->children.push_back(leaf);
    leaf->parent = root;
    std::cout << "Bad  root.use_count = " << root.use_count() << "\n";
  }
  std::cout << "--- left the scope of Bad ---\n";

  {
    auto root = std::make_shared<Good>("root");
    auto leaf = std::make_shared<Good>("leaf");
    root->children.push_back(leaf);
    leaf->parent = root;
    std::cout << "Good root.use_count = " << root.use_count() << "\n";
  }
  std::cout << "--- left the scope of Good ---\n";
  return 0;
}
```

<details>
<summary>Predict: how many times is <code>~Bad</code> printed?</summary>

**0 times.**

```
Bad  root.use_count = 2
--- left the scope of Bad ---
Good root.use_count = 1
~Good root
~Good leaf
--- left the scope of Good ---
```

The difference in `use_count` is everything. In `Bad`, `leaf->parent` holds `root`, so it is 2.
Even when the `root` variable disappears at the end of the scope, 1 remains, and it is not released.
`leaf` is also still held by `root->children`, so both of them leak.

In `Good`, `parent` is a `weak_ptr`, so it does not increase the count. It stays 1 → becomes 0, and
the release goes in the order `root` → `children` → `leaf`.

**This program does not crash and does not print an error.** The memory just quietly remains.
If you do this in a node that runs for a long time or on a microcontroller, the allocation will eventually fail.
</details>

## 11.9 Conclusion for microcontrollers

The conclusion is that **a dynamic tree structure itself is hard to use.**
`std::vector<std::unique_ptr<Entry>>` runs a heap allocation for every node
just to build one tree. Once at startup might be fine, but
you cannot write code that rearranges the tree while running.

### Way 1: If the structure is decided at compile time, put it in a `const` array

The tree of diagnostic items **already has its shape decided at power-on.** Then put it in ROM.

```cpp
#include <cstddef>
#include <cstdint>

struct DiagNode
{
  const char * name;
  bool (*check)();            // function pointer for a leaf. nullptr for a node
  std::uint8_t first_child;   // meaningful only for a node
  std::uint8_t child_count;
};

bool check_imu_whoami();
bool check_imu_bias();
bool check_motor_l();

// The tree is expressed as "array + index". No pointers, no dynamic allocation, no vtable.
// Because it is constexpr, it is placed in ROM and uses not a single byte of RAM.
constexpr DiagNode kTree[] = {
  {"robot",      nullptr,           1, 2},  // 0: children are 1,2
  {"imu",        nullptr,           3, 2},  // 1: children are 3,4
  {"motors",     nullptr,           5, 1},  // 2: child is 5
  {"imu_whoami", check_imu_whoami,  0, 0},  // 3: leaf
  {"imu_bias",   check_imu_bias,    0, 0},  // 4: leaf
  {"motor_l",    check_motor_l,     0, 0},  // 5: leaf
};

/// Run everything under index and return the number of passes. Leaves and nodes are handled by the same function.
std::uint8_t run_diagnostics(std::uint8_t index)
{
  const DiagNode & node = kTree[index];
  if (node.check != nullptr) {
    return node.check() ? 1 : 0;
  }
  std::uint8_t passed = 0;
  for (std::uint8_t i = 0; i < node.child_count; ++i) {
    passed = static_cast<std::uint8_t>(passed + run_diagnostics(static_cast<std::uint8_t>(node.first_child + i)));
  }
  return passed;
}
```

The purpose of Composite, **to treat leaves and nodes the same**, is achieved with this.
The caller of `run_diagnostics(0)` does not know whether number 0 is a leaf or a group.

What we gained: zero allocation, zero vtable, zero `std::string`.
Each `DiagNode` is 2 pointers + 2 bytes, and it is placed in ROM.

What we lost: you cannot rearrange the tree at run time. **First check whether that is acceptable.**
In a club robot, a situation where you add diagnostic items while running almost never comes.

Recursion is still there, but **the depth is decided by the shape of `kTree`**, so you can read the upper limit.
If you are worried, keep your own stack of indices and turn it into a loop.

### Way 2: If you want to rearrange at run time, keep a fixed-length node pool

```cpp
template <std::size_t Capacity>
class DiagTree
{
public:
  /// Returns the index if added, and Capacity if full (does not throw)
  std::size_t add_check(const char * name, bool (*check)());

private:
  DiagNode nodes_[Capacity] = {};
  std::size_t used_ = 0;
};
```

The allocation happens **only once, when you create this one object at startup.**
When it is full you cannot `throw` (`-fno-exceptions`), so
**the failure is returned as a return value.** The caller only needs to check it once at startup.

### When you may use the `std::unique_ptr` version

If you build the tree once at startup and then only run it, and there is enough heap
(tens of KB are usable on an RTOS), you may bring the implementation of the exercise as it is.
The only condition is **do not call `add()` inside a loop.**

## 11.10 Conclusion for ROS 2 (supplement)

In ROS 2, trees are often expressed **flat, with names separated by `/`, instead of nested classes.**

- The parameter `motors.left.max_duty` looks hierarchical, but inside it is a string key
- Diagnostic messages also embed the hierarchy in the name and are sent as a flat array

The reason `full_names()` in this exercise builds `"/robot/imu/imu_whoami"` is that
it follows the actual way ROS 2 does it: **build the tree inside the node, and use flat names when sending it out.**
You talk in terms of the tree only inside the node.

Also, ROS 2 "composition" (`ComposableNode`, the mechanism to put several nodes into one process) is
**unrelated to the Composite pattern.** The names are only similar, so do not confuse them.

## 11.11 Common pitfalls

| Symptom | Cause |
| --- | --- |
| `error: call to implicitly-deleted copy constructor of 'std::unique_ptr<...>'` | `children_.push_back(child)` has no `std::move` |
| You passed a variable to `add()` and got a compile error | A named variable is an lvalue. Write `add(std::move(group))` |
| You used the original variable after `add()` and it crashed | It was moved from and is `nullptr`. Never use it again after moving |
| You throw away the tree but the destructors of the children are not called | The base has no virtual destructor |
| The aggregate is always 0 | You hold the children in `std::vector<Entry>` and they were sliced |
| Memory keeps shrinking. No crash | You hold the parent with `shared_ptr` and made a cycle. Use `weak_ptr` or a raw pointer |
| You wrote a function that returns `Group` and were told it cannot be moved | You wrote the destructor yourself, so move is not generated implicitly. Write out all 5 |
| SIGSEGV when throwing away a deep tree | Stack overflow from the recursion of destructors (11.6) |
| You made a design where `add()` on a leaf fails at run time | Put `add()` only on nodes (11.1 Change 2) |

## 11.12 Matching exercise

```bash
./drill run dp11
```

The subject is the **self-diagnosis of a robot at startup.**
You treat individual diagnostics (leaves) such as "read WHO_AM_I of the IMU" and
groups (nodes) such as "all diagnostics about the IMU" the same way, and
**run the whole tree with one `run()` and aggregate the pass/fail results.**

You implement it in `exercises/dp11_composite/src/diagnostic_tree.cpp`.

1. **`DiagnosticCheck` (leaf)**: `check_count()` / `run()` / `collect_names()`
2. **`DiagnosticGroup::add()`**: take a `std::unique_ptr<DiagnosticEntry>` by value and put it in by move
3. **`DiagnosticGroup` (node)**: `check_count()` / `run()` / `collect_names()`, which recurse into the children and aggregate

The tests check the following 4 points.

- Leaves and nodes go into the same array of `const DiagnosticEntry *`, and can be aggregated with the same call
- The recursive aggregation (the total number of leaves, the numbers of passes and failures) is correct
- **When the parent is destroyed, the children are also destroyed** (we observe the logs of the destructors)
- **`DiagnosticGroup` is not copyable and is movable** (`static_assert`)

You do not write a single `delete`. If you feel like writing one, the design is wrong.

## 11.13 Summary of this chapter

- The structure is almost the same as the Java version. **Only the ownership is different**
- The children are `std::vector<std::unique_ptr<Entry>>`.
  By value it is **slicing**, with raw pointers **the releaser is unknown**, and `shared_ptr` is too much without a reason
- A class that holds `unique_ptr` **cannot be copied.** `add()` takes by value and puts in with `std::move`
- If you write a destructor yourself, **write out all 5 of copy/move**
- **If you hold the parent with `shared_ptr`, it becomes a cycle and is never released.** The parent is a raw pointer or `weak_ptr`
- If you hold the children with `unique_ptr`, **a raw pointer for the parent is correct** (because the parent does not die first)
- Release is recursive. **A deep tree can overflow the stack.** Check whether the depth depends on the input
- Put `add()` **only on nodes**, not on the base. The run-time exception of Yuki's book becomes a compile error
- On microcontrollers, do not build a dynamic tree. Express the tree with a **`constexpr` array + indices**

---

Previous: [10. Strategy](10_Strategy.md) / Next: 12. Decorator (coming soon)
