# 20. Flyweight

> **Corresponds to Hiroshi Yuki's *Learning Design Patterns in Java*, chapter 20.** Keep `BigChar`, `BigCharFactory` and `BigString` open next to this chapter.
>
> **Goal of this chapter**: Flyweight is **the only pattern that exists for optimization**. It is not there to improve your design.
> So you decide whether to use it only after you measure. On top of that, there is one problem specific to C++.
> In Yuki's book, `BigCharFactory` keeps adding instances to a `HashMap`. The book says that in Java these are
> "never released". But in C++, if you use `std::map<K, std::shared_ptr<T>>`, they are
> **really never released until the process ends**. Here we measure why you change it to `std::weak_ptr`.

## 20.0 First, the decision not to use it

In [0. Before you use them](00_before_you_use_them.md), item 4 of the checklist,
"Is there something in the standard library that does the same thing?", listed Flyweight. That is the main point of this chapter.

Flyweight is something you use **after you measure**. Do not get the order wrong.

1. You have a **symptom**: memory is short, or allocation is slow
2. You measured, and you **know** the cause is "a huge number of objects with the same content"
3. These objects have a part that is **immutable**

Only when all three are true is it Flyweight. If you use it without them, you gain nothing,
and **only the lifetime complexity of sharing increases**.

- You cannot release anything unless you care whether "someone is still using it"
- If someone changes it, everyone is affected
- If you look it up from several threads, you need a lock
- You have to decide when to clean up the pool

So **using Flyweight means making lifetime management one level more complex**.
If the memory you can save is a few hundred bytes, it is not worth it.

Here is a rule of thumb.

| Situation | Conclusion |
| --- | --- |
| Few kinds, many instances (4 models x 200 sensors) | Worth considering |
| Few instances (6 sensors) | **Do not use it** |
| The content to share is decided at compile time | Not Flyweight but **`constexpr`** (20.7) |
| You only want to share strings | **`std::string_view`** or string interning (20.5) |
| The content is mutable | It cannot be a Flyweight. Only immutable things can be shared |

To be honest, the exercise in this chapter is also a subject where "in real work, `constexpr` is the end of it".
We still write it by hand once, **so that you can judge that `constexpr` is enough**.

## 20.1 What if you move the Java version straight to C++?

This is the body of `BigCharFactory` in Yuki's book.

```java
private Map<String, BigChar> pool = new HashMap<>();

public synchronized BigChar getBigChar(char charname) {
    BigChar bc = pool.get("" + charname);
    if (bc == null) {
        bc = new BigChar(charname);
        pool.put("" + charname, bc);
    }
    return bc;
}
```

If you move it to C++ in a straightforward way, it looks like this.

```cpp
std::map<std::string, std::shared_ptr<BigChar>> pool_;

std::shared_ptr<BigChar> get_big_char(char name)
{
  const std::string key(1, name);
  auto found = pool_.find(key);
  if (found != pool_.end()) {
    return found->second;
  }
  auto created = std::make_shared<BigChar>(name);
  pool_[key] = created;
  return created;
}
```

There are 3 changes from the Java version.

### Change 1: Return `std::shared_ptr<const T>`

The Java version returns a reference to `BigChar`. In C++, **you must decide who releases it**.
Flyweight is a pattern where "many users point to the same object", so ownership is shared too.
`std::unique_ptr` cannot express this. This is where `std::shared_ptr` comes in.

And you add `const`.

```cpp
std::shared_ptr<const CalibrationTable> get(const std::string & model_id);
```

**Make shared things const.** This is the premise of Flyweight itself.
If you return `shared_ptr<T>` (without const), when one user changes the content,
**it affects everyone who shares it**. The Java `BigChar` is immutable only by chance,
but in C++ you can enforce it with the type.

In the `Iterator` chapter we wrote that "if you forget `&`, it becomes a copy".
In Flyweight it is the opposite: **if you forget `const`, sharing becomes an accident**.

### Change 2: Delete the copy constructor

```cpp
class CalibrationTable
{
public:
  CalibrationTable(const CalibrationTable &) = delete;
  CalibrationTable & operator=(const CalibrationTable &) = delete;
  // ...
};
```

Java has no value copy. In C++, if you write nothing, a copy constructor is generated.

```cpp
CalibrationTable copy = *handle;   // if you write nothing, this compiles
```

The moment you copy a Flyweight, the point of sharing is gone. **Block it with `= delete`.**
Writing "shared things cannot be copied" in the type is the C++ way.

### Change 3: What to do with `synchronized`

The Java version has `synchronized`. C++ has no matching language feature.
We cover this in 20.6. **If you forget to write it, you get a data race right away**, so do not put it off.

## 20.2 Who releases the pool? The biggest difference from Java

This is the main point of this chapter.

Yuki's book also says: "Instances put in the pool are not targets of GC". In Java it **just leaks**.
If you use `std::map<K, std::shared_ptr<T>>` in C++, **the same thing happens, more reliably**.
The pool holds a `shared_ptr`, so the reference count never reaches 0.
Nothing is released until the pool dies.

You can see it with `use_count`. Try it yourself (the code in 20.4).

```
[strong]
  + Table(gyro)
  same? 1  use_count=3
  All users are gone
  - ~Table(gyro)
  The pool died
```

`use_count=3`. There are only 2 users, but the count is 3. **The remaining 1 is the pool.**
And `~Table` is called **just before** "the pool died",
that is, inside the destructor of the pool. It was not released when all the users were gone.

To fix this, **make the pool not own the objects**. Use `std::weak_ptr`.

```cpp
std::map<std::string, std::weak_ptr<const CalibrationTable>> pool_;

Handle get(const std::string & model_id)
{
  auto found = pool_.find(model_id);
  if (found != pool_.end()) {
    if (auto alive = found->second.lock()) {   // if it is still alive, a shared_ptr is returned
      return alive;
    }
    // lock() returned nullptr = leftover that nobody uses. Create it again
  }
  auto created = std::make_shared<CalibrationTable>(/* ... */);
  pool_[model_id] = created;                   // registered as a weak_ptr
  return created;
}
```

This is the output of the second half of the same program.

```
[weak]
  + Table(gyro)
  same? 1  use_count=2
  - ~Table(gyro)
  All users are gone
  pool.size()=1
```

Read 3 things.

1. `use_count=2`: the pool is not counted. It does not own the object
2. `~Table` appears **before** "All users are gone". It was released the moment the last user let go
3. **`pool.size()=1` stays the same**

The 3rd one is the pitfall of the `weak_ptr` version. `Table` is gone, but
**the `map` entry (the key and an empty `weak_ptr`) is still there**.
A `weak_ptr` cannot tell the `map` that it has expired.

So even with `weak_ptr`, **entries pile up, one for each kind of model you looked up**.
The content (a few hundred bytes) is gone, but the key (`std::string`) remains.
You need a function to clean up.

```cpp
std::size_t sweep_expired()
{
  std::size_t removed = 0;
  for (auto it = pool_.begin(); it != pool_.end();) {
    if (it->second.expired()) {
      it = pool_.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }
  return removed;
}
```

`erase` returns an iterator to the next element, so take it.
If you do `++it` and then `erase`, you touch an invalid iterator.

**When to call it** is a design decision. There are 3 choices.

| Method | Good for |
| --- | --- |
| Do not call it (accept the leftovers) | When the kinds are finite and few. For the subject of this exercise, this is enough |
| Inside `get()`, recreate only the leftover that you hit | Almost zero cost. But the whole pool is not cleaned |
| Call `sweep_expired()` periodically | When the kinds can grow without limit at run time (for example, the key is user input) |

**If the kinds are finite, a `shared_ptr` pool does no real harm.**
It is just "create 4 at startup and keep them until exit". You need `weak_ptr` only
**when the keys keep growing at run time**. Do not mix this up and add complexity you do not need.

## 20.3 Intrinsic and extrinsic state: if you split them wrong, you cannot share

In the words of Yuki's book, `BigChar` (the shape of the character) is **intrinsic**,
and "which character it is in the string" in `BigString` is **extrinsic**.

Let us say it again with the subject of the exercise.

| | Content | Where it lives |
| --- | --- | --- |
| Intrinsic | `gain` / `offset` for each model | `CalibrationTable` (shared, `const`) |
| Extrinsic | The zero-point correction `zero_offset` of each sensor | `Sensor` (each one owns it) |

There is one question for the split.

> **When you change this value, is it OK for other users to be affected too?**
> If yes, it is intrinsic. If no, it is extrinsic.

The moment you put `zero_offset` into `CalibrationTable`,
two sensors of the same model **have to hold separate tables**. Sharing is broken.

In C++, `const` catches this mistake for you.

```cpp
std::shared_ptr<const CalibrationTable> table_;   // the shared thing
double zero_offset_;                              // my own thing

double convert(int raw) const
{
  return raw * table_->gain() + table_->offset() + zero_offset_;
  //     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^   shared  ^^^^^^^^^^^^ individual
}
```

You cannot call a non-const member function through `table_`. **The moment you try to change it, you get a compile error.**
The Java version protects this with conventions and comments. C++ can protect it with types.

## 20.4 Try it yourself

Check the story of 20.2 with your own eyes, using `use_count` and the destructor.

```cpp
#include <iostream>
#include <map>
#include <memory>
#include <string>

struct Table
{
  explicit Table(std::string id) : id_(std::move(id))
  {
    std::cout << "  + Table(" << id_ << ")\n";
  }
  ~Table() { std::cout << "  - ~Table(" << id_ << ")\n"; }
  std::string id_;
};

// A version that moves Yuki's BigCharFactory over as it is
class StrongPool
{
public:
  std::shared_ptr<const Table> get(const std::string & id)
  {
    auto found = pool_.find(id);
    if (found != pool_.end()) {
      return found->second;
    }
    auto created = std::make_shared<Table>(id);
    pool_[id] = created;
    return created;
  }

private:
  std::map<std::string, std::shared_ptr<const Table>> pool_;
};

// A version that holds weak_ptr
class WeakPool
{
public:
  std::shared_ptr<const Table> get(const std::string & id)
  {
    auto found = pool_.find(id);
    if (found != pool_.end()) {
      if (auto alive = found->second.lock()) {
        return alive;
      }
    }
    auto created = std::make_shared<Table>(id);
    pool_[id] = created;
    return created;
  }

  std::size_t size() const { return pool_.size(); }

private:
  std::map<std::string, std::weak_ptr<const Table>> pool_;
};

int main()
{
  std::cout << "[strong]\n";
  {
    StrongPool pool;
    {
      auto a = pool.get("gyro");
      auto b = pool.get("gyro");
      std::cout << "  same? " << (a.get() == b.get()) << "  use_count=" << a.use_count() << "\n";
    }
    std::cout << "  All users are gone\n";
  }
  std::cout << "  The pool died\n";

  std::cout << "[weak]\n";
  {
    WeakPool pool;
    {
      auto a = pool.get("gyro");
      auto b = pool.get("gyro");
      std::cout << "  same? " << (a.get() == b.get()) << "  use_count=" << a.use_count() << "\n";
    }
    std::cout << "  All users are gone\n";
    std::cout << "  pool.size()=" << pool.size() << "\n";
  }
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic try.cpp -o try && ./try
```

<details>
<summary>Predict: what are the two <code>use_count</code> values? When does <code>~Table</code> appear?</summary>

This is the actual output.

```
[strong]
  + Table(gyro)
  same? 1  use_count=3
  All users are gone
  - ~Table(gyro)
  The pool died
[weak]
  + Table(gyro)
  same? 1  use_count=2
  - ~Table(gyro)
  All users are gone
  pool.size()=1
```

- The `strong` version has `use_count=3`: 2 users + **1 pool**.
  `~Table` appears just before the pool dies, not when the users let go
- The `weak` version has `use_count=2`. `~Table` appears **the moment** the users let go
- But even in the `weak` version, `pool.size()` stays at 1.
  **A `weak_ptr` cannot remove itself from the `map` when it expires**

`same? 1` is the same in both. Whether sharing works is not the difference between `strong` and `weak`.
The only difference is **when the object is released**.
</details>

## 20.5 Does the standard library or the language already have the same thing?

**Yes. In fact, C++ has 3 tools that are lighter than writing a Flyweight class.**
Before you write your own, always check whether these 3 are enough.

### (1) `std::shared_ptr<const T>`: this is itself a tool for sharing

`std::shared_ptr` already implements half of the Flyweight pattern (sharing and lifetime).
**The only part you write yourself is the pool.**
You do not need a "Flyweight class" base class, or `virtual`, at all.

`BigChar` in Yuki's book has no virtual functions. **Copy that part.**
Flyweight does not need inheritance.

### (2) `std::string_view`: share the string (do not copy)

If the thing you want to share is a string, you do not need Flyweight.

```cpp
// Bad example: if you hold the diagnostic message as std::string, there are as many copies of the same text as there are instances
struct Diagnostic
{
  std::string message;   // "over current" is copied 200 times
};

// With std::string_view, it only points. Zero allocation
struct Diagnostic
{
  std::string_view message;
};

inline constexpr std::string_view kOverCurrent = "over current";
```

`std::string_view` is a **pair of a pointer and a length**. Copying it does not grow the content.
This is the lightest way to share.

**But there is one danger. If the original string dies, the `string_view` dangles.**

```cpp
std::string_view get_name()
{
  std::string name = "MPU6050";
  return name;              // name dies here
}                           // the returned string_view is dangling

std::string_view bad = std::string("MPU") + "6050";   // the temporary object dies immediately
```

This is **exactly the same shape** as the problem in the `Iterator` chapter (1.3), "the iterator outlives the bookshelf".
Think of `string_view` as a kind of iterator.

It is safe when the target is one of the following.

| Target | Safe? |
| --- | --- |
| String literal (`"abc"`) / `constexpr` table | **Safe**. Same lifetime as the program |
| A `std::string` the caller holds (passed as an argument) | Safe only during the call |
| A local `std::string` | **Dangerous**. It dies when the function returns |
| A temporary object (the result of `a + b`) | **Dangerous**. It dies when the expression ends |

`std::string_view` is most dangerous when you store it as a member.
**If you store it, promise the lifetime of the target in the type or in a comment.**

### (3) String interning

If you handle a huge number of the same strings, you can keep only one copy in a `std::set<std::string>`
and hand out `std::string_view`. This is "string interning", and it is
**the most common example of Flyweight**.

```cpp
class StringPool
{
public:
  // Do not let the returned string_view outlive this StringPool.
  std::string_view intern(std::string value)
  {
    // The addresses of std::set elements are stable (they do not move on reallocation).
    // With std::vector<std::string>, all the addresses change on reallocation,
    // and every string_view you handed out is gone. This is why we chose it.
    return *pool_.insert(std::move(value)).first;
  }

private:
  std::set<std::string> pool_;
};
```

As the comment says, **the choice of container is directly tied to lifetime**.
Do not use `std::vector`.

### Things the standard does not have

The pool itself is not in the standard. There is no `std::flyweight`
(Boost has `boost::flyweight`).
**The standard has sharing and lifetime, and you write only the pool.** This is how it looks in C++.

## 20.6 Thread safety: `shared_ptr` protects only half

You write the equivalent of the Java `synchronized` yourself. If you skip this, it breaks.

**The reference count of `std::shared_ptr` is atomic.** It is safe for several threads to copy and destroy
the same `shared_ptr`.

**But the pool (`std::map`) is not protected at all.**
If two threads call `get()` at the same time,

- One is in the middle of `insert` and the other does `find`: undefined behavior
- Both decide "it does not exist, so let me create it": **two instances of the same model** are created (sharing is broken)

The fix is to wrap the whole of `get()` and `sweep_expired()` in a lock.

```cpp
class CalibrationRegistry
{
public:
  Handle get(const std::string & model_id)
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    // ... find / lock / make_shared / insert
  }

  std::size_t sweep_expired()
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    // ...
  }

private:
  mutable std::mutex mutex_;
  std::map<std::string, std::weak_ptr<const CalibrationTable>> pool_;
};
```

**Do not let go of the lock between `find` and `insert`.** "Look up, and create if missing" is
one indivisible operation. If you split it into two locks, the 2nd accident above happens.

The registry in the exercise is **single-thread only**. It has no lock.
The comment in the header says so. **Writing it down is the job.**

And this is also why we return to 20.0.
When you use Flyweight, **a new slowness called the lock** comes with it.
You pay time to save memory, so you cannot know whether you really gain without measuring.

## 20.7 The conclusion for microcontrollers

**On a microcontroller, you do not write the pool from this chapter.** There are 3 reasons, and all are fatal.

1. `std::map`, `std::string` and `std::make_shared` all **allocate on the heap**
2. A `std::shared_ptr` control block is allocated for each object
3. In the first place, you do not create a huge number of objects. If you do not create them, there is nothing to share

What you use instead is `constexpr`. **Put the immutable thing you want to share in ROM.**

```cpp
#include <cstdio>
#include <string_view>

struct Spec
{
  std::string_view id;
  double gain;
  double offset;
};

inline constexpr Spec kRom[] = {
  {"MPU6050-GYRO", 0.0076294, 0.0},
  {"AS5600-ENC", 0.0878906, 0.0},
  {"ACS712-30A", 0.0666000, -2.5},
  {"NTC-10K", 0.0244140, -40.0},
};

constexpr const Spec * find_spec(std::string_view id)
{
  for (const Spec & spec : kRom) {
    if (spec.id == id) {
      return &spec;
    }
  }
  return nullptr;
}

int main()
{
  constexpr const Spec * ntc = find_spec("NTC-10K");
  static_assert(ntc != nullptr, "");
  static_assert(ntc->offset == -40.0, "");
  static_assert(find_spec("NO-SUCH") == nullptr, "");

  std::printf("sizeof(kRom) = %zu\n", sizeof(kRom));
  std::printf("offset = %f\n", ntc->offset);
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic rom.cpp -o rom && ./rom
```

This is the output.

```
sizeof(kRom) = 128
offset = -40.000000
```

**This is the strongest Flyweight.**

- It uses **0 bytes** of RAM. `kRom` is placed in `.rodata` (ROM)
- You need no pool and no `map`. `find_spec` **is itself the pool lookup**
- If the model is known at compile time, `static_assert` passes = **nothing happens at run time**.
  Every call to `find_spec("NTC-10K")` disappears
- Everyone points to the same `&kRom[3]`. Sharing is achieved automatically
- There is no lifetime problem. You need neither `weak_ptr` nor `sweep_expired()`

In fact, if you compile the code above with `-O2` and look inside,
no call to `find_spec` is left. The 3 `static_assert` lines disappear at compile time,
and only `printf` remains at run time.

Using `std::string_view` is also on purpose. With `std::string`, allocation would run.
A `string_view` that points to a literal points to something with the same lifetime as the program, so it is a **safe use** (the first row of the table in 20.5).

### When the model is known only at run time

Even if you read the model from EEPROM, you can still use a `constexpr` table.
What disappears is only "the call to `find_spec`";
**the table is still in ROM, and the number of allocations is still zero**.

```cpp
const Spec * spec = find_spec(read_model_id_from_eeprom());
if (spec == nullptr) {
  // Unknown model. Exceptions are not available, so return it as a return value
  return Error::UnknownModel;
}
```

`-fno-exceptions` is normal, so **return `nullptr` when it is not found**.
Do not use `throw`, and do not use the throwing version of `std::optional` (`.value()`) either.
`CalibrationRegistry::get()` in the exercise returns `nullptr` for unknown models for the same reason.

### When you still have to create them at run time

This is the case where you want to rewrite the calibration values in the field. Then
**create all of them at startup in a fixed-length array and hand them out by index**.

```cpp
// Only once at startup. Zero allocation inside the loop
class CalibrationStore
{
public:
  /// Returns nullptr if not found. The Store keeps ownership (users only point to it)
  const Spec * find(std::string_view id) const
  {
    for (std::size_t i = 0; i < count_; ++i) {
      if (entries_[i].id == id) {
        return &entries_[i];
      }
    }
    return nullptr;
  }

private:
  static constexpr std::size_t kCapacity = 8;
  Spec entries_[kCapacity] = {};
  std::size_t count_ = 0;
};
```

Note that this does not use `shared_ptr`.
**If the owner is decided to be one (`CalibrationStore`), you do not need shared ownership.**
Users only point to it with raw pointers. Write the lifetime promise in a comment: "`CalibrationStore` is one global instance,
with the same lifetime as the program".

You need `shared_ptr` only when "**you do not know who lets go last**".
On a microcontroller, you almost never create such a situation.

## 20.8 The conclusion for ROS 2 (supplement)

In ROS 2, there are almost no cases where you write Flyweight yourself.

- Message type definitions and QoS profiles are few in number to begin with
- `rclcpp` uses `shared_ptr` a lot internally, but that is not
  Flyweight (sharing the same content). It is `shared_ptr` for **lifetime management**. Do not confuse them
- When you pass large data around, what you think about is not Flyweight but
  **zero copy** (`loaned message` / intra-process communication). The goal is not "share the same content" but
  "avoid copying and serialization", so it is a different topic

The closest case is when "you pass parameter names or frame IDs around as `std::string`".
If you copy a `std::string` in every callback,
change it to `std::string_view`, or hold one `static const std::string`.
This is the topic of (2) and (3) in 20.5, and you do not need a Flyweight class.

## 20.9 Common pitfalls

| Symptom | Cause |
| --- | --- |
| Not released even after all users are gone | The pool holds it with `shared_ptr`. Change it to `weak_ptr` |
| `use_count` is 1 larger than expected | Same as above. The pool counts 1 |
| You used `weak_ptr` but `pool.size()` does not shrink | Expired entries are not removed automatically. You need `sweep_expired()` |
| A different instance is returned for the same model | You did not check the return value of `lock()`, or you forgot to register it in the pool |
| When someone changed a value, everyone broke | You return `shared_ptr<T>`. Make it `shared_ptr<const T>` |
| Sharing broke when you copied a Flyweight | You did not `= delete` the copy constructor |
| Each sensor gets a separate table | You put the extrinsic state (`zero_offset`) into the Flyweight |
| `sweep_expired()` crashes | You did `++` on an iterator that you `erase`d. Use `it = pool_.erase(it)` |
| A stored `string_view` shows garbage | The `std::string` it pointed to is dead. See the table in 20.5 |
| Two instances of the same model appeared with multiple threads | You let go of the lock between `find` and `insert` |
| The microcontroller ran out of heap | You are using `std::map` + `make_shared`. Use a `constexpr` table |

## 20.10 Matching exercise

```bash
./drill run dp20
```

You share sensor calibration tables. The subject is a situation where there are only 4 models,
but the vehicle carries dozens of sensors. You implement 3 things in `exercises/dp20_flyweight/src/calibration.cpp`.

1. **`CalibrationRegistry::get()`**: get a shared instance from a `std::weak_ptr` pool.
   If it is alive, share it. If it is a leftover, create it again. If it is not in ROM, return `nullptr`
2. **`CalibrationRegistry::sweep_expired()`**: clean up expired leftovers
3. **`Sensor::convert()`**: combine the shared `gain` / `offset` with the per-sensor `zero_offset`

The `constexpr` version (`kCalibrationRom` / `find_spec`) is **already implemented**. Read it.
The conclusion of this chapter is that in real work, it is often the end of the story.

There are 7 tests.

- Getting the same model twice returns **the same address**
- The number of creations matches **the number of kinds** (3), not the number of lookups (5)
- When everyone lets go, it is destroyed, and `sweep_expired()` removes it from the pool too (measured with `use_count` and the destructor)
- Extrinsic state is not shared
- The `constexpr` table passes `static_assert`, and **there are 0 heap allocations at run time**
  (the test replaces the global `operator new` and counts)

## 20.11 Summary of this chapter

- Flyweight is an **optimization**. It is not a pattern that improves your design. **Measure first, then use it**
- When you use it, in exchange for the memory you save, **lifetime complexity and locks** increase
- If you move a Java `HashMap` pool to `std::map<K, std::shared_ptr<T>>`,
  **nothing is released until the process ends**. You can notice it because `use_count` is 1 larger
- With a `std::weak_ptr` pool, objects are released. But **you must clean up the empty entries yourself**
- Shared things are **`const`**. Return `shared_ptr<const T>` and `= delete` the copy
- The way to split intrinsic and extrinsic state is "when you change it, is it OK for others to be affected?"
- C++ has light tools: **`std::shared_ptr<const T>` / `std::string_view` / string interning**.
  For `string_view`, the lifetime of the target is everything
- The reference count of `shared_ptr` is atomic, but **the pool is not protected**. Do not let go between `find` and `insert`
- **On a microcontroller, use a `static constexpr` ROM table**. 0 bytes of RAM, 0 allocations, no lifetime problem.
  This is the strongest Flyweight

---

Previous: [19. State](19_State.md) / Next: 21. Proxy (coming soon)
