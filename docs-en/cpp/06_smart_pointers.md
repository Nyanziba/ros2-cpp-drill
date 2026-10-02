# 6. Smart pointers

> **Goal of this chapter**: `make_shared` appears **89 times** in the drill. It is the second most common syntax of all.
> And this is the first bug that beginners hit.
>
> ```cpp
> MinimalPublisher::MinimalPublisher() : Node("minimal_publisher")
> {
>   auto publisher = this->create_publisher<std_msgs::msg::String>("topic", 10);
>   //   ^^^^^^^^^ made a local variable instead of the member variable publisher_
> }
> ```
>
> **There is no error. There is no warning. Nothing is published, that's all.**
> This chapter explains why, using how `shared_ptr` and `weak_ptr` work.
> After this chapter, the rclcpp rule "always keep the return value in a member" will not be
> something to memorize. You will see it as a natural consequence.

> **Prerequisite**: This chapter assumes you understand raw pointers.
> If you are unsure, read [C++ Basics chapter 4 Pointers 1](../cpp-basics/04_pointers_1.md) and
> [chapter 5 Pointers 2](../cpp-basics/05_pointers_2.md) first.
> **If you do not understand `nullptr` and dereferencing, you cannot follow what `lock()` of `weak_ptr` does.**

## 6.1 What is wrong with raw pointers

```cpp
void leaky()
{
  int * p = new int(42);
  if (something_wrong) {
    return;              // forgot delete -> leak
  }
  may_throw();           // an exception is thrown -> leak
  delete p;
}
```

You must write `delete` on "every path that leaves the function".
If there are 3 `return` statements, you write it in 3 places. If an exception may be thrown, there are even more paths.

Remember RAII from chapter 2. **A destructor is always called.**
So if you wrap the pointer in "a class that calls `delete` in its destructor", forgetting `delete` becomes structurally impossible.
That is a smart pointer.

There are 3 kinds. You choose by **"how many owners there are"**.

| Type | Owners | Copy | Occurrences in the drill |
| --- | --- | --- | --- |
| `std::unique_ptr<T>` | **1** | Not allowed (move only) | 6 times |
| `std::shared_ptr<T>` | **Many** | Allowed (reference count) | 33 times + `make_shared` 89 times |
| `std::weak_ptr<T>` | **0** (only observes) | Allowed | 0 times (but rclcpp uses it internally) |

## 6.2 `unique_ptr` — one owner

```cpp
#include <iostream>
#include <memory>

int main()
{
  auto p = std::make_unique<int>(42);
  std::cout << *p << "\n";              // 42
  std::cout << p.get() << "\n";         // address

  // auto q = p;                        // error: cannot copy
  auto q = std::move(p);                // a move is allowed
  std::cout << (p == nullptr) << "\n";  // 1 (a moved-from pointer is guaranteed to be nullptr)

  return 0;                             // q deletes here
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/frYsvxP1T)

There is no `delete` in this code. The destructor of `q` does it.

You cannot copy it because `std::unique_ptr` uses the `= delete` you saw in chapter 2.

```cpp
unique_ptr(const unique_ptr &) = delete;
unique_ptr & operator=(const unique_ptr &) = delete;
```

**The design rule "one owner" is enforced by the type.**
If you accidentally try to make 2 owners, you get a compile error.

As written in chapter 5, a `unique_ptr` is **guaranteed by the standard to be `nullptr` after a move**.
This is a stronger guarantee than "valid but unspecified" for types such as `std::string`.

### `->`, `*`, and `.get()`

```cpp
auto p = std::make_unique<std_msgs::msg::String>();

p->data = "hello";        // a member of the pointed-to object (used most)
(*p).data = "hello";      // same meaning. Nobody writes this
std::string s = *p;       // the pointed-to object itself (a copy happens)
p.get();                  // get the raw pointer (does not give up ownership)
p.reset();                // release it right now
```

The difference between `->` and `.` is important.

- `p->data` — a member of the **pointed-to object** (`std_msgs::msg::String`)
- `p.get()` — a member function of the **smart pointer itself**

Both `msg.get()` and `msg->data` appear in exercise 14.

```cpp
// solutions/14_zero_copy/src/zero_copy_nodes.cpp
auto msg = std::make_unique<std_msgs::msg::String>();
ss << std::hex << reinterpret_cast<std::uintptr_t>(msg.get());   // <- look at the address
msg->data = ss.str();                                             // <- write the contents
```

**Do not `delete` a raw pointer you got from `.get()`.**
The smart pointer still owns the object, so this causes a double free.
Use `get()` only to "look" or to "pass to a C API".

### The difference between `make_unique` and `new`

```cpp
auto a = std::make_unique<Foo>(1, 2);           // recommended
std::unique_ptr<Foo> b(new Foo(1, 2));          // works, but verbose
```

There are 3 reasons to use `make_unique`.

1. You write the type only once (the line for `b` has `Foo` twice)
2. You do not write `new`, so you never have to look for `delete`
3. Exception safety (no leak even if an exception is thrown while evaluating arguments)

**For `shared_ptr` there is one more reason in addition to the third.** The next section covers it.

## 6.3 `shared_ptr` — reference counting

```cpp
#include <iostream>
#include <memory>

int main()
{
  auto a = std::make_shared<int>(42);
  std::cout << "a only:      " << a.use_count() << "\n";   // 1

  {
    auto b = a;                                             // copy (count +1)
    std::cout << "a and b:    " << a.use_count() << "\n";   // 2

    auto c = a;
    std::cout << "a, b, c:    " << a.use_count() << "\n";   // 3
  }   // b and c disappear here

  std::cout << "back:        " << a.use_count() << "\n";    // 1
  return 0;                                                 // becomes 0 and deletes
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/vbfTzGWd7)

```
a only:      1
a and b:    2
a, b, c:    3
back:        1
```

**The object is deleted when the last owner disappears.**
The value is that a human does not have to track "who is last".

A `shared_ptr` points to 2 things.

```
  a ─┬─▶ control block { strong count=1, weak count=0, deleter }
     └─▶ object { 42 }
```

So a `shared_ptr` is twice the size of a raw pointer (16 bytes).
Changing the count is an atomic operation, so a copy is not free.
**If you only pass it to a function, use `const shared_ptr<T> &` or `const T &`**. This is lighter.

### Why use `make_shared`

```cpp
auto a = std::make_shared<Foo>();               // 1 allocation
std::shared_ptr<Foo> b(new Foo());              // 2 allocations
```

`make_shared` **allocates the object and the control block together in one allocation.**
The form with `new` allocates once for the object and once for the control block, 2 times in total.

**`make_shared` appears 89 times in the drill because this is the standard way to write it.**

```cpp
rclcpp::spin(std::make_shared<MinimalPublisher>());
auto request = std::make_shared<AddTwoInts::Request>();
```

(There is a trade-off. The object and the control block are in the same allocation,
so while a `weak_ptr` is still alive, the memory of the object part is not freed either.
This matters only when you keep a `weak_ptr` for a long time to a huge object. It does not matter in the drill.)

### Do not create two from the same raw pointer

```cpp
Foo * raw = new Foo();
std::shared_ptr<Foo> a(raw);
std::shared_ptr<Foo> b(raw);      // two control blocks are created -> double free
```

`a` and `b` each think "I am the only owner, with count 1".
When both leave scope, `delete` runs twice and the program crashes.

**If you use `make_shared`, you cannot cause this accident.** The raw pointer never appears.

### Circular reference — the only pattern where `shared_ptr` leaks

```cpp
#include <iostream>
#include <memory>

struct Node
{
  std::shared_ptr<Node> other;
  ~Node() { std::cout << "destroyed\n"; }
};

int main()
{
  auto a = std::make_shared<Node>();
  auto b = std::make_shared<Node>();
  a->other = b;     // a holds b
  b->other = a;     // b holds a
  std::cout << "leaving main\n";
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/nx83Tx6c3)

```
leaving main
```

**"destroyed" is not printed.** Both objects leaked.

Even after `main` ends and the variables `a` and `b` disappear,
the count of `a` is still 1 because `b->other` holds it, and the count of `b` is still 1 because `a->other` holds it.
Neither reaches 0, so they are never freed.

`weak_ptr` is what breaks this.

## 6.4 `weak_ptr` — observe without owning

A `weak_ptr` is a reference that lets you check "is it still there?" but **does not keep it alive**.

```cpp
#include <iostream>
#include <memory>

int main()
{
  std::weak_ptr<int> w;

  {
    auto s = std::make_shared<int>(42);
    w = s;                                          // weak reference (count does not increase)
    std::cout << "alive: use_count = " << s.use_count() << "\n";   // 1

    if (auto locked = w.lock()) {                   // temporarily promote to a strong reference
      std::cout << "lock succeeded: " << *locked << "\n";
    }
  }   // s disappears -> the object is deleted

  std::cout << "expired = " << w.expired() << "\n";  // 1

  if (auto locked = w.lock()) {
    std::cout << "lock succeeded\n";
  } else {
    std::cout << "lock failed (it is gone)\n";
  }
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/sbarEx94h)

```
alive: use_count = 1
lock succeeded: 42
expired = 1
lock failed (it is gone)
```

Check that the count stays 1 even after `w = s`. **A weak reference is not counted.**

Always use it through `lock()`.

```cpp
if (auto p = w.lock()) {
  // p is a shared_ptr. Inside this block it is guaranteed to be alive
}
```

`lock()` returns a `shared_ptr`. When it succeeds, the count goes up by 1 for a while.
**Avoid the style "check with `w.expired()` and then use it".**
Right after you check, another thread may free the object (TOCTOU).
`lock()` checks and gets at the same time, so it is safe.

You can break a circular reference by making one side a `weak_ptr`.

```cpp
struct Node
{
  std::shared_ptr<Node> child;
  std::weak_ptr<Node> parent;      // hold the parent weakly
};
```

The standard approach is to make it asymmetric: "the parent owns the child, and the child only observes the parent".

## 6.5 Why it does not work unless you keep the return value in a member

With the tools so far, we can explain the mystery at the top.

The return value of `create_publisher()` is a `shared_ptr`.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp
template<typename MessageT, ...>
std::shared_ptr<PublisherT>
create_publisher(const std::string & topic_name, const rclcpp::QoS & qos, ...);
```

And **rclcpp keeps it only as a `weak_ptr`.**

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/callback_group.hpp
for (auto & weak_ptr : vect_ptrs) {
  auto ref_ptr = weak_ptr.lock();
  ...
}
```

`add_publisher()` takes a `SharedPtr` as an argument,
but it converts it to a `weak_ptr` internally and keeps that.

So the ownership picture is this.

```
Your code (member variable publisher_)
   │  shared_ptr (strong reference, the only owner)
   ▼
Publisher<T>
   ▲
   │  weak_ptr (only checks alive or dead. Does not own)
CallbackGroup ◀───── weak_ptr ───── Executor
```

**The strong arrow comes only from "your code".**

So if you write

```cpp
auto publisher = this->create_publisher<...>("topic", 10);   // local variable
```

the count becomes 0 the moment you leave the constructor, and the `Publisher` is destroyed.
rclcpp holds only a `weak_ptr`, so `lock()` returns `nullptr` every time.
The Executor decides "that entity no longer exists", and **runs nothing.**

There is no error and no warning. **This is not a "careless mistake".
It is the natural consequence in C++ that "something without an owner disappears".**

The same structure exists between a node and the Executor.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/executors/executor_entities_collector.hpp
using NodeCollection = std::set<
  rclcpp::node_interfaces::NodeBaseInterface::WeakPtr,
  std::owner_less<rclcpp::node_interfaces::NodeBaseInterface::WeakPtr>>;
NodeCollection weak_nodes_ RCPPUTILS_TSA_GUARDED_BY(mutex_);
```

**The Executor does not own nodes either.** So in

```cpp
rclcpp::spin(std::make_shared<MinimalPublisher>());
```

the return value of `make_shared` is valid only while it lives as the argument of `spin()`.
The node stays alive because `spin` takes the `shared_ptr` by value.

The same trap waits in exercise 07.

```cpp
// comment in ParameterEventHandler
// Note: the object returned from add_parameter_callback must be captured
// or the callback will [be unregistered].
```

If you do not keep the return value of `add_parameter_callback` (`ParameterCallbackHandle::SharedPtr`),
the callback is unregistered right after you register it. **Same design, same trap.**

**You only need to remember one rule. If rclcpp returns a `shared_ptr`, keep it in a member.**

## 6.6 Type aliases in rclcpp

rclcpp classes have an alias called `SharedPtr`.

```cpp
rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
rclcpp::TimerBase::SharedPtr timer_;
rclcpp::Node::SharedPtr node;
std_msgs::msg::String::ConstSharedPtr msg;
```

This is **exactly the same thing** as `std::shared_ptr<...>`.

```cpp
// these 2 are the same type
rclcpp::Publisher<std_msgs::msg::String>::SharedPtr a;
std::shared_ptr<rclcpp::Publisher<std_msgs::msg::String>> b;
```

It comes from a macro.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/macros.hpp:36
#define RCLCPP_SMART_PTR_DEFINITIONS(...) \
  RCLCPP_SHARED_PTR_DEFINITIONS(__VA_ARGS__) \
  RCLCPP_WEAK_PTR_DEFINITIONS(__VA_ARGS__) \
  RCLCPP_UNIQUE_PTR_DEFINITIONS(__VA_ARGS__)
```

This is the first line of the `public` section of `rclcpp::Node`.

```cpp
// node.hpp:82
RCLCPP_SMART_PTR_DEFINITIONS(Node)
```

It defines the aliases `SharedPtr` / `WeakPtr` / `UniquePtr` / `ConstSharedPtr`, and
a static function `Node::make_shared(...)`, all at once.

Think of it as **a shorthand for writing `std::shared_ptr<...>` at length**.
Nested templates quickly stop fitting in the 100-character line limit, so this is a practical trick.

### `enable_shared_from_this`

Back to the `Node` declaration you saw in chapter 3.

```cpp
// node.hpp:79
class Node : public std::enable_shared_from_this<Node>
```

If you inherit from this, you can get **a `shared_ptr` to yourself** inside a member function.

```cpp
void MyNode::register_somewhere()
{
  some_registry.add(shared_from_this());   // a shared_ptr to myself
}
```

Why is it needed? `this` is a raw pointer, so you must not make a `shared_ptr` from it.

```cpp
some_registry.add(std::shared_ptr<Node>(this));   // never do this
```

A new control block is created, and you get the double free from section 6.3.
`shared_from_this()` **finds the existing control block** and adds 1 to the count.

**One caution. If you call `shared_from_this()` on an object that was not created as a `shared_ptr`,
an exception (`std::bad_weak_ptr`) is thrown.**

```cpp
MinimalPublisher node;                              // on the stack
node.some_method_calling_shared_from_this();        // std::bad_weak_ptr

auto node = std::make_shared<MinimalPublisher>();   // this is OK
```

You also cannot call it inside the constructor (the control block is not attached yet).

**This is one more reason "always create a node with `make_shared`".**
The idiom `rclcpp::spin(std::make_shared<MinimalPublisher>())` is required both by
the no-copy rule from chapter 3 and by this `enable_shared_from_this`.

## Try it yourself

**You will see the counts with your own eyes, and create and break a circular reference yourself.**

```cpp
// ptr.cpp
#include <iostream>
#include <memory>
#include <string>

struct Resource
{
  std::string name;
  explicit Resource(std::string n) : name(std::move(n))
  {
    std::cout << "  [" << name << "] allocated\n";
  }
  ~Resource() { std::cout << "  [" << name << "] released\n"; }
};

void observe(const std::weak_ptr<Resource> & w, const char * tag)
{
  if (auto p = w.lock()) {
    std::cout << tag << ": alive (" << p->name << ")\n";
  } else {
    std::cout << tag << ": gone\n";
  }
}

int main()
{
  std::cout << "-- unique_ptr\n";
  {
    auto u = std::make_unique<Resource>("unique");
    // auto copy = u;                 // ① what error do you get if you remove the comment
    auto moved = std::move(u);
    std::cout << "  is u null: " << (u == nullptr) << "\n";
  }

  std::cout << "\n-- shared_ptr count\n";
  std::weak_ptr<Resource> w;
  {
    auto a = std::make_shared<Resource>("shared");
    w = a;
    std::cout << "  a only:          " << a.use_count() << "\n";
    {
      auto b = a;
      std::cout << "  a and b:         " << a.use_count() << "\n";
      observe(w, "  inner");
    }
    std::cout << "  after b is gone: " << a.use_count() << "\n";
  }
  observe(w, "  outer");

  std::cout << "\n-- circular reference\n";
  struct Cycle
  {
    std::string name;
    std::shared_ptr<Cycle> other;      // ② what happens if you change this to weak_ptr
    explicit Cycle(std::string n) : name(std::move(n)) {}
    ~Cycle() { std::cout << "  [" << name << "] released\n"; }
  };
  {
    auto x = std::make_shared<Cycle>("X");
    auto y = std::make_shared<Cycle>("Y");
    x->other = y;
    y->other = x;
    std::cout << "  x.use_count = " << x.use_count() << "\n";
    std::cout << "  leaving the scope\n";
  }
  std::cout << "  left the scope\n";
  return 0;
}
```

**Make 4 predictions.**

1. Right after `w = a`, is `use_count` 1 or 2?
2. Does the outer `observe` print "alive" or "gone"?
3. What is `x.use_count`?
4. When you leave the block with the circular reference, are `[X] released` and `[Y] released` printed?

```bash
g++ -std=c++17 -Wall -Wextra ptr.cpp -o ptr && ./ptr
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/dq5M564e8)

After you check, make 2 changes.

**① Remove the comment from `auto copy = u;`.**
Read the error message. You will see `use of deleted function`.
This is the real thing: "one owner" is enforced by the type.

**② Change `std::shared_ptr<Cycle> other;` to `std::weak_ptr<Cycle> other;`.**
`x->other = y;` still compiles (you can assign a `shared_ptr` to a `weak_ptr`).
How does the output change? What happens to `use_count`?

Finally, **we reproduce the bug that is the subject of this chapter.**
This simulates the picture from section 6.5 without rclcpp.

```cpp
// weakbug.cpp
#include <iostream>
#include <memory>
#include <vector>

struct Timer
{
  int id;
  explicit Timer(int i) : id(i) {}
  void fire() const { std::cout << "  timer " << id << " fired\n"; }
};

class Executor
{
public:
  void add(const std::shared_ptr<Timer> & t) { timers_.push_back(t); }   // keeps it as weak

  void spin_once()
  {
    for (const auto & w : timers_) {
      if (auto t = w.lock()) {
        t->fire();
      } else {
        std::cout << "  (skipped one entity that is gone)\n";
      }
    }
  }

private:
  std::vector<std::weak_ptr<Timer>> timers_;      // <- same as rclcpp
};

class GoodNode
{
public:
  explicit GoodNode(Executor & e)
  {
    timer_ = std::make_shared<Timer>(1);
    e.add(timer_);
  }
private:
  std::shared_ptr<Timer> timer_;                 // keep it in a member
};

class BadNode
{
public:
  explicit BadNode(Executor & e)
  {
    auto timer = std::make_shared<Timer>(2);     // made it a local variable
    e.add(timer);
  }
};

int main()
{
  Executor exec;
  GoodNode good(exec);
  BadNode bad(exec);
  std::cout << "spin_once:\n";
  exec.spin_once();
  return 0;
}
```

**Predict: which fires, `timer 1` or `timer 2`?**

```bash
g++ -std=c++17 -Wall -Wextra weakbug.cpp -o weakbug && ./weakbug
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/MWqcvGeYz)

`BadNode` produces no error and no warning. **This is the real cause of the most common pitfall in exercise 01.**
Fix `BadNode` and check that both timers fire.

## Common pitfalls

**`error: use of deleted function ‘std::unique_ptr<...>::unique_ptr(const std::unique_ptr<...>&)’`**
You are trying to copy a `unique_ptr`. Add `std::move`, or use a `shared_ptr`.

**`error: invalid use of incomplete type`**
You use `*p` or `p->x` on a smart pointer to a type that only has a forward declaration (`class Foo;`).
`#include` the full definition of that type.
`unique_ptr` needs the complete type in its destructor, so even if you only hold it as a member,
you may need to write the destructor on the `.cpp` side.

**A segmentation fault happened**
There are 3 common causes.

1. You used a pointer after a move (`p` is `nullptr`)
2. You used the result of `lock()` without checking it
3. You used a raw pointer from `.get()` after the smart pointer died

You can see a crash through `nullptr` at a glance with `gdb`.

```bash
gdb --batch -ex run -ex bt ./your_binary
```

**`std::bad_weak_ptr` was thrown**
You called `shared_from_this()` on an object that was not created as a `shared_ptr`.
Or you called it inside the constructor. See section 6.6.

**`use_count()` is not the value you expected**
Do not use `use_count()` except for debugging.
The check "if it is 1, it is only me" has a race in multi-threaded code.
In the drill, `expired()` / `lock()` are enough.

**You publish, but `ros2 topic echo` shows nothing**
This is section 6.5 of this chapter. First check whether the return values of `create_publisher` / `create_wall_timer`
are stored in member variables.
A QoS incompatibility (like chapter 12; exercise 12 in the drill) gives the same symptom,
but **not keeping the return value is by far the more common cause.**

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 01 | Keep `publisher_` / `timer_` in members. **If you miss this, nothing happens** |
| 02 | Keep `subscription_` in a member |
| 04, 05 | Keep `service_` / `client_` in members. `std::make_shared<AddTwoInts::Request>()` |
| 05 | Receive the return value of `async_send_request` (a future) |
| 07 | You must keep **both** `ParameterEventHandler` and `ParameterCallbackHandle` |
| 10 | Receive the action `GoalHandle` as a `shared_ptr` |
| 13 | Keep `rclcpp::CallbackGroup::SharedPtr` in a member |
| 14 | `std::make_unique` -> publish with `std::move`. Look at the address with `msg.get()` |
| All exercises | `rclcpp::spin(std::make_shared<...>())` |

Exercise 07 is the harshest example of "forgetting to keep". **You must keep 2 things.**

```cpp
private:
  std::shared_ptr<rclcpp::ParameterEventHandler> param_subscriber_;   // ①
  std::shared_ptr<rclcpp::ParameterCallbackHandle> cb_handle_;        // ②
```

If you keep only ① and drop ②, the callback is unregistered right after it is registered.
If you keep only ② and drop ①, the subscription to `/parameter_events` itself disappears.
**Neither gives an error. The callback is just never called.**

This is what the README of exercise 07 means when it says "the biggest trap this time".
If you understand the picture in section 6.5, you can avoid it mechanically with "keep every `shared_ptr` that rclcpp returns in a member".

## Matching exercise

After reading this chapter, practice with the matching drill exercise.

- `cpp06_smart_pointers` — express ownership with smart pointers

```bash
./drill run cpp06
```

From the exercise, you can come back to this chapter with `./drill read`.

## References

- `/opt/ros/jazzy/include/rclcpp/rclcpp/callback_group.hpp` — holds entities as `weak_ptr`
- `/opt/ros/jazzy/include/rclcpp/rclcpp/executors/executor_entities_collector.hpp` — nodes are also `weak_ptr`
- `/opt/ros/jazzy/include/rclcpp/rclcpp/macros.hpp` — `RCLCPP_SMART_PTR_DEFINITIONS` (line 36)
- `/opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp` — `class Node : public std::enable_shared_from_this<Node>` (line 79)
- [rclcpp design philosophy](../rclcpp_design_philosophy.md) chapter 3 — "Create everything and hold it with shared_ptr". It explains the same story from the rclcpp header side
- `cppreference` [std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) and [std::weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)

---

Previous chapter → [5. Move and ownership](05_move_and_ownership.md)
Next chapter → [7. Lambdas and `std::bind`](07_lambdas_and_std_bind.md)
