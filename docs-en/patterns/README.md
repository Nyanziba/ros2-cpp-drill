# Design Patterns (23 chapters)

This course is meant to run **alongside a reading group on Hiroshi Yuki's *Learning Design Patterns in Java* (revised and expanded edition, in Japanese).**
It does not repeat the content of the book. **Please read it with the book open next to you.**

It covers only one point.

> **When you write the pattern of that chapter in C++, what changes from the Java version?**

## This track is independent of the other tracks

This course is a **separate track from C++ Basics, C++, and ROS 2**.
You can read it from the top without having read those tracks. In the other direction, the
other tracks are complete without this one. **There is no required order.**

The theme is "designing your own library as a team", not reading rclcpp.
Notes about rclcpp appear only as **supplements** at the end of each chapter.

## Why do we need a separate course?

Hiroshi Yuki's book is written in Java. Java and C++ differ in assumptions that directly affect how patterns are implemented.

| Assumption in the book | What it becomes in C++ |
| --- | --- |
| There is a GC. Objects disappear by themselves | You must **state in the type who releases the object**: `unique_ptr` / `shared_ptr` / reference / value |
| Every variable is a reference (except primitives) | There is value semantics. If you write without thinking, **copies happen** |
| You can implement any number of `interface`s | Multiple inheritance of pure virtual classes. Diamond inheritance and `virtual` inheritance problems appear |
| You can return `null` | You choose between returning `nullptr` and returning `std::optional` |
| `clone()` / `Cloneable` | The copy constructor already exists. `clone()` is needed **only for polymorphism** |
| You are told not to use `finalize()` | Destructors and RAII are the main tools. C++ is even stronger here |
| Every method is virtual | A function is not virtual unless you write `virtual`. **Forgetting it is an immediate bug** |

If you copy the Java version without closing this gap, you get **code that runs, but whose lifetimes are broken**.
This is the accident you least want in your team's library.

## How to use this course

Every article has the same fixed form.

```
> Corresponds to Yuki's book, Chapter N

1. What happens if you write the Java structure in C++ as is
2. How to write it naturally in C++
3. Implement it (→ go to the exercise)
4. Does the standard library already have the same thing?
5. Conclusion for microcontrollers
6. Conclusion for ROS 2
```

**Always do the "Implement it" step.** You write every pattern by hand at least once.
Then, each time, you check "does the standard library already have the same thing?".
In C++, some of the GoF patterns are **already language features or standard library features**.
Only when you put your own version next to the standard one can you see "why the standard one has this form".
If you skip writing your own, you cannot make this comparison.

## Chapter table

The "Rewrite in C++" column **does not mean you can skip implementing.**
It means how you change the form when you adopt the pattern in real work, after you have implemented it.

### Part 1: Getting used to design patterns

| Chapter | Pattern | Rewrite in C++ | Exercise |
| --- | --- | --- | --- |
| [1](01_Iterator.md) | Iterator | After writing the GoF version, **move to the STL iterator style** (`begin()` / `end()`) | `dp01` |
| 2 | Adapter | Do not use the multiple-inheritance version. **Use only the delegation version** | `dp02` |

### Part 2: Leave it to subclasses

| Chapter | Pattern | Rewrite in C++ | Exercise |
| --- | --- | --- | --- |
| 3 | Template Method | **NVI** (public non-virtual → private virtual). A virtual destructor is required | `dp03` |
| 4 | Factory Method | Return `std::unique_ptr<Product>`, not a raw pointer | `dp04` |

### Part 3: Creating instances

| Chapter | Pattern | Rewrite in C++ | Exercise |
| --- | --- | --- | --- |
| 5 | Singleton | The Java static field **breaks because of initialization order**. Move to the Meyers Singleton | `dp05` |
| 6 | Prototype | `clone()` uses a covariant return type + `unique_ptr`. How it shares the work with the copy constructor | `dp06` |
| 7 | Builder | Method chaining, and up to **ref-qualifiers** (`&&` overloads) | `dp07` |
| 8 | Abstract Factory | Almost the same. But it is **the top case of overuse** | `dp08` |

### Part 4: Thinking in separate parts

| Chapter | Pattern | Rewrite in C++ | Exercise |
| --- | --- | --- | --- |
| 9 | Bridge | **Same family as Pimpl**. ABI and compile time come into the story | `dp09` |
| 10 | Strategy | **Three choices**: virtual function / `std::function` / template. The choice changes on microcontrollers | `dp10` |

### Part 5: Treating things the same

| Chapter | Pattern | Rewrite in C++ | Exercise |
| --- | --- | --- | --- |
| 11 | Composite | Same structure. The hard part is **ownership of children** (a vector of `unique_ptr`) | `dp11` |
| 12 | Decorator | Nested `unique_ptr`. Build it with moves | `dp12` |

### Part 6: Walking through a structure

| Chapter | Pattern | Rewrite in C++ | Exercise |
| --- | --- | --- | --- |
| 13 | Visitor | After writing the GoF double dispatch, compare it with **`std::variant` + `std::visit`** | `dp13` |
| 14 | Chain of Responsibility | Same structure. The hard part is the **lifetime** of the chain | `dp14` |

### Part 7: Keeping it simple

| Chapter | Pattern | Rewrite in C++ | Exercise |
| --- | --- | --- | --- |
| 15 | Facade | You rarely need a class. **A namespace + free functions** is often enough | `dp15` |
| 16 | Mediator | Mutual `shared_ptr` references make a **cycle**. You need `weak_ptr` | `dp16` |

### Part 8: Managing state

| Chapter | Pattern | Rewrite in C++ | Exercise |
| --- | --- | --- | --- |
| 17 | Observer | **The peak of this course.** The problem of a subscriber dying first. `weak_ptr` and unsubscribing | `dp17` |
| 18 | Memento | Value semantics make it easier than in Java | `dp18` |
| 19 | State | After writing the class version, compare it with `enum` + `switch` / `std::variant` | `dp19` |

### Part 9: Removing waste

| Chapter | Pattern | Rewrite in C++ | Exercise |
| --- | --- | --- | --- |
| 20 | Flyweight | Sharing with `shared_ptr` and `std::string_view` | `dp20` |
| 21 | Proxy | **A smart pointer itself** is a Proxy. You write `operator->` | `dp21` |

### Part 10: Expressing with classes

| Chapter | Pattern | Rewrite in C++ | Exercise |
| --- | --- | --- | --- |
| 22 | Command | `std::function` and a command queue. On microcontrollers, allocation is a problem | `dp22` |
| 23 | Interpreter | Almost the same. There is little that is specific to C++ | `dp23` |

## Microcontrollers and ROS 2 give different conclusions

In robot development, you often write for **both microcontrollers and ROS 2**. The two have different assumptions,
so every article ends with the conclusion for both. Here is a summary of the differences first.

| Topic | Microcontroller | ROS 2 (on Linux) |
| --- | --- | --- |
| Dynamic allocation (`new` / `make_unique`) | **Only at startup, as a rule.** Forbidden inside loops | Free. But avoid it on real-time paths |
| Exceptions | `-fno-exceptions` is normal. You cannot `throw` | Usable |
| RTTI / `dynamic_cast` | Often **not usable** because of `-fno-rtti` | Usable |
| Virtual functions | Usable, but the vtable costs ROM/RAM. Do not overuse | No need to worry |
| `std::function` | **May allocate on the heap**. Replace it with a function pointer or a template | Fine to use |
| `std::string` | Allocates. Use a fixed-length buffer or `string_view` | Fine to use |
| Singleton | A peripheral is effectively a singleton. **Initialization order is the real issue** | Avoidable. You should avoid it |
| How to implement Strategy | Prefer a template (compile time) first | Virtual functions / `std::function` are fine |

Because of this table, it is natural that **the same pattern is written one way on a microcontroller and another way in ROS 2**.
Each article separates the two clearly, every time.

## How to run the exercises

The exercises run with the same `drill` as the ROS 2 and C++ tracks. **ROS 2 is not used.**
Only plain C++17 and gtest.

```bash
./drill list              # list
./drill run dp01          # exercise for chapter 1
./drill watch dp01        # run the tests every time you save
./drill hint dp01         # hints
./drill solution dp01     # sample solution
```

Microcontroller constraints (such as `-fno-exceptions`) are **not in the build settings of the exercises**.
This reduces the time you lose to build environment problems. How to write under those constraints is shown
as code in the "Conclusion for microcontrollers" section of each article.

## Prerequisites

**Required**

- Hiroshi Yuki, *Learning Design Patterns in Java* (revised and expanded edition, in Japanese) (SB Creative)
- You have written inheritance, virtual functions, and `std::unique_ptr` / `std::shared_ptr` in C++

**Where to look when you are stuck** (you do not need to have read them)

| Where you are stuck | Reference |
| --- | --- |
| Difference between reference and value | [References](../cpp-basics/03_references.md) / [Value semantics](../cpp-basics/09_value_semantics.md) |
| `virtual` / `override` / virtual destructor | [Inheritance](../cpp/03_inheritance.md) |
| `unique_ptr` / `shared_ptr` / `weak_ptr` | [Smart pointers](../cpp/06_smart_pointers.md) |
| Move | [Move and ownership](../cpp/05_move_and_ownership.md) |
| Lambdas and `std::function` | [Lambdas and `std::bind`](../cpp/07_lambdas_and_std_bind.md) |

**Ownership becomes the main topic in about 15 of the 23 chapters.**
If you feel unsure about `unique_ptr`, look at the matching chapter in the table above first.

## How to proceed

**Do not get ahead of the reading group.** If you read the article for a chapter before you read the book,
the axis of "what is different from the book" does not work. The order is: read one chapter → read the same chapter in this course → do the exercise.

Each chapter takes 45 to 90 minutes. The estimate for 23 chapters is 25 to 35 hours.

## What to read first

Right after you learn patterns is when you break code the most.
Please read [0. Before you use them](00_before_you_use_them.md) first. It takes 5 minutes.
