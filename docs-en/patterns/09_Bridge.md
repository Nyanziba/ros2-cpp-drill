# 9. Bridge

> **Matches chapter 9 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep `Display` / `CountDisplay` (the class hierarchy for functions) and
> `DisplayImpl` / `StringDisplayImpl` (the class hierarchy for implementations) open next to you.
>
> **Goal of this chapter**: The structure of Bridge is almost the same as in the Java version. But in C++,
> **the exact same structure is used for a completely different purpose under a different name: "Pimpl".**
> When the purpose is different, the choice of whether the implementation side is virtual also changes.
> C++ also has one pitfall that Java does not have.
> If you hold a `std::unique_ptr<Impl>`, **you cannot write the destructor in the header.**
> This is the main topic of this chapter.

## 9.1 Porting the Java version to C++ as is

Yuki's `Display` looks like this.

```java
public class Display {
    private DisplayImpl impl;
    public Display(DisplayImpl impl) { this.impl = impl; }
    public void open()  { impl.rawOpen();  }
    public void print() { impl.rawPrint(); }
    public void close() { impl.rawClose(); }
    public final void display() { open(); print(); close(); }
}
```

If you move it to C++ in a straightforward way, it looks like this.

```cpp
class TelemetrySink
{
public:
  virtual ~TelemetrySink() = default;
  virtual void open() = 0;
  virtual void put_line(const std::string & line) = 0;
  virtual void close() = 0;
};

class TelemetryView
{
public:
  explicit TelemetryView(std::unique_ptr<TelemetrySink> sink)
  : sink_(std::move(sink))
  {
  }

  virtual ~TelemetryView() = default;

  void show(const std::string & text);

protected:
  TelemetrySink & sink() { return *sink_; }

private:
  std::unique_ptr<TelemetrySink> sink_;
};
```

There are 3 changes from the Java version.

### Change 1: Added a virtual destructor to both the implementation side and the function side

For the implementation side (`TelemetrySink`), this is obvious. It is released through `unique_ptr<TelemetrySink>`,
so without it the destructor of the derived class is not called.

**The function side (`TelemetryView`) needs one too.** The function side of Bridge is always derived from
(`CountDisplay` in Yuki's book).

```cpp
std::unique_ptr<TelemetryView> view = std::make_unique<RepeatView>(/* ... */);
// when view is destroyed, the members of RepeatView are not released
```

**"The function side is not meant to be inherited, so it does not need virtual" does not hold for Bridge.**
Bridge is a pattern that extends the function side by inheritance.

### Change 2: Made the `impl` field a `std::unique_ptr`

`private DisplayImpl impl;` in Java is a reference. The GC collects it.
To give the same meaning in C++, **you write who releases it in the type**.

```cpp
TelemetrySink * sink_;                    // who deletes it? It is not written
std::unique_ptr<TelemetrySink> sink_;     // TelemetryView owns it. Clear
```

`TelemetrySink & sink_;` (a reference member) is also possible,
but a reference member makes the class non-assignable and moves lifetime management outside.
**Bridge is a pattern that "holds exactly one implementation", so an owning `unique_ptr` is the natural choice.**

### Change 3: Expressed `final` on `display()` by not writing `virtual` at all

The Java version has `public final void display()`. It declares "do not override this".
In C++, if you do not write `virtual`, it cannot be overridden from the start.
You do not even need to write `final`.

**Java is "virtual by default", so you stop it with `final`. C++ is "non-virtual by default", so
you need to do nothing.** On the other hand, if you forget to write `virtual` where you want to allow overriding,
**it fails silently** (the same story as chapter 3, Template Method).

## 9.2 Who owns the implementation?

In Bridge, the function-side object holds one implementation object.
There are 3 possible forms of ownership.

| Form | Meaning | When to use |
| --- | --- | --- |
| `std::unique_ptr<Impl>` | The function side owns it alone | **Default. The exercise of this chapter also uses this** |
| `std::shared_ptr<Impl>` | Several function sides share the implementation | Several Views use one UART, for example |
| `Impl &` (reference member) | Does not own it. The outside keeps it alive | When the implementation is global (a peripheral) |

The third form is really used on microcontrollers. There is only one `UART1` in the world,
and it lives from the start to the end of the program, so there is no need to own it.

The constructor looks like this.

```cpp
explicit TelemetryView(std::unique_ptr<TelemetrySink> sink)
: sink_(std::move(sink))
{
}
```

**If you forget to write `std::move`, it is a compile error because `unique_ptr` cannot be copied.**
This is one of the few places where C++ protects you.

The caller writes it like this.

```cpp
TelemetryView view{std::make_unique<RecordingSink>(log)};
```

"Choose the implementation at run time" is now concentrated in this one line. This is the value of Bridge.

## 9.3 Why a member and not inheritance?

First, let us see what happens if you do not use Bridge.
Suppose there are 2 kinds of functions (output as is / output repeatedly) and 2 kinds of implementations (record as is / numbered).
If you build it with inheritance only, it looks like this.

```
RecordingView          NumberedView
RepeatRecordingView    RepeatNumberedView
```

That is **4 classes**. Adding one function adds 2 classes, and adding one implementation adds 2 classes.
It is M×N.

With Bridge, you have **M+N classes**: `TelemetryView` / `RepeatView` / `RecordingSink` / `NumberedSink`,
and you can make the combinations at run time.

```cpp
TelemetryView a{std::make_unique<RecordingSink>(log)};
TelemetryView b{std::make_unique<NumberedSink>(log)};
RepeatView    c{std::make_unique<RecordingSink>(log), 3};
RepeatView    d{std::make_unique<NumberedSink>(log), 3};
```

**The essence is that the inheritance now has two vertical lines, and `unique_ptr` is the bridge that connects the two.**

> **Note for 9.3**: 0.1 of [0. Before you use them](00_before_you_use_them.md) applies here too.
> **If there is only one kind of implementation, do not add Bridge.** It only adds one class.
> Add Bridge only when you know **now** that "both the function and the implementation will grow".

## 9.4 Bridge and Pimpl have the same structure. The purpose is different

When you write C++, you always meet a class like this.

```cpp
class LinkStats
{
public:
  LinkStats();
  ~LinkStats();
  void add_sample(double latency_ms);
  double mean() const;

private:
  struct Impl;                    // forward declaration only
  std::unique_ptr<Impl> impl_;    // the bridge to the implementation
};
```

**The structure is exactly the same as Bridge.** The class holds one pointer to the implementation as a member,
and the public methods delegate to it. This is **Pimpl** (pointer to implementation).

The difference is the purpose.

| | Bridge | Pimpl |
| --- | --- | --- |
| What it is for | To **swap** the implementation (extend on 2 axes) | To **hide** the implementation (compile time, ABI) |
| Number of implementations | 2 or more. That is why it is worth having | **Always 1** |
| Is the implementation polymorphic? | **Yes. A pure virtual base is needed** | **No. It is just a struct** |
| Type name of the implementation | Appears in the header (because the user chooses it) | Does not appear in the header (only `struct Impl;`) |
| Hierarchy of the function side | Extended by inheritance | Often not extended |
| Cost | Virtual call + 1 heap allocation | **Indirection + 1 heap allocation. No virtual** |

**If someone says "write Bridge in C++", the implementation side is pure virtual. If someone says "make it Pimpl",
the implementation side is not virtual.** Because the shape is the same, they are easy to mix up.

In the exercise you write both. `TelemetryView` is Bridge, and `LinkStats` is Pimpl.

## 9.5 A C++-specific danger: you cannot write the destructor in the header

**This is the one you step on most often in this chapter.** Take the `LinkStats` from before and,
thinking "the destructor does nothing", delete the declaration `~LinkStats();`.

```cpp
#include <memory>

class LinkStats
{
public:
  LinkStats();
  void add_sample(double v);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

int main()
{
  LinkStats stats;   // the destructor is required here
  stats.add_sample(1.0);
  return 0;
}
```

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -c pimpl_bad.cpp
```

```
/.../c++/v1/__memory/unique_ptr.h:75:19: error: invalid application of 'sizeof' to an incomplete type 'LinkStats::Impl'
   75 |     static_assert(sizeof(_Tp) >= 0, "cannot delete an incomplete type");
      |                   ^~~~~~~~~~~
/.../c++/v1/__memory/unique_ptr.h:259:71: note: in instantiation of member function 'std::unique_ptr<LinkStats::Impl>::~unique_ptr' requested here
  259 |   _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX23 ~unique_ptr() { reset(); }
pimpl_bad.cpp:10:10: note: forward declaration of 'LinkStats::Impl'
   10 |   struct Impl;
      |          ^
```

(With GCC, the message reads `static assertion failed: can't delete an incomplete type`.
Both have the same cause.)

**Reason**: The place where the compiler generates the implicit destructor is the closing brace of `class LinkStats { ... };`.
At that point `Impl` is only forward-declared = an **incomplete type**.
The destructor of `std::unique_ptr` needs `sizeof(Impl)` in order to call `delete`.
You cannot use `sizeof` on an incomplete type.

**Fix**: **Write only the declaration of the destructor in the header, and put the definition in the `.cpp`.**

```cpp
// link_stats.hpp
class LinkStats
{
public:
  LinkStats();
  ~LinkStats();                 // declaration only
  // ...
};
```

```cpp
// link_stats.cpp
struct LinkStats::Impl        // only here does it become a complete type
{
  std::vector<double> samples;
};

LinkStats::~LinkStats() = default;   // here Impl is a complete type
```

**The key point is to write `= default` on the .cpp side.** If you write `~LinkStats() = default;` in the header,
it means "define it here", so you get the same error.

### Move has the same problem

As soon as you declare a destructor, **the move constructor and the move assignment are no longer generated implicitly**
(the C++ rule for special member functions). So `LinkStats` becomes a class that cannot be moved.
You notice this only when you try to put it in a `std::vector<LinkStats>`.

So you add move as well, with the declaration and the definition written separately.

```cpp
// header
LinkStats(LinkStats && other) noexcept;
LinkStats & operator=(LinkStats && other) noexcept;

// .cpp
LinkStats::LinkStats(LinkStats && other) noexcept = default;
LinkStats & LinkStats::operator=(LinkStats && other) noexcept = default;
```

Move assignment "deletes the old `impl_` on the left side", so this also cannot be written in the header.

Remember these as **the 5 lines that always appear in a Pimpl header**.

```cpp
LinkStats();
~LinkStats();
LinkStats(LinkStats &&) noexcept;
LinkStats & operator=(LinkStats &&) noexcept;
LinkStats(const LinkStats &) = delete;          // copy disappears unless you write it yourself
```

Copy is not generated anyway because of `unique_ptr`.
**If you want copy, write it yourself** (`impl_ = std::make_unique<Impl>(*other.impl_);`).
In that case, the definition of the copy constructor is also in the .cpp.

The reason this error does not appear on the Bridge side (`TelemetryView`) is that
`TelemetrySink` is **fully defined** in the header.
Understand that **this is not a problem specific to Pimpl. It is a problem of "incomplete type + `unique_ptr`".**

## 9.6 Fewer header dependencies = fewer recompiles

This is the purpose of Pimpl. Let us see it in real code. The `link_stats.hpp` of the exercise includes only this.

```cpp
#include <cstddef>
#include <memory>
```

On the other hand, `link_stats.cpp` includes this.

```cpp
#include <algorithm>
#include <vector>
```

**Neither `<vector>` nor `<algorithm>` leaks into the header side.**
If you stop using Pimpl and write `std::vector<double> samples_;` as a member in the header,
`link_stats.hpp` has to include `<vector>`.

What changes?

| | With Pimpl | Without Pimpl |
| --- | --- | --- |
| Add 1 member to `Impl` | **Only `link_stats.cpp` is recompiled** | **All .cpp files** that include the header are recompiled |
| Size of the class | Always the size of one pointer | Changes each time you add a member |
| Ship the library as .so | **Users do not need to rebuild** even if you change the implementation (the ABI does not change) | The size changes = the ABI breaks |

For a club library, the first row is the one that matters.
Suppose 30 .cpp files include `link_stats.hpp`.
Without Pimpl, every time you add one member to the statistics, 30 files are recompiled.
With Pimpl, it is 1 file.

The third row matters when you **ship as a shared library**, as ROS 2 does.
If `sizeof(LinkStats)` changes, user code that is already compiled
reserves the wrong size on the stack. With Pimpl, the size never changes.

The test of the exercise checks this with `static_assert`.

```cpp
static_assert(
  sizeof(LinkStats) == sizeof(std::unique_ptr<void *>),
  "LinkStats should be the size of one pointer. Did you write the implementation in the header?");
```

The **costs** are also clear.

- One heap allocation is added (every time you create an object)
- One level of indirection is added to member access. **It is not inlined**
- The code is spread over 2 places. For a small class, the harm is bigger than the benefit

**Decision rule**: Use Pimpl if the header is included by many .cpp files
and the implementation is still changing. **In club code, "you do not need it" is the right answer in most cases.**

## 9.7 Virtual if you want to swap the implementation, non-virtual if you only want to hide it

Let us line up the table of 9.4 in code.

```cpp
// Bridge: the implementation side is pure virtual. It can be swapped
class TelemetrySink { public: virtual ~TelemetrySink() = default; /* ... */ };
class TelemetryView
{
private:
  std::unique_ptr<TelemetrySink> sink_;   // what is inside is decided at run time
};
```

```cpp
// Pimpl: the implementation side is just a struct. It is not swapped
class LinkStats
{
private:
  struct Impl;                            // there is only one kind of implementation in the world
  std::unique_ptr<Impl> impl_;
};
```

**If you change the `Impl` of Pimpl into a pure virtual base, it becomes Bridge as it is.**
Conversely, if you notice that a Bridge has only one kind of implementation,
you can drop the pure virtual and make it a plain `struct Impl`, and it becomes Pimpl. One virtual call disappears.

Making it pure virtual "because with Bridge we can swap it in the future" is
exactly 0.1 of [0. Before you use them](00_before_you_use_them.md). **If there is one implementation, do not make it virtual.**

## 9.8 Is there something in the standard library / language that does the same?

**Bridge itself is not in the standard library.** "Separate two hierarchies" is a matter of design,
and not something a library can provide.

However, the standard has some things that serve as **a way to hold the implementation side** of Bridge.

| Standard tool | What it is in Bridge |
| --- | --- |
| `std::unique_ptr<Impl>` | The bridge itself. Used in this chapter |
| `std::function<void(const std::string &)>` | If the implementation has only one method, you can use this without making a class |
| Template argument | Decide the implementation at compile time. The microcontroller version in 9.10 |

The `std::function` version can be written like this.

```cpp
class TelemetryView
{
public:
  explicit TelemetryView(std::function<void(const std::string &)> put_line)
  : put_line_(std::move(put_line))
  {
  }

private:
  std::function<void(const std::string &)> put_line_;
};
```

**If the interface of the implementation side is a single method, this is enough.** One class hierarchy disappears.
We treat this decision head-on in chapter 10, Strategy.
If **several methods share state**, like `open()` / `put_line()` / `close()`,
one pure virtual class is more natural than holding three `std::function`.

On the other hand, **Pimpl is widely used as a standard C++ idiom.** The implementations of the standard library itself,
Qt (`Q_DECLARE_PRIVATE`), and many libraries from before `std::pmr` use it.
There is no language support, so you write the 5 lines of 9.5 by hand every time.

## 9.9 Try it yourself

Before you solve the exercise, compile this one file and **predict the output** before you run it.

```cpp
#include <cstdio>
#include <memory>
#include <string>

// ── the class hierarchy for implementations ──
class Sink
{
public:
  virtual ~Sink() = default;
  virtual void put_line(const std::string & line) = 0;
};

class StdoutSink : public Sink
{
public:
  void put_line(const std::string & line) override { std::printf("[out] %s\n", line.c_str()); }
};

class NumberedSink : public Sink
{
public:
  void put_line(const std::string & line) override
  {
    std::printf("[out] %d: %s\n", n_++, line.c_str());
  }

private:
  int n_ = 0;
};

// ── the class hierarchy for functions ──
class View
{
public:
  explicit View(std::unique_ptr<Sink> sink) : sink_(std::move(sink)) {}
  virtual ~View() = default;
  void show(const std::string & text) { sink_->put_line(text); }

protected:
  Sink & sink() { return *sink_; }

private:
  std::unique_ptr<Sink> sink_;
};

class RepeatView : public View
{
public:
  RepeatView(std::unique_ptr<Sink> sink, int times) : View(std::move(sink)), times_(times) {}
  void show_repeat(const std::string & text)
  {
    for (int i = 0; i < times_; ++i) {
      sink().put_line(text);
    }
  }

private:
  int times_;
};

int main()
{
  View a{std::make_unique<StdoutSink>()};
  a.show("v=1");

  View b{std::make_unique<NumberedSink>()};
  b.show("v=1");

  RepeatView c{std::make_unique<StdoutSink>(), 2};
  c.show_repeat("v=1");

  RepeatView d{std::make_unique<NumberedSink>(), 2};
  d.show_repeat("v=1");

  std::printf("sizeof(View)=%zu sizeof(RepeatView)=%zu\n", sizeof(View), sizeof(RepeatView));
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: what is <code>sizeof(View)</code>? What is the difference from <code>RepeatView</code>?</summary>

```
[out] v=1
[out] 0: v=1
[out] v=1
[out] v=1
[out] 0: v=1
[out] 1: v=1
sizeof(View)=16 sizeof(RepeatView)=24
```

`View` is 16 bytes because it is **vptr 8 + `unique_ptr` 8**.
As soon as you write `virtual ~View()`, one vptr is added.
`RepeatView` adds `int times_` to that, and alignment makes it 24.

**The size of `StdoutSink` is not included in `sizeof(View)`.**
The implementation is in another place (the heap), and the function side holds only one pointer.
This is the real form of the "bridge" in Bridge.

We pass `n_++` to `%d` in `std::printf`, but this is an `int`, so there is no problem.
If you print a `std::size_t` with `%d`, it is undefined behavior,
so `sizeof` uses `%zu`.
</details>

## 9.10 Conclusion for microcontrollers

**Pimpl cannot be used as it is.** This is because `std::make_unique<Impl>()` is a heap allocation.
Of course not inside a loop, and even at startup, you cannot write it in a project with a "no heap at all" policy.

There are 3 options.

### (a) Just write it in the header (first choice)

Microcontroller firmware **is not shipped as a shared library. You do not need ABI compatibility.**
There are at most a few dozen files to build, and a full build takes a few seconds.
**All 3 benefits of Pimpl are gone.** There is no reason to hide.

```cpp
class LinkStats
{
public:
  void add_sample(float latency_ms);
  float max() const;

private:
  float max_ = 0.0F;      // fine to show in the header
  std::uint16_t count_ = 0;
};
```

**Consider this first.** "I want to hide it" is not a goal but a means.

### (b) fast-pimpl (placement new into fixed-size storage)

If you really want to hide it, construct it in a **fixed buffer inside the object** instead of the heap.

```cpp
// link_stats.hpp
#include <cstddef>
#include <new>

class LinkStats
{
public:
  LinkStats();
  ~LinkStats();
  LinkStats(const LinkStats &) = delete;
  LinkStats & operator=(const LinkStats &) = delete;

  void add_sample(float latency_ms);
  float max() const;

private:
  struct Impl;

  // fixed-size storage. Does not use the heap.
  static constexpr std::size_t kImplSize = 32;
  static constexpr std::size_t kImplAlign = 4;
  alignas(kImplAlign) unsigned char storage_[kImplSize];

  Impl * impl();
  const Impl * impl() const;
};
```

```cpp
// link_stats.cpp
#include "link_stats.hpp"

struct LinkStats::Impl
{
  float max = 0.0F;
  unsigned count = 0;
};

LinkStats::Impl * LinkStats::impl() { return reinterpret_cast<Impl *>(storage_); }
const LinkStats::Impl * LinkStats::impl() const { return reinterpret_cast<const Impl *>(storage_); }

LinkStats::LinkStats()
{
  // Check **at compile time** that the size and alignment match.
  // If you make Impl bigger, this fails here and you notice.
  static_assert(sizeof(Impl) <= kImplSize, "Increase kImplSize");
  static_assert(alignof(Impl) <= kImplAlign, "Increase kImplAlign");
  new (storage_) Impl();   // placement new. Does not use the heap
}

LinkStats::~LinkStats() { impl()->~Impl(); }   // call the destructor explicitly

void LinkStats::add_sample(float latency_ms)
{
  if (latency_ms > impl()->max) {
    impl()->max = latency_ms;
  }
  ++impl()->count;
}

float LinkStats::max() const { return impl()->max; }
```

It also builds with `-fno-exceptions -fno-rtti` (I measured it. `sizeof(LinkStats)` is 32).

There are **2 costs**.

- You now **manage `kImplSize` by hand**. If you make `Impl` bigger,
  `static_assert` fails so you notice, but a human has to fix it
- **A fixed `sizeof` now appears in the header**, so the effect of protecting the ABI is gone
  (a microcontroller does not need it anyway, so it is not a problem)

You still have to put the destructor in the .cpp, as with Pimpl. This is because `Impl` is an incomplete type.

### (c) Do the 2-axis separation of Bridge with templates

**If you only want the "separate function and implementation" part of Bridge, you need no virtual functions and no heap.**
Take the implementation as a template argument.

```cpp
// implementation (decided at compile time)
class Uart1Sink
{
public:
  void put_line(const char * line) { hal_uart_write(1, line); }   // send to UART1
};

class NullSink
{
public:
  void put_line(const char *) {}    // kill the output. Can also be used for tests
};

// function (takes the implementation as a template argument)
template <typename SinkT>
class TelemetryView
{
public:
  void show(const char * text) { sink_.put_line(text); }

protected:
  SinkT sink_;
};

template <typename SinkT>
class RepeatView : public TelemetryView<SinkT>
{
public:
  explicit RepeatView(int times) : times_(times) {}
  void show_repeat(const char * text)
  {
    for (int i = 0; i < times_; ++i) {
      this->sink_.put_line(text);   // this-> is needed because it is a dependent name
    }
  }

private:
  int times_;
};
```

```cpp
TelemetryView<Uart1Sink> view;
view.show("v=1");

TelemetryView<NullSink> off;
off.show("vanishes");          // the whole call is optimized away
```

- **Zero vtable, zero heap, zero virtual calls.** Everything is inlined
- Both `sizeof(TelemetryView<Uart1Sink>)` and `sizeof(TelemetryView<NullSink>)` are **1**
  (they are empty classes, so I measured 1 byte). They do not even hold a pointer
- In the `NullSink` version, `put_line` is empty, so **the whole call disappears**

The cost is that "you cannot switch the implementation at run time".
**On a microcontroller, the implementation is almost always decided when you flash the board.** You do not need to switch.

If you forget the `this->` in `this->sink_`, you get
`error: use of undeclared identifier 'sink_'`.
When the base class depends on a template argument, C++ does not look up members of the base automatically.
You will always step on this when you write Bridge with templates.

**Order of priority on microcontrollers**: (c) templates → (a) write it in the header → (b) fast-pimpl.
Use the `unique_ptr` version of Bridge **only when you really must switch the implementation at run time**.

## 9.11 Conclusion for ROS 2 (supplement)

rclcpp uses Pimpl a lot. Look at `rclcpp::Node`.
It delegates to `shared_ptr` members such as `NodeBaseInterface` / `NodeTopicsInterface`,
so the contents cannot be seen from the header.

The reason is exactly 9.6. **rclcpp is shipped as a shared library**,
so if the class size in the header changes, users have to rebuild.

`rclcpp::Publisher` / `Subscription` are close to Bridge.
The function side, "deliver a message", and the implementation side, rmw (the DDS implementation), are separate,
and **the same `publish()` works with Fast DDS and with Cyclone DDS**.
The choice of implementation is made at run time by the `RMW_IMPLEMENTATION` environment variable.
This is a typical example of removing the M×N of 9.3.

You will almost never need Pimpl when you write your own node.
**Putting Pimpl in a header that is used only inside the package means paying only the costs of 9.6.**

## 9.12 Common pitfalls

| Symptom | Cause |
| --- | --- |
| `error: invalid application of 'sizeof' to an incomplete type 'X::Impl'` | The destructor is defined in the header (or you forgot to declare it). Only the declaration goes in the header, and `= default` goes in the .cpp |
| `static assertion failed: can't delete an incomplete type` | Same as above (the GCC wording) |
| You tried to put a Pimpl class in a `std::vector` and got "cannot be moved" | You declared a destructor, so move is not generated implicitly. Add the 2 lines for move too |
| Only the move assignment was written in the header, and you got an error again | Move assignment deletes the old `impl_`. This also goes in the .cpp |
| It crashes at `impl_->...` | You forgot `make_unique<Impl>()` in the constructor. Or you are using an object that was already moved from |
| You released through a base pointer of the function side and members leaked | The function side has no virtual destructor. In Bridge, the function side is also inherited |
| The classes on the function side grew to `M×N` | You are expressing the implementation by inheritance. Change it to a member (the bridge) |
| `error: use of undeclared identifier 'sink_'` (template version) | Members of a dependent base are written as `this->sink_` |
| You passed a `unique_ptr` to the constructor and got a compile error | You forgot `std::move`. `unique_ptr` cannot be copied |
| You used Pimpl but the build did not get faster | The header side still includes `<vector>` and so on. Move the includes to the .cpp |

## 9.13 Matching exercise

```bash
./drill run dp09
```

You implement **Bridge** in `exercises/dp09_bridge/src/telemetry_view.cpp`.

1. `RecordingSink` / `NumberedSink`: the class hierarchy for implementations
2. `TelemetryView::show()`: the class hierarchy for functions. `open()` → `put_line()` → `close()`
3. `RepeatView::show_repeat()`: add one function. **Do not change the implementation side**

You implement **Pimpl** in `exercises/dp09_bridge/src/link_stats.cpp`.

4. `struct LinkStats::Impl` and the constructor
5. `add_sample()` / `count()` / `mean()` / `max()`

The tests check the following 4 points.

- **All 4 combinations work**: 2 functions × 2 implementations
- Even if you swap the implementation, **the function that calls the function side stays the same single one** (`run_show`)
- `sizeof(LinkStats)` is **the size of one pointer** (`static_assert`)
- `LinkStats` is **movable and not copyable** (`static_assert`)

The definitions of `~LinkStats()` and move are already written for you. **Do not delete them.**
What happens if you delete them is what you confirmed in 9.5.

## 9.14 Summary of this chapter

- The structure of Bridge is almost the same as the Java version. The difference is that **the function side also needs a virtual destructor**
- The bridge to the implementation is `std::unique_ptr<Impl>`. **A member, not inheritance**. That is why M×N becomes M+N
- **Bridge and Pimpl have the same structure. The purpose is different.** Virtual if you want to swap, non-virtual if you only want to hide
- If you hold `unique_ptr<incomplete type>`, **you cannot write the destructor in the header.**
  Only the declaration goes in the header, and `= default` goes in the .cpp
- If you declare a destructor, **move is no longer generated implicitly**. Add 2 lines for move too
- The value of Pimpl is **fewer header dependencies**. For a shared library, it also protects the ABI.
  The cost is one heap allocation and one level of indirection
- On microcontrollers, consider **(c) templates → (a) write it in the header → (b) fast-pimpl** in this order.
  The `unique_ptr` version is only "when you must switch at run time"
- **If there is only one kind of implementation, do not add Bridge**

---

Previous: [8. Abstract Factory](08_AbstractFactory.md) / Next: 10. Strategy (coming soon)
