# 5. Move and ownership

> **Goal of this chapter**: In exercise 14 you write this one line.
>
> ```cpp
> publisher_->publish(std::move(msg));
> ```
>
> Despite its name, `std::move` **moves nothing.** It is a one-line cast.
> Even so, whether this line is there or not changes "whether the message is copied once or zero times",
> and the test of exercise 14 detects it by **comparing the address of the message.**
> In this chapter you learn the idea of "handing over ownership".
> It is the most misunderstood feature in C++, so we look inside `std::move` too.

> **Prerequisites**: [C++ Basics Chapter 9 Value semantics](../cpp-basics/09_value_semantics.md)
> covers "assignment is a copy" and "when and how many times copies happen".
> **Move is the continuation of that.** Read this chapter in a state where you can count copies.
> If pointers worry you, go to [Chapter 4](../cpp-basics/04_pointers_1.md) and [Chapter 5](../cpp-basics/05_pointers_2.md).

## 5.0 Why did we need a feature called move?

We start with "what problem we wanted to solve". Once you see this, the rest all becomes a story with reasons.

Before C++11, there were **only 2 ways** to pass or return an object to or from a function.

1. **Copy** — safe. But slow if the contents are large
2. **Pass a pointer / reference** — fast. But **who releases it** does not appear in the type

The problem of the second way was serious.

```cpp
std::vector<int> * make_data();     // who deletes the return value?
```

You cannot tell from the signature. You have to read the documentation, and if you do not, you leak or
free twice. This is an accident that has been repeated endlessly in C libraries.

**What we wanted was a way of passing "without copying, and where the transfer of ownership appears in the type".**
That is move.

```cpp
std::unique_ptr<std::vector<int>> make_data();   // the caller owns it. The type says so
```

So **move is less "an optimization to make things fast" and more "a feature to express ownership with types".**
Being faster is a result, not the purpose.
In this drill `std::move` appears in only one place, exercise 14, but
to understand `std::unique_ptr` and `std::shared_ptr` (Chapter 6),
this sense that "ownership moves" is the foundation.

### Line up the 3 ways of passing

```cpp
void copy_it(std::vector<int> v);                  // copy: the caller also keeps its own
void borrow_it(const std::vector<int> & v);        // borrow: ownership does not move
void take_it(std::vector<int> && v);               // move: take ownership
```

In plain words, it is like this.

| How it is written | Meaning | After the call, the original variable |
| --- | --- | --- |
| `T v` | **Give me a copy** | Can be used as it is |
| `const T & v` | **Let me see it** | Can be used as it is |
| `T && v` | **I will take it** | **You promise not to use it any more** |

**These 3 steps are the continuation of Chapter 4.** In Chapter 4 you chose between "a value or `const &`",
and the story is that there was actually a third option.

### "You promise not to use it" is only a promise

I deliberately wrote "promise" on the third line. It is easy to misunderstand, so let us deal with it first.

**Just receiving with `T &&` does not move even one byte of the contents.**

```cpp
#include <iostream>
#include <utility>
#include <vector>

void take_it(std::vector<int> && v)
{
  std::cout << "take (size " << v.size() << ")\n";
  // nothing was taken from v
}

int main()
{
  std::vector<int> v{1, 2, 3};
  take_it(std::move(v));
  std::cout << "after move, size = " << v.size() << "\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/YETdsP484)

```
take (size 3)
after move, size = 3
```

**We wrote `std::move`, but `v` did not become empty.**

This is because `&&` is a **permission** that says "you may take it", not the **action** of taking.
To really take it, the receiving side must move the contents.

```cpp
void take_it(std::vector<int> && v)
{
  std::vector<int> mine = std::move(v);    // it is taken here for the first time
  std::cout << "take (size " << mine.size() << ")\n";
}
```

With this, you get `after move, size = 0`.

In other words, even if `std::move` and `&&` are both there, **nothing happens unless someone calls
the move constructor or the move assignment.** The relation among these 3 is the subject of Section 5.2.

- `std::move(x)` — marks "this may be taken" (only a cast)
- `T &&` — the argument type that "receives only marked things"
- Move constructor / move assignment — **the processing that actually takes**

## 5.1 Lvalues and rvalues

Before we talk about `T &&`, let us define the "rvalue" that `&&` receives.

Remember the error from the end of the previous chapter.

```cpp
void f(std::string & s);
f("hello");   // error: cannot bind non-const lvalue reference ... to an rvalue
```

The word "rvalue" appeared. Let us deal with it first.

### The origin of the name, and a practical way to tell them apart

Originally it was the distinction of **whether it can be placed to the left of `=` (left value) or only to the right (right value).**

```cpp
a = 42;         // a can be placed to the left of = -> lvalue
42 = a;         // this cannot be written        -> 42 is an rvalue
```

But this explanation is not accurate in modern C++ (`const int a` cannot be placed on the left, but it is an lvalue).
**The practical way to tell is "does it have a name".**

```cpp
std::string a = "hello";
std::string b = "world";

a                       // lvalue (it has a name)
"hello"                 // rvalue (it has no name)
std::string("hi")       // rvalue (a temporary object)
a + b                   // rvalue (a temporary object of the calculation result)
make_string()           // rvalue (a temporary object of the return value)
std::move(a)            // rvalue (it has a name, but we instructed to treat it so)
```

Another mechanical test is **whether you can take its address with `&`.**

```cpp
&a;                     // OK -> lvalue
&std::string("hi");     // error -> rvalue
```

```
error: taking address of rvalue
```

### Why this distinction matters

**This is because the compiler knows that nobody can access an rvalue after this expression ends.**

```cpp
std::string make() { return "long string..."; }

std::string s = make();
```

The return value of `make()` is a temporary object, and after this line it has no name, so there is no way to access it.
**If nobody is looking, we may take the contents as they are, without copying.**
This is the idea of move.

On the other hand, taking from an lvalue is dangerous.

```cpp
std::string a = "hello";
std::string b = a;        // a should still be usable afterward. We must not take from it
std::cout << a;           // so this works
```

**The rvalue reference `&&` is what made "may we take it" decidable by the type.**

### A slightly more precise story (you may skip this)

The standard divides them into 3 kinds. You will not need this in the drill,
but `xvalue` and similar words sometimes appear in error messages, so we leave the correspondence here.

| Term | What | Example |
| --- | --- | --- |
| **lvalue** | Has a name, and is still used | `a` |
| **prvalue** | Has no name, a value that is about to be created | `42`, the return value of `make()` |
| **xvalue** | Has a name, but is declared as no longer used | `std::move(a)` |

"rvalue" is the name that groups prvalue and xvalue together.
It is enough to remember that **what `&&` can receive is an rvalue (prvalue and xvalue).**

### The rvalue reference `&&`

There is a way to write a reference that receives only rvalues.

```cpp
void f(std::string & s);        // lvalue reference — receives only lvalues
void f(const std::string & s);  // const lvalue reference — receives both
void f(std::string && s);       // rvalue reference — receives only rvalues
```

Let us actually see the dispatch.

```cpp
#include <iostream>
#include <string>

void take(const std::string & s) { std::cout << "const & : " << s << "\n"; }
void take(std::string && s)      { std::cout << "&&      : " << s << "\n"; }

int main()
{
  std::string a = "named";
  take(a);                    // lvalue -> const &
  take("literal");            // rvalue -> &&
  take(std::string("temp"));  // rvalue -> &&
  take(std::move(a));         // <- what about this?
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/Eeqnvzqba)

```
const & : named
&&      : literal
&&      : temp
&&      : named
```

The last line is the key. **`std::move(a)` makes `a` be treated as an rvalue.**

## 5.2 `std::move` does nothing

Let us write the conclusion right away.

**`std::move(x)` only casts `x` to an rvalue. It moves not even one byte of data.**

The implementation in the standard library is practically this.

```cpp
template<typename T>
constexpr std::remove_reference_t<T> && move(T && t) noexcept
{
  return static_cast<std::remove_reference_t<T> &&>(t);
}
```

There is only one `static_cast`. The run-time cost is zero.

Then what is it for? **It is to change overload resolution.**
When you write `std::move`, the compiler chooses the overload that takes `&&` (that is, the move version).
The one that does the actual "stealing" is the move constructor, and `std::move` is only its trigger.

It is often said that **the name is bad**, and if it had a name like `std::rvalue_cast`,
there would have been fewer misunderstandings.

### The move constructor "steals"

Let us reproduce what the move of `std::string` does with our own class.

```cpp
#include <iostream>
#include <utility>

class Buffer
{
public:
  explicit Buffer(std::size_t n) : size_(n), data_(new int[n]())
  {
    std::cout << "  allocated " << n << "\n";
  }

  ~Buffer() { delete[] data_; }

  // copy: allocate anew and copy the contents
  Buffer(const Buffer & o) : size_(o.size_), data_(new int[o.size_])
  {
    std::copy(o.data_, o.data_ + o.size_, data_);
    std::cout << "  copy (with allocation)\n";
  }

  // move: only swap the pointer
  Buffer(Buffer && o) noexcept : size_(o.size_), data_(o.data_)
  {
    o.data_ = nullptr;      // <- this is the important part
    o.size_ = 0;
    std::cout << "  move (no allocation)\n";
  }

  std::size_t size() const { return size_; }
  const int * raw() const { return data_; }

private:
  std::size_t size_;
  int * data_;
};

int main()
{
  Buffer a(4);
  std::cout << "address of a: " << static_cast<const void *>(a.raw()) << "\n";

  std::cout << "copy:\n";
  Buffer b = a;
  std::cout << "address of b: " << static_cast<const void *>(b.raw()) << "\n";

  std::cout << "move:\n";
  Buffer c = std::move(a);
  std::cout << "address of c: " << static_cast<const void *>(c.raw()) << "\n";
  std::cout << "size of a: " << a.size() << "\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/8d7158eME)

Example output (addresses change on every run).

```
  allocated 4
address of a: 0x55555556b2b0
copy:
  copy (with allocation)
address of b: 0x55555556c2e0
move:
  move (no allocation)
address of c: 0x55555556b2b0
size of a: 0
```

**The address of `c` is the same as the original address of `a`.**
With the copy, a different address was allocated for `b`, but
with the move, `c` took over the same memory. This is what "ownership moved" means.

And the `size` of `a` became 0. **The moved-from object becomes empty.**

### What happens to the moved-from object

What the standard guarantees is a "**valid but unspecified state**".

- **What you may do**: destroy it, assign a new value to it
- **What you must not do**: read its contents and expect them to mean something

```cpp
std::string a = "hello";
std::string b = std::move(a);

std::cout << a;        // what is printed is unspecified (in practice an empty string in many implementations)
a = "reset";           // this is OK
std::cout << a;        // "reset"
```

Make it a habit: **"after you move, do not read it any more."**
A `std::string` actually becomes empty, but that is a matter of the implementation, not a guarantee.

`std::unique_ptr` is an exception: the standard guarantees that it **always becomes `nullptr` after a move.**
We use this in Chapter 6.

### Why add `noexcept`

We added `noexcept` to the move constructor. This is important.

When the number of elements grows, `std::vector` reallocates and moves all the elements.
At that time, **if the move is guaranteed not to throw an exception**, it uses the move,
and if it is not guaranteed, it uses the copy for safety.

So if you forget `noexcept`, **it silently becomes a copy only when you put it in a `std::vector`.**
Add `noexcept` to the move constructor and the move assignment.

## 5.3 Where you should write `std::move` and where you must not

### Where you should write it ①: when you hand over ownership

This is exercise 14.

```cpp
// solutions/14_zero_copy/src/zero_copy_nodes.cpp
void ZeroCopyTalker::publish_once()
{
  auto msg = std::make_unique<std_msgs::msg::String>();
  // ... build msg ...
  publisher_->publish(std::move(msg));
}
```

`publish` has 2 overloads.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/publisher.hpp (summarized)
void publish(std::unique_ptr<T, ...> msg)   // takes ownership
{
  if (!intra_process_is_enabled_) { this->do_inter_process_publish(*msg); return; }
  this->do_intra_process_publish(std::move(msg));    // pass it on as it is, without copying
}

void publish(const T & msg)                  // takes a value
{
  if (!intra_process_is_enabled_) { this->do_inter_process_publish(msg); return; }
  // Otherwise we have to allocate memory in a unique_ptr and pass it along.
  auto unique_msg = this->duplicate_ros_message_as_unique_ptr(msg);
  this->publish(std::move(unique_msg));
}
```

The comment says `we have to allocate memory`.
**The by-value version always makes one copy.** The caller's `msg` is still alive, so it cannot be stolen.

If you forget `std::move`, a `unique_ptr` cannot be copied, so you get a compile error and
you notice right away. This place is safe.

The test of exercise 14 checks **whether the address of the message is the same on the sender side and the receiver side.**
If `std::move` is there and no copy happens, they match, and if a copy is made in between, the address is different.

### Where you should write it ②: when a constructor receives an argument

```cpp
class Node
{
public:
  explicit Node(std::string name) : name_(std::move(name)) {}
private:
  std::string name_;
};
```

The form is "receive by value and put it in with `std::move`".
If the caller passes an rvalue it costs 1 move, and if it passes an lvalue it costs 1 copy.

If you make it `const std::string & name` and write `name_(name)`, even when an rvalue is passed
it always costs 1 copy. **This form wins by one operation.**

### Where you must not write it ①: when you `return`

```cpp
std::string make_bad()
{
  std::string s = "hello";
  return std::move(s);       // <- do not write this
}

std::string make_good()
{
  std::string s = "hello";
  return s;                  // this is correct
}
```

**`return std::move(s)` is slower.**
If you write `return s;`, the compiler can use **NRVO (named return value optimization)** to
"construct directly in the memory of the return destination", and not even a move happens.
If you write `std::move`, it becomes an rvalue reference, this optimization stops working, and one move happens.

g++ warns you about this.

```bash
g++ -std=c++17 -Wall -Wextra -Wpessimizing-move ...
```

```
warning: moving a local object in a return statement prevents copy elision [-Wpessimizing-move]
```

**`-Wpessimizing-move` is included in `-Wall`.** If this warning appears, delete the `std::move`.

### Where you must not write it ②: a variable you use afterward

```cpp
auto msg = std::make_unique<std_msgs::msg::String>();
publisher_->publish(std::move(msg));
std::cout << msg->data;              // msg is nullptr. A crash
```

**Do not read a variable you `std::move`d on any line after it.** This is as in Section 5.2.

Moving the same variable inside a loop is a typical accident.

```cpp
auto msg = std::make_unique<std_msgs::msg::String>();
for (int i = 0; i < 10; ++i) {
  publisher_->publish(std::move(msg));   // nullptr on the 2nd iteration
}
```

**Recreate it on every iteration** is the right answer.

```cpp
for (int i = 0; i < 10; ++i) {
  auto msg = std::make_unique<std_msgs::msg::String>();
  publisher_->publish(std::move(msg));
}
```

### Where you do not need to write it: temporary objects

```cpp
v.push_back(std::move(std::string("hi")));   // meaningless
v.push_back(std::string("hi"));              // it is already an rvalue
```

## 5.4 Copy elision — when neither copy nor move happens at all

In C++17, **the standard guarantees that neither copy nor move happens** in the following case.

```cpp
std::string make() { return std::string("hello"); }   // returns a temporary object
std::string s = make();                                // it is constructed only once
```

You can check it.

```cpp
#include <iostream>

struct Noisy
{
  Noisy() { std::cout << "create\n"; }
  Noisy(const Noisy &) { std::cout << "copy\n"; }
  Noisy(Noisy &&) noexcept { std::cout << "move\n"; }
};

Noisy make() { return Noisy(); }

int main()
{
  Noisy a = make();
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/sqqd7f8YG)

```
create
```

**Only "create".** Neither a copy nor a move happened even once.
`Noisy()` was constructed in the place of `a` from the start.

If you know this, you can see that the idea "I am worried about copying the return value, so let us return a reference"
is not needed. **You may return a value.** You can avoid the dangling reference of Chapter 4.

(You can stop the elision with `-fno-elide-constructors`, so
if you want to see the difference, compare with it. However, the cases guaranteed by C++17 do not stop.)

## 5.5 Rule of Zero / Rule of Five

In Chapter 2 we wrote that "copy and move are generated automatically".
Let us sort out the criteria for deciding whether to write them yourself.

**Rule of Zero: writing nothing is best.**
If you make all the members RAII types (`std::string`, `std::vector`, `std::unique_ptr`),
the copy, the move, and the destructor are all generated correctly and automatically.

The `Buffer` above had a raw pointer `int * data_`, so we had to write them ourselves.
If you make it a `std::vector<int>`, none of them is needed.

```cpp
class Buffer
{
public:
  explicit Buffer(std::size_t n) : data_(n) {}
  // copy, move, destructor: you do not need to write any of them
private:
  std::vector<int> data_;
};
```

**Rule of Five: if you write one, write all five.**
If you really have to manage by hand, the following 5 are a set.

1. Destructor
2. Copy constructor
3. Copy assignment operator
4. Move constructor
5. Move assignment operator

**If you write even one, some of the others are no longer generated automatically.**
The especially dangerous ones are these.

| What you wrote yourself | What is no longer generated |
| --- | --- |
| Destructor | **Move constructor, move assignment** |
| Copy constructor | **Move constructor, move assignment** |
| Move constructor | **Copy constructor, copy assignment** (they are `delete`d) |

"I wrote only the destructor, the move disappeared, and everything became a copy" is a typical accident.
Even `-Wall` cannot detect it.

**Aim for the Rule of Zero.** If you make the members RAII types, you do not need to write all 5.
The fact that all the members of `rclcpp::Node` are `SharedPtr` also follows this policy.

## Try it yourself

**Look at addresses and tell copy and move apart.** This is the same idea as the test of exercise 14.

```cpp
// move.cpp
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

void show(const char * tag, const std::string & s)
{
  std::cout << tag << " len=" << s.size()
            << " buf=" << static_cast<const void *>(s.data()) << "\n";
}

int main()
{
  // make it a long enough string (if it is short it does not use the heap, and the address moves)
  std::string a(64, 'x');
  show("a       ", a);

  std::string b = a;                 // copy
  show("b (copy)", b);

  std::string c = std::move(a);       // move
  show("c (move)", c);
  show("a (after)", a);

  std::cout << "\n-- unique_ptr\n";
  auto p = std::make_unique<std::string>("owned");
  std::cout << "p.get() = " << static_cast<const void *>(p.get()) << "\n";
  auto q = std::move(p);
  std::cout << "q.get() = " << static_cast<const void *>(q.get()) << "\n";
  std::cout << "p.get() = " << static_cast<const void *>(p.get()) << "\n";

  std::cout << "\n-- use after move\n";
  std::cout << "q is " << *q << "\n";
  // std::cout << *p << "\n";   // <- what happens if you remove the comment
  return 0;
}
```

**Make 3 predictions.**

1. Is the `buf` of `b (copy)` the same as `a`, or different?
2. Is the `buf` of `c (move)` the same as `a`, or different?
3. What does `p.get()` become after the move?

```bash
g++ -std=c++17 -Wall -Wextra move.cpp -o move && ./move
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/696hWbW6b)

**Change `std::string a(64, 'x')` to `std::string a = "hi"` and run it again.**
With a short string, the result changes. It is called **short string optimization (SSO)**:
up to about 15 characters, the string is held directly inside the object without using the heap,
so even with a move the address changes. Please confirm that "with a move the address is kept" is
**a story for the case where the heap is used.**

Next, let us experience `-Wpessimizing-move`.

```cpp
std::string make_bad()
{
  std::string s(64, 'y');
  return std::move(s);
}
```

```bash
g++ -std=c++17 -Wall -Wextra move.cpp -o move
```

Read the warning, and then delete the `std::move`.

Finally, enable the commented-out `std::cout << *p` and run it.
**`p` is `nullptr`, so it crashes here.**
If you see once what message it shows and how it crashes, you will notice faster when you meet the same symptom on a real robot.

## Common pitfalls

**`error: use of deleted function ‘std::unique_ptr<T>::unique_ptr(const std::unique_ptr<T>&)’`**
You are trying to copy a `unique_ptr`. Add `std::move`.
It often appears when you pass to a function, when you `return`, and when you put it in a `vector`.

**A copy happens even though you used `std::move`**
There are 3 possibilities.

1. The move constructor is not generated automatically (see the table of the Rule of Five)
2. `noexcept` is not added, and the reallocation of `std::vector` chose the copy
3. You tried to move a `const` object
   → `std::move(const T &)` becomes `const T &&`, and the move constructor takes
   `T &&`, so it is not chosen. It silently becomes a copy

**`error: cannot bind rvalue reference of type ‘T&&’ to lvalue of type ‘T’**
You passed a named variable to a function that takes `&&`. Add `std::move`.

**You used a variable you thought you had moved**
The compiler does not tell you (`bugprone-use-after-move` of `clang-tidy` can detect it).
Protect yourself with the rule: **below the line where you wrote `std::move`, do not write the name of that variable.**

**You wrote `&&` in a function argument and can no longer pass an lvalue**
`T &&` is for rvalues only. If you want to receive both, make 2 overloads, or use
`const T &`. A `T &&` with a template argument is called a "forwarding reference" and has a different meaning,
but it does not appear in the drill.

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 14 | `publisher_->publish(std::move(msg))` — the subject of this chapter. The test detects it by comparing addresses |
| 14 | `std::make_unique<std_msgs::msg::String>()` — creates a state with exactly one owner |
| 01, 15 | `publisher_->publish(message)` — the by-value version. **One copy happens** |
| All exercises | The members of message types are `std::string` / `std::vector`, so the Rule of Zero is working |

If you put exercise 01 and exercise 14 side by side, the point of this chapter appears as it is.

```cpp
// exercise 01: pass a value (1 copy)
auto message = std_msgs::msg::String();
message.data = "Hello, world! " + std::to_string(count_++);
publisher_->publish(message);

// exercise 14: pass ownership (0 copies)
auto msg = std::make_unique<std_msgs::msg::String>();
msg->data = ss.str();
publisher_->publish(std::move(msg));
```

**The way exercise 01 is written is not wrong.**
`std_msgs::msg::String` is small, so 1 copy is not a problem.
Matching the wording of the official tutorial is worth more.

The difference appears with messages of several MB such as `sensor_msgs::msg::PointCloud2`.
It is enough if you can make the judgment: **"switch to the `std::move` version only when you send large messages at a high frequency".**
There are 4 conditions for zero-copy to hold (the same process, `use_intra_process_comms(true)`,
the QoS History, and `std::move`), and `std::move` is only one of them.
The other 3 are in Chapter 6 of [rclcpp design philosophy](../rclcpp_design_philosophy.md).

## Matching exercise

After you read this chapter, practice with the matching drill.

- `cpp05_move` — move ownership

```bash
./drill run cpp05
```

From the exercise side, you can come back to this chapter with `./drill read`.

## References

- `/opt/ros/jazzy/include/rclcpp/rclcpp/publisher.hpp` — the 2 overloads of `publish`
- [rclcpp design philosophy](../rclcpp_design_philosophy.md) Chapter 6 — the conditions under which zero-copy holds and does not hold
- `cppreference` [std::move](https://en.cppreference.com/w/cpp/utility/move) and [Copy elision](https://en.cppreference.com/w/cpp/language/copy_elision)
- `cppreference` [Value categories](https://en.cppreference.com/w/cpp/language/value_category) — the strict definition of lvalue and rvalue (if you want to go as far as glvalue / xvalue / prvalue)

---

Previous → [4. References and const](04_references_and_const.md)
Next → [6. Smart pointers](06_smart_pointers.md)
