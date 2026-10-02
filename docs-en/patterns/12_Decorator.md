# 12. Decorator

> **Matches chapter 12 of Hiroshi Yuki's *Learning Design Patterns in Java*.** Keep `Display` / `Border` / `SideBorder` / `FullBorder` open next to you.
>
> **Goal of this chapter**: The `Border` of the Java version has just one field, `Display display;`.
> In C++, this one line needs a decision: **"do I own the contents, or only borrow them?"**
> If you own, use `std::unique_ptr<Display>`. If you borrow, use `Display &`.
> If you choose the former, assembling becomes a **chain of moves**, and the way of writing is quite different from Java.
> And if you forget the virtual destructor, **the whole inside of the nesting is not released.**

## 12.1 Porting the Java version to C++ as is

Yuki's `Border` looks like this.

```java
public abstract class Border extends Display {
    protected Display display;
    protected Border(Display display) {
        this.display = display;
    }
}
```

If you move it to C++ in a straightforward way, it looks like this.

```cpp
class LogSink
{
public:
  virtual ~LogSink() = default;
  virtual std::string format(const std::string & message) const = 0;
};

class SinkDecorator : public LogSink
{
public:
  explicit SinkDecorator(std::unique_ptr<LogSink> inner)
  : inner_(std::move(inner))
  {
  }

protected:
  const LogSink & inner() const { return *inner_; }

private:
  std::unique_ptr<LogSink> inner_;
};
```

There are 3 changes from the Java version.

### Change 1: `Display display;` became `std::unique_ptr<LogSink> inner_;`

`Display display;` in Java is a **reference.** You do not need to write who releases it.
If you write `LogSink inner_;` in C++, it is a **value**, and since this is an abstract class, it does not even compile.
Even if it were a concrete type, the moment you assign to it, it would slice and the derived part would disappear.

There are 3 choices.

| Way of writing | Meaning | Who releases it |
| --- | --- | --- |
| `std::unique_ptr<LogSink> inner_;` | **Owns** the contents | This decorator |
| `LogSink & inner_;` | **Borrows** the contents | The caller |
| `std::shared_ptr<LogSink> inner_;` | **Shares** the contents | The last one |

**This chapter writes it with number 1.** For Decorator, it is natural that "once you wrap it, the wrapper takes care of the contents".
If you throw away one outermost object, the whole nesting disappears.

Number 2 (hold by reference) can also be written. But,

```cpp
std::unique_ptr<LogSink> build()
{
  PlainMessage plain;              // local variable
  return std::make_unique<LevelTag>(plain, "INFO");   // with the hold-by-reference version you can write this
}                                  // plain dies here. The returned object points to dead contents
```

**The responsibility to protect the lifetime remains with the caller.** Nothing is written in the type.
This is the same problem as the iterator in chapter 1. In this course, when in doubt, lean toward the owning side.

### Change 2: Made `protected Display display;` a `private` member + `protected` accessor

The Java version makes `display` `protected` and lets derived classes touch it directly. You can write that in C++ too, but
a `protected` member variable can be rewritten by all derived classes,
so there is room for `inner_` to be swapped or set to `nullptr`.

Make **the member variable `private`, and if needed a `protected` const accessor.**
All that the derived classes want is "to read the contents".

### Change 3: Added `const` to `format()`

Formatting does not change state. Java has no such distinction, so you forget to write it.
If you add `const`, you can also format something received as `const LogSink &`.

## 12.2 Who owns it? The constructor takes by value and uses `std::move`

For a constructor that receives a `unique_ptr`, this is practically the only way to write it.

```cpp
explicit SinkDecorator(std::unique_ptr<LogSink> inner)   // take by value
: inner_(std::move(inner))                               // move into the member
{
}
```

The key is to **take by value.** `unique_ptr` cannot be copied, so
the caller can only `std::move` or pass a temporary object.
So **the message "please hand over the ownership" is written in the type of the argument.**

If you write `inner_(inner)` in the initializer list, it is a compile error.

```
error: call to implicitly-deleted copy constructor of 'std::unique_ptr<LogSink>'
```

Receiving it as `const std::unique_ptr<LogSink> &` is **wrong.**
If you only borrow, there is no point in going through `unique_ptr` (`LogSink &` is enough).

## 12.3 Assembling the nesting becomes a chain of moves

If you choose the owning design, assembling looks like this.

```cpp
auto sink = std::make_unique<TimestampTag>(
              std::make_unique<LevelTag>(
                std::make_unique<PlainMessage>(), "INFO"), "12:00:00");
```

The structure is the same as the Java version

```java
Display d = new SideBorder(new FullBorder(new StringDisplay("Hello")), '*');
```

but `std::make_unique<...>` gets in every time, so **you cannot read it.**
It is already like this with 3 levels.

The fix is easy: **make the wrapping operation a function.**

```cpp
std::unique_ptr<LogSink> with_level(std::unique_ptr<LogSink> inner, std::string level)
{
  return std::make_unique<LevelTag>(std::move(inner), std::move(level));
}
```

The caller looks like this.

```cpp
auto sink = with_level(with_timestamp(plain(), "12:00:00"), "INFO");
```

**Everything passing through is a move.** The `unique_ptr` goes in by value, comes out by value,
and in the end only the one outermost `unique_ptr` is left in your hand.
If you throw away this one, the whole nesting disappears.

## 12.4 Decorator is a special form of Composite

Compare it with the `Composite` of chapter 11.

| | Composite | Decorator |
| --- | --- | --- |
| Number of children | 0 or more (`std::vector<std::unique_ptr<Component>>`) | **Exactly 1** (`std::unique_ptr<Component>`) |
| Purpose | Treat the whole and the parts the same | Add functions later |
| Recursion | Tree | A single chain |

**Structurally, Decorator is "a Composite with one child".**
So the handling of ownership is the same too, and the `vector` just became one `unique_ptr`.
If you were able to write the `vector` of `unique_ptr` in chapter 11, there is almost nothing new here.

Conversely, **where you got stuck in chapter 11, you will get stuck at the same places here.**
For example, it cannot be copied, and `push_back` needs `std::move`.

## 12.5 A C++-specific danger: without a virtual destructor, the inside is not released

I actually tried what happens if you delete `virtual ~LogSink() = default;` from the base class.

```cpp
class Sink
{
public:
  ~Sink() {}                                   // forgot to write virtual
  virtual std::string format(const std::string & m) const = 0;
};
```

Apple clang warns at compile time (`-Wall -Wextra -Wpedantic`).

```
warning: delete called on 'Sink' that is abstract but has non-virtual destructor
         [-Wdelete-abstract-non-virtual-dtor]
warning: delete called on non-final 'Border' that has virtual functions but
         non-virtual destructor [-Wdelete-non-abstract-non-virtual-dtor]
```

And when I ran it on my machine, it crashed with exit code 133 without even printing the output of `format()`.
**It is undefined behavior, so what happens depends on the environment.**
If you are lucky it crashes, and if you are unlucky it silently keeps leaking only the inside.

The reasoning is this. The outer object is held by `std::unique_ptr<Sink>`,
so only `~Sink()` is called at release. `~Border()` is not called
= **the destructor of `inner_` (the whole inside), a member of `Border`, does not run.**
The deeper the nesting, the more is leaked all at once.

The nesting is the main body of Decorator, so **the damage of forgetting the virtual destructor is the biggest in this pattern.**

Let us also look at the order of destruction when it is written correctly (this is the measurement of 12.6).

```
~Border([INFO])
~Border(12:00:00)
~Plain
```

They are called in order **from the outside to the inside.** The body of the outer destructor runs first,
and after that the members (`inner_`) are destroyed.

## 12.6 Is there something in the standard library / language that does the same?

**Yes. And in a central place.**

- **`std::istream` / `std::ostream` and `std::streambuf`**
  Streams separate "formatting of strings" and "the actual input/output destination".
  `std::ofstream` and `std::ostringstream` have the same face as `std::ostream`,
  and only the `streambuf` inside is different. Putting your own `streambuf` in between
  to add processing such as "add a timestamp at the start of a line" is exactly Decorator.
- **The deleter of `std::unique_ptr`**
  You can also see it as a compile-time Decorator that swaps the behavior with a type parameter.
- **The views of `<ranges>` in C++20**
  `v | std::views::filter(...) | std::views::transform(...)` is a Decorator that
  wraps a range in a range. **This course is C++17, so we do not use it**,
  but the shape "when you wrap it, the function is added with the same interface" is the same.

So **Decorator is a pattern you relatively rarely write yourself in C++.**
When you think "I want to add a function with the same interface",
first think whether `streambuf` or templates are enough (→ [0. Before you use them](00_before_you_use_them.md)).

## 12.7 Try it yourself

It is complete in one file. Run it **after predicting the output.**

```cpp
#include <iostream>
#include <memory>
#include <string>
#include <utility>

class Sink
{
public:
  virtual ~Sink() = default;
  virtual std::string format(const std::string & m) const = 0;
};

class Plain : public Sink
{
public:
  ~Plain() override { std::cout << "~Plain\n"; }
  std::string format(const std::string & m) const override { return m; }
};

class Border : public Sink
{
public:
  Border(std::unique_ptr<Sink> inner, std::string tag)
  : inner_(std::move(inner)), tag_(std::move(tag))
  {
  }
  ~Border() override { std::cout << "~Border(" << tag_ << ")\n"; }

  std::string format(const std::string & m) const override
  {
    return tag_ + " " + inner_->format(m);
  }

private:
  std::unique_ptr<Sink> inner_;
  std::string tag_;
};

std::unique_ptr<Sink> wrap(std::unique_ptr<Sink> inner, std::string tag)
{
  return std::make_unique<Border>(std::move(inner), std::move(tag));
}

int main()
{
  {
    auto a = std::make_unique<Border>(
      std::make_unique<Border>(std::make_unique<Plain>(), "[INFO]"), "12:00:00");
    std::cout << a->format("moving") << "\n";
  }
  std::cout << "----\n";
  {
    auto b = wrap(wrap(std::make_unique<Plain>(), "12:00:00"), "[INFO]");
    std::cout << b->format("moving") << "\n";
  }
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: are the formatted results of the 2 blocks the same? How many destructors are called?</summary>

```
12:00:00 [INFO] moving
~Border(12:00:00)
~Border([INFO])
~Plain
----
[INFO] 12:00:00 moving
~Border([INFO])
~Border(12:00:00)
~Plain
```

- **The formatted results are different.** The order of wrapping is reversed, so the order of the tags is also reversed.
  This is the essence of Decorator: even with the same parts, **the order of assembly decides the result**
- 3 destructors are called in each block. Throwing away just one `unique_ptr`
  reaches all the way to the inside. The order is **from the outside to the inside**
- The lower block uses `wrap()` to remove the nesting of `make_unique`.
  The order of the arguments can be read as "from the inside to the outside"

</details>

## 12.8 Conclusion for microcontrollers

**You cannot use nested `unique_ptr`.** A heap allocation runs for every wrap.
Building it once at startup and using it forever might be fine, but
if you write code that reassembles depending on the situation inside a loop, the heap will fragment and the allocation will eventually fail.

Instead, **nest by type.** It is `Border<Border<Text>>`.

```cpp
#include <cstdio>
#include <cstring>
#include <type_traits>

// The output destination. Only a fixed-length buffer. No dynamic allocation.
class Buffer
{
public:
  void put(const char * text)
  {
    const std::size_t n = std::strlen(text);
    if (used_ + n >= sizeof(data_)) {
      return;                       // if it overflows, discard. Do not throw
    }
    std::memcpy(data_ + used_, text, n);
    used_ += n;
    data_[used_] = '\0';
  }

  const char * c_str() const { return data_; }

private:
  char data_[128] = {};
  std::size_t used_ = 0;
};

// Component. No virtual functions.
class PlainMessage
{
public:
  void emit(Buffer & out, const char * message) const { out.put(message); }
};

// Decorator. Holds the contents "by value". Nests by type.
template <typename Inner>
class TagDecorator
{
public:
  constexpr TagDecorator(Inner inner, const char * tag)
  : inner_(inner), tag_(tag)
  {
  }

  void emit(Buffer & out, const char * message) const
  {
    out.put(tag_);
    out.put(" ");
    inner_.emit(out, message);      // not a virtual call. It is inlined
  }

private:
  Inner inner_;
  const char * tag_;
};

template <typename Inner>
constexpr TagDecorator<Inner> with_tag(Inner inner, const char * tag)
{
  return TagDecorator<Inner>(inner, tag);
}

int main()
{
  const auto sink = with_tag(with_tag(PlainMessage{}, "12:00:00.000"), "[INFO]");

  static_assert(!std::is_polymorphic<decltype(sink)>::value, "there must be no vtable");
  // The contents are only the 2 tag pointers and 1 byte of padding for the empty class PlainMessage.
  static_assert(sizeof(sink) <= 3 * sizeof(const char *), "something extra is in there");

  Buffer out;
  sink.emit(out, "moving");
  std::printf("%s\n", out.c_str());
  std::printf("sizeof(sink) = %zu\n", sizeof(sink));
  return 0;
}
```

I built it with settings close to a microcontroller and ran it.

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti micro.cpp -o micro && ./micro
```

```
[INFO] 12:00:00.000 moving
sizeof(sink) = 24
```

**What you can confirm**

- It passes with `-fno-exceptions -fno-rtti` with zero warnings and zero errors
- `std::is_polymorphic` is `false` = **there is no vtable.** It uses neither ROM nor RAM
- Zero heap allocations. `sink` fits entirely on the stack (24 bytes in this example)
- `inner_.emit(...)` is not a virtual call, so the optimizer can inline it

There are **2 costs.**

1. **You cannot reassemble at run time.** The type holds the order of assembly,
   so you cannot "change the wrapping order depending on the contents of a config file"
2. Code is generated for each combination. If you nest 3 kinds of decorators 3 levels deep,
   the number of real `emit()` functions grows by the number of combinations you used

First check whether you really need to reassemble at run time.
For club log formatting, **it is almost always decided at build time.**

## 12.9 Conclusion for ROS 2 (supplement)

On Linux, nested `unique_ptr` is fine. You only build it once at startup.

rclcpp has no GoF-style Decorator class, but
you can write your own, such as wrapping `rclcpp::PublisherBase` to collect statistics, without trouble.
However, the ROS 2 log (`RCLCPP_INFO`) already adds the time, node name, and severity on the macro side, so
**you do not need to rebuild that with a Decorator.**

## 12.10 Common pitfalls

| Symptom | Cause |
| --- | --- |
| `error: call to implicitly-deleted copy constructor of 'std::unique_ptr<...>'` | You wrote `inner_(inner)` in the initializer list. Make it `std::move(inner)` |
| `warning: delete called on ... non-virtual destructor` | The base has no `virtual ~LogSink() = default;` |
| You threw away the outside but the destructor of the inside is not called | Same as above. The whole nesting is leaking |
| The order of the tags is the opposite of what you expected | The order in which `format()` calls the contents. **Let the contents format first**, then wrap |
| You swapped the contents and got a double free | You passed a `unique_ptr` through a raw pointer. Move ownership only with `std::move` |
| You cannot read the nesting of `make_unique` | Write `with_xxx()` helpers (12.3) |
| You made the hold-by-reference version and it crashed at run time | The contents are a local variable and died before the outside |
| In the template version, `sizeof` is bigger than you expected | An empty class also takes at least 1 byte if held as a member, and is padded by alignment |

## 12.11 Matching exercise

```bash
./drill run dp12
```

In `exercises/dp12_decorator/src/log_sink.cpp` you implement

1. `join_tag()`: join the tag and the body (shared by the `unique_ptr` version and the template version)
2. `PlainMessage`: the innermost one. Records destruction in its destructor
3. `SinkDecorator`: receives the contents with `std::move` and owns them
4. `LevelTag` / `TimestampTag` / `SourceTag`: the 3 kinds of decorators
5. `with_level()` / `with_timestamp()` / `with_source()`: the assembling helpers

The tests check that **the output changes if you change the wrapping order**,
that **throwing away one outer object destroys the inside too** (we compare with the record of the destructors),
and that **the template version returns the same output and `std::is_polymorphic_v` is `false`.**

## 12.12 Summary of this chapter

- Java's `Display display;` becomes, in C++, a choice of **own (`unique_ptr`) / borrow (`&`) / share (`shared_ptr`)**
- The owning version of Decorator is the natural one. **If you throw away one outer object, the whole nesting disappears**
- The constructor **takes `std::unique_ptr<T>` by value and uses `std::move`.** The transfer of ownership is written in the type
- Assembling is a **chain of moves.** The nesting of `make_unique` cannot be read, so make wrapping functions
- **Decorator is a Composite with one child** (chapter 11). The handling of ownership is the same
- **If you forget the virtual destructor, the whole inside of the nesting is not released.** The damage is the biggest in this pattern
- In the standard library, `streambuf` and (C++20) `views` are Decorators. Look for them before you write your own
- On microcontrollers, do not use nested `unique_ptr`. **Nest by type with templates** and you get zero heap and zero vtable

---

Previous: [11. Composite](11_Composite.md) / Next: [13. Visitor](13_Visitor.md)
