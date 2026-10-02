# 4. Factory Method

> **Corresponds to Yuki's book, Chapter 4.** Please open `Factory` / `Product` and `IDCardFactory` / `IDCard` next to you.
>
> **Goal of this chapter**: The Java `createProduct()` just returns `new IDCard(owner)`.
> If you write the same thing in C++, the type does not say **"who `delete`s the thing that came back"**.
> The answer is to return `std::unique_ptr<Product>`. You can accept this quickly.
> The main topic is what comes after: a pitfall specific to C++, that **covariant return types do not work with `unique_ptr`**, and
> **how to write creation on a microcontroller where you cannot use the heap**.
> And one more thing. We first decide **the cases where you should not add a Factory at all**.

If you have just read chapter 3, this structure should look familiar.
**Factory Method is a kind of Template Method.** Yuki's book also places it that way.

| | Template Method (chapter 3) | Factory Method (chapter 4) |
| --- | --- | --- |
| What the base class decides | **The procedure of the processing** | **The procedure of creation** |
| What it leaves to the derived class | The contents of each step | **What to create** |
| The topic in C++ | NVI and virtual destructors | **Handing over ownership** |

## 4.1 If you write the Java version in C++ as is

`Factory` in Yuki's book looks like this.

```java
public abstract class Factory {
    public final Product create(String owner) {
        Product p = createProduct(owner);
        registerProduct(p);
        return p;
    }
    protected abstract Product createProduct(String owner);
    protected abstract void registerProduct(Product product);
}
```

If you move it to C++ in the plain way, it looks like this.

```cpp
class Factory
{
public:
  virtual ~Factory() = default;

  // The procedure is fixed. Do not write virtual (= same as Java's final)
  std::unique_ptr<Product> create(const std::string & owner);

private:
  virtual std::unique_ptr<Product> create_product(const std::string & owner) = 0;
  virtual void register_product(const Product & product) = 0;
};
```

There are four changes from the Java version.

### Change 1: Added `virtual ~Factory() = default;`

This is the same as chapter 1 and chapter 3. **If you write even one pure virtual function, add a virtual destructor.**
We follow this in all 23 chapters.

### Change 2: Removed `final` and made it `private virtual`

`public final void create()` → `public` and no `virtual` has the same meaning.
`protected abstract` → **`private virtual`**. The reason is the same as NVI in chapter 3:
"it does not need to be called from the derived class, it only needs to be overridden".

### Change 3: Changed `Product` to `std::unique_ptr<Product>`

**This is the main topic of this chapter.** We cover it in 4.2.

### Change 4: Changed `registerProduct(Product p)` to `register_product(const Product & product)`

All Java arguments are references, so even if you write `registerProduct(p)`,
**the same object is passed**. If you write `void register_product(Product product)` in C++,
a copy happens (and if `Product` is an abstract class, it is a compile error in the first place).

And by making it `const Product &`, the type now says that **"the registering side does not receive ownership"**.
This is the difference from the form that passes `std::unique_ptr<Product>`.

| How to write the argument | Meaning |
| --- | --- |
| `const Product & p` | **Only looks. Does not own** (this chapter uses this) |
| `std::unique_ptr<Product> p` | **Receives ownership**. It disappears from the caller's hand |
| `std::shared_ptr<Product> p` | Shares ownership |
| `Product * p` | **Unclear. In C++, a design flaw** |

## 4.2 Who owns it?

The Java version of `createProduct()` looks like this.

```java
protected Product createProduct(String owner) {
    return new IDCard(owner);
}
```

It does `new`, returns, and that is all. The GC cleans up. If you write the same form in C++,

```cpp
Product * create_product(const std::string & owner);   // does the caller delete it?
```

**The type does not say who `delete`s it.** Even if you write it in documentation, nobody reads it.
A person who receives the return value with `auto` does not even notice that a pointer is in there.

```cpp
auto card = factory.create("motor");   // it was a Product *. I forgot to delete and it leaked
```

If you return `std::unique_ptr`, you cannot cause this accident.

```cpp
std::unique_ptr<Product> create(const std::string & owner);
```

Now the type says: **"the receiver is the only owner. It disappears by itself when it leaves the scope"**.

```cpp
{
  auto card = factory.create("motor");   // std::unique_ptr<Product>
  card->use();
}                                        // released automatically here
```

**Not only for Factory Method, the principle is: "where Java does `new` and returns, C++ returns
`std::unique_ptr`".** It is the same decision as `iterator()` in chapter 1.

The details of ownership are in [C++ 6. Smart pointers](../cpp/06_smart_pointers.md).
We do not repeat them here.

### Do not write `std::move` in `return logger;`

The end of `create()` looks like this.

```cpp
std::unique_ptr<Product> Factory::create(const std::string & owner)
{
  std::unique_ptr<Product> product = create_product(owner);
  if (product == nullptr) {
    return nullptr;
  }
  register_product(*product);
  return product;        // do not write std::move
}
```

`product` is a local variable, so it is **moved automatically** at `return`.
If you write `return std::move(product);`, it gets in the way of the compiler optimization (NRVO), and
you may get a warning from `-Wpessimizing-move` / `-Wredundant-move`.

### Why we use `std::make_unique`

```cpp
return std::make_unique<IDCard>(owner);              // this one
return std::unique_ptr<Product>(new IDCard(owner));  // works, but do not write it
```

1. You write the type only once
2. `new` disappears from the code. **You structurally no longer need to look for `delete`**
3. Exception safe (no leak even if an exception is thrown while evaluating the arguments)

## 4.3 Dangers specific to C++

### Danger 1: Covariant return types do not work with `unique_ptr`

C++ has a rule called the **covariant return type**.
A virtual function that returns `Base *` in the base can be narrowed to return `Derived *` in the derived class.

```cpp
struct Creator
{
  virtual Product * create_raw() = 0;
};
struct FileCreator : Creator
{
  FileProduct * create_raw() override { return new FileProduct(); }   // compiles
};
```

**The moment you use `unique_ptr`, you cannot use this.**

```cpp
struct Creator
{
  virtual std::unique_ptr<Product> create() = 0;
};
struct FileCreator : Creator
{
  std::unique_ptr<FileProduct> create() override { ... }   // does not compile
};
```

If you actually compile it, you get this (`g++ -std=c++17 -Wall -Wextra -Wpedantic`,
Apple clang 21).

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// covariant.cpp
#include <memory>

struct Product
{
  virtual ~Product() = default;
};

struct FileProduct : Product
{
};

struct Creator
{
  virtual ~Creator() = default;
  virtual std::unique_ptr<Product> create() = 0;
};

struct FileCreator : Creator
{
  // tried to narrow the return type to unique_ptr<FileProduct>
  std::unique_ptr<FileProduct> create() override { return std::make_unique<FileProduct>(); }
};

int main()
{
  FileCreator creator;
  return 0;
}
```

```bash
clang++ -std=c++17 -Wall -Wextra -Wpedantic covariant.cpp -o covariant
```

</details>

```
covariant.cpp:22:32: error: virtual function 'create' has a different return type ('unique_ptr<FileProduct>') than the function it overrides (which has return type 'unique_ptr<Product>')
   22 |   std::unique_ptr<FileProduct> create() override { return std::make_unique<FileProduct>(); }
      |   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^
covariant.cpp:16:36: note: overridden virtual function is here
   16 |   virtual std::unique_ptr<Product> create() = 0;
      |           ~~~~~~~~~~~~~~~~~~~~~~~~ ^
1 error generated.
```

**Reason**: The standard says covariance is allowed only for "pointers or references".
`std::unique_ptr<Derived>` and `std::unique_ptr<Base>` are **two completely unrelated classes**,
with no inheritance relation. The derived relation exists between the inner `Derived` and `Base`,
not between the `unique_ptr`s that wrap them.

**Conclusion**: **Write the return type of a factory method as `std::unique_ptr<Base>` in the derived class too.**
In chapter 6 (Prototype), you hit the same wall again with `clone()`.

### Danger 2: The conversion is one-way

Even if you fix the return type to `unique_ptr<Base>`, doing `make_unique<Derived>()` inside compiles.

```cpp
std::unique_ptr<Product> create_product() override
{
  return std::make_unique<IDCard>(owner);   // unique_ptr<IDCard> → unique_ptr<Product>
}
```

The **implicit conversion** from `unique_ptr<Derived>` to `unique_ptr<Base>` is allowed.
Because of this, even if covariance does not work, you have almost no trouble in practice.

**The reverse is not possible.**

```cpp
std::unique_ptr<Base> b = std::make_unique<Derived>();   // compiles
std::unique_ptr<Derived> d = std::move(b);               // does not compile
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// conv.cpp
#include <memory>
struct Base { virtual ~Base() = default; }; struct Derived : Base {};
int main()
{
  std::unique_ptr<Base> b = std::make_unique<Derived>();   // compiles
  std::unique_ptr<Derived> d = std::move(b);               // does not compile
  return 0;
}
```

```bash
clang++ -std=c++17 -Wall -Wextra -Wpedantic conv.cpp -o conv
```

</details>

```
conv.cpp:7:28: error: no viable conversion from '__libcpp_remove_reference_t<std::unique_ptr<Base, std::default_delete<Base>> &>' (aka 'std::unique_ptr<Base>') to 'std::unique_ptr<Derived>'
    7 |   std::unique_ptr<Derived> d = std::move(b);               // does not compile
      |                            ^   ~~~~~~~~~~~~
...
```

You may think "it is really a `Derived`, so you should be able to convert it back", but the compiler cannot know.
If you really need it, you use `dynamic_cast`, but **once you need it, suspect your design.**
The point of going through a Factory (hiding the concrete type) has disappeared.

### Danger 3: How to return when creation can fail

In Java you throw an exception or return `null`. C++ has three choices.

| Method | How to write it | Where to use it |
| --- | --- | --- |
| Return `nullptr` | Keep `std::unique_ptr<Product>` | **Microcontrollers. Environments where exceptions cannot be used** |
| Throw an exception | `throw std::runtime_error{...}` | On a PC / ROS 2, when the failure is really abnormal |
| `std::optional` | `std::optional<std::unique_ptr<Product>>` | **Do not use** (four lines below) |

`std::optional<std::unique_ptr<T>>` has **three states** (none / present but null / present and non-null),
and only adds more checks for the caller. A `unique_ptr` **can express "maybe nothing" by itself**,
so `nullptr` is enough.

In this course we **unify on returning `nullptr`**. This way you do not need to write it differently for microcontrollers and ROS 2.
**But you cannot force the caller to check for `nullptr`.**
So on the template method side (`create()`), we return early on `nullptr`, and
**the base class guarantees that "if it fails, it is not registered either"**.

### Danger 4: Calling a virtual function from a constructor

This is the same story as chapter 3, section 3.4. Even if you call `create_product()` in the constructor of `Factory`,
**the derived class version is not called** (because the derived part is not yet constructed).
If it is pure virtual, it crashes at run time. **Do not move the initialization of the factory outside `create()`.**

## 4.4 Does the standard library or a language feature already have the same thing?

`std::make_unique<T>()` and `std::make_shared<T>()` are the smallest Factories in that they
**create `T` from arguments and return ownership**. But they have no feature to "switch what to create at run time".

If you really need "switching at run time", in C++ you can **write it with functions, without making a class hierarchy**.

```cpp
using LoggerMaker = std::unique_ptr<Logger> (*)(const std::string &);   // function pointer

std::unique_ptr<Logger> make_uart(const std::string & tag);
std::unique_ptr<Logger> make_memory(const std::string & tag);

const std::map<std::string, LoggerMaker> kMakers = {
  {"uart", &make_uart},
  {"memory", &make_memory},
};
```

**In Java you could not carry a function around by itself, so you had to make a class. In C++
(and in modern Java), a function pointer / lambda / `std::function` is enough.**
You make a `Factory` class only **when there is common processing before and after creation (registration, numbering, logging)**.
If there is none, you are building an inheritance hierarchy just for one `create_product()`.

## 4.5 When you should not add a Factory

Section 0.3 "Creation becomes three steps" in [0. Before you use them](00_before_you_use_them.md) is the other side of this chapter.
This is where things break the most right after the reading group, so we draw the line first.

**If any of the following applies, do not add a Factory.**

1. **The type is decided at compile time.** `Imu sensor{0x68};` is enough.
   Is there really a case in your own code where you choose a sensor by a string at run time?
2. **There is only one ConcreteCreator.** Same as 0.1: you only added files
3. **There is no common processing before and after creation.** That is not a Factory, just a function (4.4)
4. **The created object does not need to be on the heap.** On a microcontroller this is normal (4.7)

**You may add one only when both of the following hold.**

- What you create is **decided at run time** (a configuration file, an instruction that arrived by communication, swapping in tests)
- There is **a common procedure before and after** creation (registration, numbering, guaranteeing initialization order)

`LoggerFactory` in the exercise `dp04` has the common procedure of **"register what was created with a tag"**,
so it satisfies the second condition. Without that, writing `std::make_unique<MemoryLogger>(...)` directly is the right answer.

## 4.6 Try it yourself

It is complete in one file. **Predict the output** before you run it.

```cpp
// try.cpp
#include <iostream>
#include <memory>
#include <string>

// ---- Product ----
class Logger
{
public:
  virtual ~Logger() { std::cout << "  Logger destroyed\n"; }
  virtual void write(const std::string & message) = 0;
};

class ConsoleLogger : public Logger
{
public:
  explicit ConsoleLogger(std::string tag) : tag_(std::move(tag)) {}
  void write(const std::string & message) override
  {
    std::cout << "  [" << tag_ << "] " << message << "\n";
  }

private:
  std::string tag_;
};

// ---- Creator (Template Method itself) ----
class LoggerFactory
{
public:
  virtual ~LoggerFactory() = default;

  // The procedure is fixed. Only what to create is left to the subclass
  std::unique_ptr<Logger> create(const std::string & tag)
  {
    std::unique_ptr<Logger> logger = create_logger(tag);
    if (logger == nullptr) {
      return nullptr;                 // creation failed. Do not throw an exception
    }
    ++created_count_;
    return logger;                    // ownership moves to the caller
  }

  int created_count() const { return created_count_; }

private:
  virtual std::unique_ptr<Logger> create_logger(const std::string & tag) = 0;
  int created_count_ = 0;
};

class ConsoleLoggerFactory : public LoggerFactory
{
private:
  std::unique_ptr<Logger> create_logger(const std::string & tag) override
  {
    if (tag.empty()) {
      return nullptr;
    }
    // unique_ptr<ConsoleLogger> → unique_ptr<Logger> converts implicitly
    return std::make_unique<ConsoleLogger>(tag);
  }
};

int main()
{
  ConsoleLoggerFactory factory;

  {
    auto logger = factory.create("motor");
    logger->write("duty=0.5");
    std::cout << "Leaving the scope\n";
  }

  auto ng = factory.create("");
  std::cout << "Empty tag: " << (ng == nullptr ? "nullptr" : "created") << "\n";
  std::cout << "created_count = " << factory.created_count() << "\n";
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: where does "Logger destroyed" appear? What is <code>created_count</code>?</summary>

```
  [motor] duty=0.5
Leaving the scope
  Logger destroyed
Empty tag: nullptr
created_count = 1
```

**"Logger destroyed" comes after "Leaving the scope".** `factory` does not own the created object.
The `unique_ptr` called `logger` is the only owner, and it is released when you leave the inner block.

`created_count` is 1 because **`create()` returns before counting when creation fails**.
If the design let you call `create_logger()` directly, you could not write this guarantee.
**This is where the meaning of fixing the procedure in the base class (Template Method) shows.**

Try changing the return type of `create_logger()` to `std::unique_ptr<ConsoleLogger>`.
You get the compile error from 4.3.
</details>

## 4.7 Conclusion for microcontrollers

**A Factory Method that uses the heap cannot be used as it is.**
`make_unique` is `new`. Dynamic allocation is only at startup, as a rule. It is forbidden inside loops.
The heap becomes fragmented, and allocation eventually fails.

There are three options. **Consider them in order from the top.**

### Option 1 (first choice): Do not use a Factory at all

If what you create is decided at compile time, **place it statically and hand out references**.

```cpp
static UartLogger g_uart_logger{1};      // one object in .bss. Zero allocation
Logger & logger() { return g_uart_logger; }
```

90% of microcontroller code can be done with this. You need neither `unique_ptr` nor switching of vtables.
Things where "there is only one piece of hardware" belong to chapter 5 (Singleton).

### Option 2: Fixed pool + placement new

If you really need to choose the kind at run time, **allocate only the memory statically first,
and construct in it**. You use not a single byte of heap.

```cpp
// micro.cpp
#include <cstddef>
#include <cstdio>
#include <new>
#include <type_traits>

class Logger
{
public:
  virtual ~Logger() = default;
  virtual void write(const char * message) = 0;
};

class UartLogger : public Logger
{
public:
  explicit UartLogger(int channel) : channel_(channel) {}
  void write(const char * message) override
  {
    std::printf("  UART%d: %s\n", channel_, message);
  }

private:
  int channel_;
};

class NullLogger : public Logger
{
public:
  void write(const char *) override {}
};

// Fixed pool. Does not use the heap at all
template <typename Base, std::size_t Capacity, std::size_t SlotSize>
class LoggerPool
{
public:
  template <typename Derived, typename... Args>
  Base * create(Args &&... args)
  {
    static_assert(sizeof(Derived) <= SlotSize, "slot is too small");
    static_assert(alignof(Derived) <= alignof(std::max_align_t), "not enough alignment");
    if (used_ >= Capacity) {
      return nullptr;                       // failure is nullptr. Do not throw
    }
    Base * p = new (storage_[used_]) Derived(static_cast<Args &&>(args)...);
    ++used_;
    return p;
  }

  std::size_t used() const { return used_; }

private:
  alignas(std::max_align_t) unsigned char storage_[Capacity][SlotSize] = {};
  std::size_t used_ = 0;
};

int main()
{
  LoggerPool<Logger, 2, 32> pool;

  Logger * a = pool.create<UartLogger>(1);
  Logger * b = pool.create<NullLogger>();
  Logger * c = pool.create<UartLogger>(2);   // the third one. The pool is used up

  a->write("boot ok");
  b->write("this is discarded");
  std::printf("  3rd: %s\n", c == nullptr ? "nullptr" : "created");
  std::printf("  used = %u\n", static_cast<unsigned>(pool.used()));
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -fno-exceptions micro.cpp -o micro && ./micro
```

```
  UART1: boot ok
  3rd: nullptr
  used = 2
```

Check that it builds with `-fno-exceptions`. **It uses not a single exception.**
Failure is `nullptr`.

This code **deliberately does not do** three things. These are decisions for microcontrollers.

1. **It does not return `unique_ptr`.** It returns a raw pointer.
   These objects are created at startup and used for the whole lifetime, so **they are never released** (the pool never decreases `used_`)
2. **It does not call destructors.** If you do, you write `p->~Base();` yourself.
   If you release in the middle, you need to manage free slots, and that is the job of an allocator, not a Factory
3. **It does not use `std::string`.** It uses `const char *`. This is because no allocation happens

**If you `delete` a pointer returned from the pool, it breaks** (because the memory was not obtained with `new`).
If you really want to handle it with `unique_ptr`, attach a **custom deleter** such as `std::unique_ptr<Logger, PoolDeleter>`,
and write "only call the destructor and do not return the memory".
But the type gets long, so **first suspect whether option 1 is enough.**

### Option 3: The caller passes a buffer

Often you do not even need the appearance of "creating".

```cpp
void init_logger(UartLogger & out, int channel);   // the caller decides where it lives
```

This is the same idea as 1.7 in chapter 1. **Making the side that allocates and the side that uses the same** is the basis of microcontroller code.

## 4.8 Conclusion for ROS 2 (supplement)

You can use the heap, so you can write the form that returns `std::unique_ptr` as it is.

But the GoF `Factory` class hierarchy hardly appears in rclcpp.
Instead, **member functions of `Node` take care of creation**.

```cpp
auto publisher = this->create_publisher<std_msgs::msg::String>("topic", 10);
```

`create_publisher` returns a `shared_ptr`. The reason it is not `unique_ptr` is
that **rclcpp itself also observes it with a weak reference** ([C++ 6.5](../cpp/06_smart_pointers.md)).
It is a clean example that satisfies the condition of 4.5,
"there is common processing before and after creation (resolving QoS, registering with the Executor)".

`ClassLoader` of `pluginlib` is a real run-time Factory, but
**it pays off only when you have a requirement to load shared libraries at run time**.
In your own library, you will almost never need that much.

## 4.9 Common pitfalls

| Symptom | Cause |
| --- | --- |
| `error: virtual function 'create' has a different return type` | The derived class returns `unique_ptr<Derived>`. Covariance does not work (4.3). Return to `unique_ptr<Base>` |
| `error: no viable conversion from 'std::unique_ptr<Base>' to 'std::unique_ptr<Derived>'` | You cannot convert in the reverse direction. If you need `dynamic_cast`, suspect the design |
| `error: use of deleted function ... unique_ptr(const unique_ptr &)` | You are copying a `unique_ptr`. Return it or `std::move` it |
| `warning: moving a local object in a return statement prevents copy elision` | You wrote `return std::move(x);`. Make it `return x;` |
| The created object is never released | The factory holds it with `shared_ptr` too. **The Creator does not own the created object** |
| The created object broke when I deleted the factory | The created object refers to a member inside the factory with a raw pointer. Lifetime inversion |
| The count increases even though creation failed | You call `create_logger()` directly from outside. Keep the procedure inside `create()` |
| Allocation fails on a microcontroller after running for a while | You call `make_unique` inside a loop. 4.7 |

## 4.10 Matching exercise

```bash
./drill run dp04
```

In `exercises/dp04_factory_method/src/logger_factory.cpp`, you implement

1. `LoggerFactory::create()`, the template method (create → stop if it failed → register → return)
2. `MemoryLoggerFactory::create_logger()`, the factory method. `std::make_unique`, and failure is `nullptr`
3. `MemoryLoggerFactory::register_logger()`, which records the tag
4. `MemoryLogger::write()` and the constructor / destructor of `Logger`

There are 6 tests, and 3 of them check **that ownership moves to the caller**.
Using a live-object counter, they check **that the object is destroyed when it leaves the scope**,
**that the owner moves with `std::move`**, and **that there is no double free even if it disappears together with the factory**.

## 4.11 Summary of this chapter

- **Factory Method is a kind of Template Method.** Fix the procedure of creation in the base, and leave only what to create
- Where Java returns `new Product()`, **return `std::unique_ptr<Product>`**.
  With a raw pointer, **the type does not say who `delete`s it**
- **Do not write `std::move`** in `return product;`
- **Covariant return types do not work with `unique_ptr`.** Even in the derived class, the return type stays `unique_ptr<Base>`
- `unique_ptr<Derived>` → `unique_ptr<Base>` converts implicitly. **The reverse is not possible**
- Creation failure is **`nullptr`**. `std::optional<unique_ptr<T>>` has three states, so do not use it
- The registering side receives `const Product &`. **Show in the type that ownership is not handed over**
- **If the type is decided at compile time, you do not need a Factory.** If there is one ConcreteCreator, do not add it
- On microcontrollers, **first consider "do not use a Factory"**. If you need one, use a fixed pool + placement new.
  Use neither `unique_ptr` nor exceptions

---

Previous: [3. Template Method](03_TemplateMethod.md) / Next: [5. Singleton](05_Singleton.md)
