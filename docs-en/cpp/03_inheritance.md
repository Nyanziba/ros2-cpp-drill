# 3. Inheritance — what does it mean to inherit `rclcpp::Node`?

> **Goal of this chapter**: All 15 exercises in the drill start with this one line.
>
> ```cpp
> class MinimalPublisher : public rclcpp::Node
> ```
>
> If you cannot read this line, these questions all stay a mystery: "why can I call `create_publisher` without writing it myself",
> "what is `: Node("minimal_publisher")`", and "why do we create it with `std::make_shared<MinimalPublisher>()`".
> This chapter starts by reading that one line completely from left to right,
> and then goes on to **virtual functions**, **virtual destructors**, **copy prohibition**, and **slicing**.
> It is the C++ feature you use most in the drill, so this chapter is longer than the others.

## 3.1 Reading the one line

```cpp
class MinimalPublisher : public rclcpp::Node
```

| Part | Meaning |
| --- | --- |
| `class MinimalPublisher` | Define a class named `MinimalPublisher` |
| `:` | "Inherit the following". **It is different from the `:` of the initializer list in Chapter 2** |
| `public` | **How to inherit.** The `public` members of the base class stay `public` in the derived class |
| `rclcpp::Node` | The base class. `Node` in the `rclcpp` namespace |

Let us fix the terms.

- **Base class** = `rclcpp::Node`. Also called a parent class or a superclass
- **Derived class** = `MinimalPublisher`. Also called a child class or a subclass

**Notice that there are 2 kinds of `:`.** If you mix them up, you cannot read the code.

```cpp
class MinimalPublisher : public rclcpp::Node    // <- ① the : for inheritance (in the class declaration)
{
  MinimalPublisher();
};

MinimalPublisher::MinimalPublisher()
: Node("minimal_publisher"), count_(0)          // <- ② the : for the initializer list (in the definition)
{
}
```

① says "what to inherit", and ② says "how to initialize the base class and the members".
They appear in different places, for different purposes.

## 3.2 What happens when you inherit

A derived class that inherits **has the members of the base class as its own.**

```cpp
// greet.cpp
#include <iostream>
#include <string>

class Base
{
public:
  explicit Base(std::string name) : name_(std::move(name)) {}

  void greet() const { std::cout << "Hello, " << name_ << "\n"; }
  const std::string & name() const { return name_; }

private:
  std::string name_;
};

class Derived : public Base
{
public:
  Derived() : Base("derived") {}

  void work() const
  {
    greet();                                    // the base member function can be called as it is
    std::cout << name() << " works\n";
  }
};

int main()
{
  Derived d;
  d.work();
  d.greet();      // it can be called from outside too (because of public inheritance)
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/MPbe145s9)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic greet.cpp -o greet && ./greet" -->
```
Hello, derived
derived works
Hello, derived
```

`Derived` does not write `greet()` at all, but it can call it. This is inheritance.

In the drill, `MinimalPublisher` does not write `create_publisher`, `get_logger`, or
`create_wall_timer`, but they all come from `rclcpp::Node`.

```cpp
publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);
//            ^^^^^^^^^^^^^^^^^^^^^ a member function of rclcpp::Node
RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
//                ^^^^^^^^^^ this is also a member function of rclcpp::Node
```

**`this->` is also there to make it clear that these are "members I did not write myself"**
(it can be omitted, as in Chapter 2).

To know what `rclcpp::Node` provides, the sure way is to look at the header directly.

```bash
grep -n "^  [a-z].*(" /opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp | head -40
```

### `private` members are not visible even if you inherit

Even if you inherit, you cannot touch the `private` members of the base class.

```cpp
class Derived : public Base
{
public:
  void bad() { std::cout << name_; }   // error
};
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// private.cpp
#include <iostream>
#include <string>

class Base
{
public:
  explicit Base(std::string name) : name_(std::move(name)) {}

  void greet() const { std::cout << "Hello, " << name_ << "\n"; }
  const std::string & name() const { return name_; }

private:
  std::string name_;
};

class Derived : public Base
{
public:
  void bad() { std::cout << name_; }   // error
};

int main()
{
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic private.cpp -o private
```

</details>

<!-- measure: -->
```
private.cpp: In member function ‘void Derived::bad()’:
private.cpp:20:29: error: ‘std::string Base::name_’ is private within this context
   20 |   void bad() { std::cout << name_; }   // error
      |                             ^~~~~
private.cpp:14:15: note: declared private here
   14 |   std::string name_;
      |               ^~~~~
```

In `rclcpp::Node` too, all the real data is `private`.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp:1633 onward
private:
  RCLCPP_DISABLE_COPY(Node)

  rclcpp::node_interfaces::NodeBaseInterface::SharedPtr node_base_;
  rclcpp::node_interfaces::NodeGraphInterface::SharedPtr node_graph_;
  // ...
```

So `MinimalPublisher` cannot touch `node_base_`.
If you want to touch it, use the `public` `get_node_base_interface()`.
**"Data is private, operations are public" is followed, so
even if you inherit, you cannot break the inside of the base class.**

### `protected` — show it only to derived classes

The third access level is `protected`. "Not visible from outside, but visible from derived classes".

| Specifier | From outside | From a derived class |
| --- | --- | --- |
| `public` | yes | yes |
| `protected` | no | yes |
| `private` | no | no |

`rclcpp::Node` also has a `protected` section.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp:1620
protected:
  /// Construct a sub-node, which will extend the namespace of all entities created with it.
  Node(
    const Node & other,
    const std::string & sub_namespace);
```

This is the constructor for making a sub-node, and **it can be called only from a class that inherits `Node`.**
It prevents someone outside from making sub-nodes freely.

## 3.3 The base class constructor is always called

When you create a derived class, **the base class constructor runs first.**

```cpp
// ctor_order.cpp
#include <iostream>

class Base
{
public:
  Base() { std::cout << "Base()\n"; }
  ~Base() { std::cout << "~Base()\n"; }
};

class Derived : public Base
{
public:
  Derived() { std::cout << "Derived()\n"; }
  ~Derived() { std::cout << "~Derived()\n"; }
};

int main()
{
  Derived d;
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/en1aGdn4M)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic ctor_order.cpp -o ctor_order && ./ctor_order" -->
```
Base()
Derived()
~Derived()
~Base()
```

**Construction is base -> derived, and destruction is derived -> base.** It is the reverse order.

This makes sense. The constructor of the derived class can use the members of the base class,
so the base must be completed first. Destruction is the opposite:
the base is destroyed after the derived class has finished using it.

### A base class that needs arguments is called in the initializer list

In the example above, `Base()` had no arguments, so it was called automatically.
**If arguments are needed, you must write it yourself.**

Look at the constructor of `rclcpp::Node`.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp:91
explicit Node(
  const std::string & node_name,
  const NodeOptions & options = NodeOptions());
```

**There is no constructor without arguments.** The node name is required (of course. You cannot make a node without a name).
So the only way is to pass it in the initializer list.

```cpp
MinimalPublisher::MinimalPublisher()
: Node("minimal_publisher"), count_(0)
//^^^^^^^^^^^^^^^^^^^^^^^^^ a call to the base class constructor
{
}
```

If you forget to write `Node(...)`, the compiler tries to "call `Node()` with no arguments" and fails.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// drill/minimal_publisher.hpp
#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

class MinimalPublisher : public rclcpp::Node
{
public:
  MinimalPublisher();

private:
  void timer_callback();

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  std::size_t count_;
};
```

```cpp
// minimal_publisher.cpp
#include "drill/minimal_publisher.hpp"

MinimalPublisher::MinimalPublisher()
: count_(0)   // forgot Node(...)
{
}
```

```bash
g++ -std=c++17 -I. -c minimal_publisher.cpp -o mp.o   # the rclcpp include paths (-I) are omitted
```

</details>

<!-- measure: env=ros files=drill/minimal_publisher.hpp,minimal_publisher.cpp cmd="g++ -std=c++17 -I. $(find /opt/ros/jazzy/include -maxdepth 1 -mindepth 1 -type d -printf '-I%p ') -c minimal_publisher.cpp -o mp.o" filter="grep -oE '(error|note):.*'" -->
```
error: no matching function for call to ‘rclcpp::Node::Node()’
note: candidate: ‘rclcpp::Node::Node(const rclcpp::Node&, const std::string&)’
note:   candidate expects 2 arguments, 0 provided
note: candidate: ‘rclcpp::Node::Node(const std::string&, const std::string&, const rclcpp::NodeOptions&)’
note:   candidate expects 3 arguments, 0 provided
note: candidate: ‘rclcpp::Node::Node(const std::string&, const rclcpp::NodeOptions&)’
note:   candidate expects 2 arguments, 0 provided
```

> This output needs rclcpp, so it is an excerpt measured in an environment with ROS 2 (the repository's Docker image). Unlike the other examples in the C++ track, you cannot reproduce it with `g++` alone.

**If "candidate expects N arguments, 0 provided" is listed for each `Node` constructor, you forgot to call the base class.**

The order to write is **base class first, members after**. The real initialization order is also like that.
If you have `-Wall` on, writing them in the opposite order gives a `-Wreorder` warning.

### The manner of passing `NodeOptions` straight through

The constructors of exercises 14 and 15 have this form.

```cpp
// solutions/15_composition/src/composable_talker.cpp
ComposableTalker::ComposableTalker(const rclcpp::NodeOptions & options)
: Node("composable_talker", options), count_(0)
```

It passes the received `options` **straight to the base**. If you write this part as

```cpp
: Node("composable_talker")     // threw away options
```

the compile passes, `ros2 run` works, and **it breaks only when you load it as a component.**
This is because `options` contains `use_intra_process_comms` and parameter overrides,
and you threw them away.

The test of exercise 14 is built to detect this.
It is the "compiles but fails" type, so you cannot tell the cause from the error message.
Remember it as a rule: **always pass a `NodeOptions` you receive as an argument straight through to the base.**

## 3.4 Virtual functions — let the derived class replace the processing

Until now, inheritance was only "borrowing the features of the base".
The other use of inheritance is **to let the derived class put the contents into a frame decided by the base class.**

```cpp
// shapes.cpp
#include <iostream>
#include <memory>
#include <vector>

class Shape
{
public:
  virtual ~Shape() = default;                  // explained in Section 3.5
  virtual double area() const = 0;             // = 0 means "the derived class writes the contents"
  virtual const char * name() const { return "shape"; }   // has a default implementation
};

class Circle : public Shape
{
public:
  explicit Circle(double r) : r_(r) {}
  double area() const override { return 3.14159 * r_ * r_; }
  const char * name() const override { return "circle"; }
private:
  double r_;
};

class Square : public Shape
{
public:
  explicit Square(double s) : s_(s) {}
  double area() const override { return s_ * s_; }
  // name() is not overridden -> the base "shape" is used
private:
  double s_;
};

int main()
{
  std::vector<std::unique_ptr<Shape>> shapes;
  shapes.push_back(std::make_unique<Circle>(1.0));
  shapes.push_back(std::make_unique<Square>(2.0));

  for (const auto & s : shapes) {
    std::cout << "area of " << s->name() << " = " << s->area() << "\n";
  }
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/d9d5713hP)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic shapes.cpp -o shapes && ./shapes" -->
```
area of circle = 3.14159
area of shape = 4
```

**We hold only `Shape` pointers, but the `area()` of `Circle` and the `area()` of `Square`
are each called.** This is a virtual function.

Let us sort out the terms.

| How it is written | Name | Meaning |
| --- | --- | --- |
| `virtual double area() const = 0;` | **Pure virtual function** | It has no body. A derived class must implement it |
| `virtual const char * name() const { ... }` | **Virtual function** | It has a default implementation. A derived class may replace it |
| `double area() const override` | **Override** | Replaces a virtual function of the base |

A class that has even one pure virtual function is called an **abstract class**, and **you cannot create an instance of it.**

```cpp
Shape s;   // error
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// abstract.cpp
#include <iostream>
#include <memory>
#include <vector>

class Shape
{
public:
  virtual ~Shape() = default;                  // explained in Section 3.5
  virtual double area() const = 0;             // = 0 means "the derived class writes the contents"
  virtual const char * name() const { return "shape"; }   // has a default implementation
};

class Circle : public Shape
{
public:
  explicit Circle(double r) : r_(r) {}
  double area() const override { return 3.14159 * r_ * r_; }
  const char * name() const override { return "circle"; }
private:
  double r_;
};

class Square : public Shape
{
public:
  explicit Square(double s) : s_(s) {}
  double area() const override { return s_ * s_; }
  // name() is not overridden -> the base "shape" is used
private:
  double s_;
};

int main()
{
  Shape s;   // error
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic abstract.cpp -o abstract
```

</details>

<!-- measure: -->
```
abstract.cpp: In function ‘int main()’:
abstract.cpp:36:9: error: cannot declare variable ‘s’ to be of abstract type ‘Shape’
   36 |   Shape s;   // error
      |         ^
abstract.cpp:6:7: note:   because the following virtual functions are pure within ‘Shape’:
    6 | class Shape
      |       ^~~~~
abstract.cpp:10:18: note:     ‘virtual double Shape::area() const’
   10 |   virtual double area() const = 0;             // = 0 means "the derived class writes the contents"
      |                  ^~~~
```

It is a tool for "deciding only the interface and making the implementation replaceable".

### Always write `override`

It works without `override`. **Write it anyway.**

`override` is a declaration that says "I intend this to replace a virtual function of the base", and
it is an instruction to **make it a compile error if that is not true.**

```cpp
class Circle : public Shape
{
public:
  double area() override { return 3.14159 * r_ * r_; }   // forgot const
};
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// override.cpp
#include <iostream>
#include <memory>
#include <vector>

class Shape
{
public:
  virtual ~Shape() = default;                  // explained in Section 3.5
  virtual double area() const = 0;             // = 0 means "the derived class writes the contents"
  virtual const char * name() const { return "shape"; }   // has a default implementation
};

class Circle : public Shape
{
public:
  explicit Circle(double r) : r_(r) {}
  double area() override { return 3.14159 * r_ * r_; }   // forgot const
  const char * name() const override { return "circle"; }
private:
  double r_;
};

int main()
{
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic override.cpp -o override
```

</details>

<!-- measure: -->
```
override.cpp:18:10: error: ‘double Circle::area()’ marked ‘override’, but does not override
   18 |   double area() override { return 3.14159 * r_ * r_; }   // forgot const
      |          ^~~~
```

What happens if you do not write `override`? **It is not an error.**
`area() const` (base) and `area()` (derived) have different signatures,
so it passes as "a completely new function that does not override".
Then, if you call through `Shape *`, the base `area() const` is called,
and because it is pure virtual... in this case the class stays abstract and the compile fails,
but if the base has a default implementation, **a wrong function is called silently.**

Forgetting `const`, a one-character difference in an argument type, the presence or absence of `&`.
All of these become "a different function". `override` is the only way to detect them mechanically.

In the same way, when you **do not want to derive any further**, add `final`.

```cpp
double area() const override final { ... }
```

### Pure virtual functions inside ROS 2

In the drill, there are few places where you write virtual functions yourself (`override` appears
in only 2 places in `tools/drill_harness.hpp`), but **the ROS 2 ecosystem runs on this.**

| What | Base class | Who writes the derived class |
| --- | --- | --- |
| Planner plugin | `mbf_simple_core::SimplePlanner` | The implementer of a path planning algorithm |
| Controller plugin | `mbf_simple_core::SimpleController` | The implementer of tracking control |
| Hardware interface | `hardware_interface::SystemInterface` | The implementer of a real robot driver |
| A ros2_control controller | `controller_interface::ControllerInterface` | The implementer of a controller |
| Executor | `rclcpp::Executor` | rclcpp itself (`SingleThreadedExecutor` and so on) |

`pluginlib` does `dlopen` on a shared library at run time, and
**receives the instance as a pointer to the base class.** This is the mechanism.
The structure is the same as the `std::vector<std::unique_ptr<Shape>>` above,
and only the class that corresponds to `Circle` is decided at run time.

The planner and controller plugins of `move_base_flex` have exactly this form,
so **when you get to the stage of reading autonomous navigation code, come back to this section.**

### How virtual functions are implemented

You can write them without knowing this, but it is easier to accept if you know, so let us touch on it.

An object of a class with virtual functions gets **one hidden pointer called a vptr** added.
What it points to is a **vtable**, a table that says "which function is `area()` for this type".

```
Circle object                  Circle vtable
+----------------+           +---------------------------+
| vptr           |─────────▶ | ~Circle                   |
+----------------+           | Circle::area              |
| r_ (double)    |           | Circle::name              |
+----------------+           +---------------------------+
```

`s->area()` becomes an indirect call: "follow the vptr of `s` and call the 2nd entry of the table".
You can check it.

```cpp
// sizeof_virtual.cpp
#include <iostream>
struct NoVirtual { double x; };
struct WithVirtual { virtual ~WithVirtual() = default; double x; };

int main()
{
  std::cout << sizeof(NoVirtual) << " " << sizeof(WithVirtual) << "\n";
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/TGbb14c3P)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic sizeof_virtual.cpp -o sizeof_virtual && ./sizeof_virtual" -->
```
8 16
```

One `double` should be 8 bytes, but with a virtual function it became 16 bytes.
The 8 bytes of the vptr were added.

So **`virtual` is not free.**
The object grows by 8 bytes, the call goes through one indirection, and it is less likely to be inlined.
That said, this difference matters only at a scale of "tens of millions of calls per second".
At a plugin boundary, with a few calls per control cycle, you can ignore it.

## 3.5 Virtual destructors — if you forget them, you leak silently

In the `Shape` example we wrote `virtual ~Shape() = default;`.
Let us actually see what happens if you remove it.

```cpp
// no_virtual_destructor.cpp
#include <iostream>
#include <memory>

class Base
{
public:
  ~Base() { std::cout << "~Base()\n"; }        // virtual is not added
};

class Derived : public Base
{
public:
  ~Derived() { std::cout << "~Derived()\n"; }
};

int main()
{
  std::unique_ptr<Base> p = std::make_unique<Derived>();
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/8Mq87fYMr)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic no_virtual_destructor.cpp -o no_virtual_destructor && ./no_virtual_destructor" -->
```
~Base()
```

**`~Derived()` is not called.**
The resources that `Derived` had acquired are never released.

If you make it `virtual ~Base()`, it is fixed.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// vdtor.cpp
#include <iostream>
#include <memory>

class Base
{
public:
  virtual ~Base() { std::cout << "~Base()\n"; }
};

class Derived : public Base
{
public:
  ~Derived() { std::cout << "~Derived()\n"; }
};

int main()
{
  std::unique_ptr<Base> p = std::make_unique<Derived>();
  return 0;
}
```

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic vdtor.cpp -o vdtor && ./vdtor
```

</details>

<!-- measure: -->
```
~Derived()
~Base()
```

The rule is simple. **Make the destructor of a class that may be inherited `virtual`.**
Of course `rclcpp::Node` does this.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp:109
virtual ~Node();
```

Because of this, when a `MinimalPublisher` held as a `std::shared_ptr<rclcpp::Node>`
is destroyed, `~MinimalPublisher()` is called correctly.
`rclcpp::spin(std::make_shared<MinimalPublisher>())` works thanks to this.

### `shared_ptr` helps you as an exception

It is a detail, but it is a source of confusion, so let us state it clearly.

`std::shared_ptr` remembers the destructor of **the type it was created with**.

```cpp
std::shared_ptr<Base> p = std::make_shared<Derived>();   // ~Derived() is called even without virtual
std::unique_ptr<Base> q = std::make_unique<Derived>();   // only ~Base()
```

This happens because `shared_ptr` holds a type-erased deleter inside.
**But do not depend on this.** It does not work with `unique_ptr`, and
it does not work with a `delete` on a raw pointer either. Writing `virtual ~` is the right answer.

## 3.6 Copy prohibition and slicing — why everything is a `shared_ptr`

Look at the first line of the `private` section of `rclcpp::Node`.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp:1634
private:
  RCLCPP_DISABLE_COPY(Node)
```

The contents of this macro are as follows.

```cpp
// /opt/ros/jazzy/include/rclcpp/rclcpp/macros.hpp:26
#define RCLCPP_DISABLE_COPY(...) \
  __VA_ARGS__(const __VA_ARGS__ &) = delete; \
  __VA_ARGS__ & operator=(const __VA_ARGS__ &) = delete;
```

When expanded, it is

```cpp
Node(const Node &) = delete;
Node & operator=(const Node &) = delete;
```

**`rclcpp::Node` cannot be copied.** This is the `= delete` we saw in Chapter 2.

Of course. If you copied a node, it is not decided whether there would be 2 DDS participants
or 2 nodes with the same name appearing.

And `MinimalPublisher` inherits `Node`, so
**`MinimalPublisher` automatically becomes non-copyable too.**
The copy constructor is about to be generated automatically, but the copy of the base is `delete`d, so it fails.

```cpp
MinimalPublisher a;
MinimalPublisher b = a;    // error
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// drill/minimal_publisher.hpp
#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

class MinimalPublisher : public rclcpp::Node
{
public:
  MinimalPublisher();

private:
  void timer_callback();

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  std::size_t count_;
};
```

```cpp
// minimal_publisher.cpp
#include "drill/minimal_publisher.hpp"

void copy_it()
{
  MinimalPublisher a;
  MinimalPublisher b = a;    // error
}
```

```bash
g++ -std=c++17 -I. -c minimal_publisher.cpp -o mp.o   # the rclcpp include paths (-I) are omitted
```

</details>

<!-- measure: env=ros files=drill/minimal_publisher.hpp,minimal_publisher.cpp cmd="g++ -std=c++17 -I. $(find /opt/ros/jazzy/include -maxdepth 1 -mindepth 1 -type d -printf '-I%p ') -c minimal_publisher.cpp -o mp.o" filter="grep -oE '(error|note):.*'" -->
```
error: use of deleted function ‘MinimalPublisher::MinimalPublisher(const MinimalPublisher&)’
note: ‘MinimalPublisher::MinimalPublisher(const MinimalPublisher&)’ is implicitly deleted because the default definition would be ill-formed:
error: use of deleted function ‘rclcpp::Node::Node(const rclcpp::Node&)’
note: declared here
note: in definition of macro ‘RCLCPP_DISABLE_COPY’
```

> This output needs rclcpp, so it is an excerpt measured in an environment with ROS 2 (the repository's Docker image). Unlike the other examples in the C++ track, you cannot reproduce it with `g++` alone.

**This is the direct answer to "why is everything in ROS 2 code a `shared_ptr`".**
You cannot pass it around as a value, so you have no choice but to handle it with a pointer.

```cpp
rclcpp::spin(std::make_shared<MinimalPublisher>());
```

### Slicing — a trap when copying is possible

What would happen if copying were not prohibited? It is worth knowing this too.

```cpp
// slicing.cpp
#include <iostream>

class Base
{
public:
  virtual const char * name() const { return "Base"; }
};

class Derived : public Base
{
public:
  const char * name() const override { return "Derived"; }
  int extra_ = 42;
};

void by_value(Base b)          // <- receives by value
{
  std::cout << "by_value: " << b.name() << "\n";
}

void by_ref(const Base & b)    // <- receives by const reference
{
  std::cout << "by_ref:   " << b.name() << "\n";
}

int main()
{
  Derived d;
  by_value(d);
  by_ref(d);
  return 0;
}
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/WY8csvTqE)

<!-- measure: cmd="g++ -std=c++17 -Wall -Wextra -Wpedantic slicing.cpp -o slicing && ./slicing" -->
```
by_value: Base
by_ref:   Derived
```

**If you receive by value, the `Derived` part is cut off and it becomes a `Base`.**
This is called **object slicing**.
`extra_` also disappears, and the base's virtual function is called.

The compile passes and no warning appears. It silently behaves wrongly.

The remedy is simple. **Handle polymorphic classes by reference or pointer, not by value.**

| How it is received | Does polymorphism work |
| --- | --- |
| `void f(Base b)` | no (slicing) |
| `void f(const Base & b)` | yes |
| `void f(Base * b)` | yes |
| `void f(std::shared_ptr<Base> b)` | yes |

You can also read the fact that `rclcpp` `delete`s the copy of `Node` as
**turning this accident from a "silent malfunction" into a "compile error".**
The `const &` topic of Chapter 4 and the ownership topic of Chapter 5 meet here.

## 3.7 Should you inherit, or should you hold?

Finally, let us talk about design. **Inheritance is not "the only right answer".**

C++ inheritance expresses "is-a" (is a kind of).
`MinimalPublisher` is "a kind of node", so inheritance is natural.

On the other hand, for "has-a" (has a), you should hold it as a member.

```cpp
// inheritance (is-a)
class MinimalPublisher : public rclcpp::Node { };

// hold as a member (has-a / composition)
class MyRobot
{
private:
  rclcpp::Node::SharedPtr node_;
};
```

In fact, **rclcpp also allows the latter.**
The comment of the `RCLCPP_COMPONENTS_REGISTER_NODE` macro says this.

> Valid arguments for NodeClass shall have a constructor that takes a single argument that is a `rclcpp::NodeOptions` instance, and a method of the signature `rclcpp::node_interfaces::NodeBaseInterface::SharedPtr get_node_base_interface`. Note: NodeClass does not need to inherit from `rclcpp::Node`, but it is the easiest way.

It says clearly: **"You do not need to inherit `rclcpp::Node`. But that is the easiest way."**
Anything is fine as long as it has `get_node_base_interface()`.

The reason this works is written in Chapter 2 of [rclcpp design philosophy](../rclcpp_design_philosophy.md).
`Node` is a facade that bundles 11 interfaces, and
free functions such as `create_publisher()` require "only the interfaces they need".

In practice, the judgment is as follows.

| Situation | Choice |
| --- | --- |
| You write an ordinary node | Inherit `public rclcpp::Node` (all exercises in the drill) |
| You want to add a ROS interface to an existing class | Hold a `Node::SharedPtr` as a member |
| You need lifecycle management | Inherit `rclcpp_lifecycle::LifecycleNode` |
| You implement a plugin | Inherit the specified base class (no choice) |

### Avoid multiple inheritance

C++ can have several base classes.

```cpp
class Foo : public Base1, public Base2 { };
```

**Do not use multiple inheritance that includes `rclcpp::Node`.**
If two bases both inherit `Node`, you get 2 copies of `Node` (diamond inheritance), and so on.
You enter an area that needs knowledge of virtual inheritance.

The exception is when you "implement several base classes that have only pure virtual functions (interfaces)".
This corresponds to Java's `implements` and is safe.

## Try it yourself

**Let us see ourselves what happens if you do not write `override`.**

```cpp
// inherit.cpp
#include <iostream>
#include <memory>
#include <vector>

class Sensor
{
public:
  virtual ~Sensor() = default;

  // virtual functions with a default implementation
  virtual double read() const { return 0.0; }
  virtual const char * label() const { return "sensor"; }

  void report() const
  {
    std::cout << label() << " = " << read() << "\n";
  }
};

class Encoder : public Sensor
{
public:
  double read() const override { return 12.5; }
  const char * label() const override { return "encoder"; }
};

class Imu : public Sensor
{
public:
  // trap: forgot const. override is not written either
  double read() { return 99.9; }
  const char * label() const override { return "imu"; }
};

int main()
{
  std::vector<std::unique_ptr<Sensor>> sensors;
  sensors.push_back(std::make_unique<Encoder>());
  sensors.push_back(std::make_unique<Imu>());

  for (const auto & s : sensors) {
    s->report();
  }
  return 0;
}
```

**Predict: what is printed on the `imu` line? `99.9`, or something else?**

```bash
g++ -std=c++17 -Wall -Wextra inherit.cpp -o inherit && ./inherit
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/K3KMoec61)

**No warning appears even with `-Wall -Wextra`.** This is the reason to write `override`.

After you check it, add `override` to `Imu::read()`.
It becomes a compile error. Read the error message, and fix it by adding `const`.

Next, try these 3 things.

**① Remove the virtual destructor**

Change `virtual ~Sensor() = default;` to `~Sensor() = default;`, and
add the following to `Encoder`.

```cpp
  ~Encoder() { std::cout << "~Encoder\n"; }
```

When it is destroyed through `unique_ptr<Sensor>`, is `~Encoder` printed? What happens if you put `virtual` back?

**② Cause slicing**

Change the body of `report()` to the following.

```cpp
  void report() const
  {
    print_value(*this);       // pass it as a Sensor value
  }
```

```cpp
void print_value(Sensor s)         // receives by value
{
  std::cout << s.label() << " = " << s.read() << "\n";
}
```

**Predict: is `encoder` printed, or `sensor`?**
Change `Sensor s` to `const Sensor & s`, and check that the output changes.

**③ Prohibit copying**

Add the following to `Sensor`.

```cpp
  Sensor(const Sensor &) = delete;
  Sensor & operator=(const Sensor &) = delete;
```

The pass-by-value version of ② becomes a compile error.
**This is what `rclcpp::Node` does.**
If you add `Sensor(const Sensor &) = delete;`, the default constructor also disappears,
so you also need `Sensor() = default;`. The reason is the same rule as in Chapter 2: "if you write even one
constructor with arguments, the default is not generated".

## Common pitfalls

**`error: no matching function for call to ‘rclcpp::Node::Node()’`**
You forgot to call `Node("name")` in the initializer list. Section 3.3.

**`error: ‘create_publisher’ was not declared in this scope`**
You forgot to write `: public rclcpp::Node`, or you forgot the `public` of the inheritance.
If you write `class Foo : rclcpp::Node`, it becomes **`private` inheritance**,
and `create_publisher` is treated as `private`, so it cannot be called from outside or from a derived class.
**The default inheritance for `class` is `private`** (for `struct` it is `public`).
There is no place to use `private` inheritance in ROS 2. **Always write `public`.**

**`error: cannot declare variable ‘x’ to be of abstract type**
You forgot to implement a pure virtual function.
`note: because the following virtual functions are pure within ...` lists
the functions you must implement.

**`marked ‘override’, but does not override`**
The signature is different from the base virtual function. Common causes are
the presence of `const`, the argument type (`int` and `int &`), and the presence of `noexcept`.
The sure way is to copy the base declaration and bring it over.

**You wrote a function with the same name as the base in the derived class, and now you cannot call another overload of the base**
This is **name hiding**. If you declare the same name in the derived class,
**all the overloads** of that name in the base are hidden (even with different signatures).

```cpp
class Base
{
public:
  void f(int) {}
  void f(double) {}
};

class Derived : public Base
{
public:
  void f(int) {}      // Base::f(double) is hidden too
};

Derived d;
d.f(1.5);             // Derived::f(int) is called (double -> int is converted)
```

If you write `using Base::f;` in the derived class, the overloads of the base come back.

**You want to inherit the constructors of the derived class**
Since C++11, `using Base::Base;` lets you take over the constructors of the base as they are.
You do not use it with `rclcpp::Node` (because we want to fix the node name).

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 01 to 15 (all exercises) | `class X : public rclcpp::Node`, and `Node("name")` in the initializer list |
| 01, 02 | Call the inherited `create_publisher` / `create_subscription` / `get_logger` |
| 06, 07 | Call the inherited `declare_parameter` / `get_parameter` |
| 13 | Call the inherited `create_callback_group` |
| 14, 15 | `Node("name", options)` — passing the received `NodeOptions` straight through. If you break this, the test fails |
| 15 | `RCLCPP_COMPONENTS_REGISTER_NODE` states clearly that "inheriting `Node` is not required" |
| 11 | **An example that does not use inheritance.** `limit_velocity` is a free function. The reason is ease of testing |

After you finish this course, when you reach the stage of writing a `move_base_flex` planner or controller yourself,
the pure virtual functions of Section 3.4 become the main player.
The structure is: a class that inherits a base class such as `SimplePlanner` / `SimpleController`
is registered in `plugins.xml`, and `move_base_flex` receives it at run time
as a pointer to the base class.

`hardware_interface::SystemInterface` of `ros2_control` is the same.
The hardware class for each servo or motor driver is an implementation that inherits it.
**Inheritance is needed not "for the drill" but "for reading real robot code".**

## Matching exercise

After you read this chapter, practice with the matching drill.

- `cpp03_inheritance` — inherit and implement a virtual function

```bash
./drill run cpp03
```

From the exercise side, you can come back to this chapter with `./drill read`.

## References

- `/opt/ros/jazzy/include/rclcpp/rclcpp/node.hpp` — `class Node : public std::enable_shared_from_this<Node>` (line 79), `virtual ~Node();` (line 109), `protected:` (line 1620), `RCLCPP_DISABLE_COPY(Node)` (line 1634)
- `/opt/ros/jazzy/include/rclcpp/rclcpp/macros.hpp` — the definition of `RCLCPP_DISABLE_COPY` (line 26)
- [rclcpp design philosophy](../rclcpp_design_philosophy.md) Chapter 2 — that `Node` is a bundle of interfaces. The background of "you do not have to inherit"
- `cppreference` [Derived classes](https://en.cppreference.com/w/cpp/language/derived_class) and [virtual function specifier](https://en.cppreference.com/w/cpp/language/virtual)

---

Previous → [2. Classes and initialization](02_classes_and_initialization.md)
Next → [4. References and const](04_references_and_const.md)
