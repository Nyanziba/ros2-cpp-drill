# 15. Checklist before you start the drill

> **Goal of this chapter**: Check the C++ features you learned up to chapter 14, in the form of implementation.
> You check whether the key point of each chapter has sunk in, by reading code and answering.

## 15.1 Read the code and answer (10 questions)

### Question 1: Smart pointers and ownership

```cpp
class MinimalPublisher : public rclcpp::Node {
private:
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
};

MinimalPublisher::MinimalPublisher() : Node("minimal_publisher") {
  publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);
  // The lower part is an error. Why?
}
```

What happens if you make `publisher_` above a local variable inside the constructor instead of a member variable?

<details markdown="1">
<summary>Answer</summary>

The return value of `create_publisher()` is a `shared_ptr`. If you make it a local variable, the count becomes 0 the moment you leave the constructor, and the Publisher is destroyed. The rclcpp side only holds the Publisher with a `weak_ptr`, so `lock()` returns `nullptr` every time, and the Executor runs nothing.

**No error and no warning appears.** Nothing is simply published.

For details, see section 6.5 of [6. Smart pointers](06_smart_pointers.md).

</details>

### Question 2: Inheritance and public

```cpp
class MinimalPublisher : public rclcpp::Node { ... };
```

What does this `public` mean? If you remove `public` and write `class MinimalPublisher : rclcpp::Node { ... };`, what breaks?

<details markdown="1">
<summary>Answer</summary>

`public` means "keep the `public` members of the base class `public` in the derived class too".

If you remove `public`, it becomes **`private` inheritance**, and the `public` members of the base class become `private` in the derived class. Then you can no longer call `create_publisher` or `get_logger` on a `MinimalPublisher` object from outside. All the implementations in the drill break.

For details, see sections 3.1 and 3.2 of [3. Inheritance](03_inheritance.md).

</details>

### Question 3: Reading references and const

```cpp
const std::shared_ptr<Request> request
```

Read this declaration in words. Where is `const` attached? Is this a reference?

<details markdown="1">
<summary>Answer</summary>

Read it from right to left.

- `request` is
- a `shared_ptr<Request>`, and
- that `shared_ptr` itself is **const**

In other words, "request is the `shared_ptr<Request>` itself", and it is not a reference. A reference has `&`.

If it were a reference, it would have `&`, such as `const std::shared_ptr<Request> & request` or `std::shared_ptr<const Request> & request`.

For details, see [C++ Basics chapter 1, Reading declarations](../cpp-basics/01_reading_declarations.md).

</details>

### Question 4: auto, references, and const

```cpp
const std::vector<std::string> v{"hello", "world"};
const std::string & ref = v[0];

auto a = ref;
auto & b = ref;
const auto & c = ref;
```

What are the types of `a`, `b`, and `c`? Does `a = "changed"` change `v[0]`?

<details markdown="1">
<summary>Answer</summary>

- `a` is `std::string` (the reference and const are dropped) → **a copy**
- `b` is `const std::string &` (the reference is kept)
- `c` is `const std::string &` (an explicit const reference)

`a = "changed"` changes **only** `a`. `v[0]` does not change.

**`auto` deduces a value, and drops the reference and const.** If you want a reference, you add `&` yourself.

For details, see section 8.2 of [8. auto and type deduction](08_auto_and_type_deduction.md).

</details>

### Question 5: Move and ownership

```cpp
auto msg = std::make_unique<std_msgs::msg::String>();
msg->data = "hello";
publisher_->publish(std::move(msg));

std::cout << msg.get();  // What is msg now?
```

After `publish()`, what has `msg` become?

<details markdown="1">
<summary>Answer</summary>

`msg` is `nullptr`.

`std::move(msg)` makes `msg` be treated as an rvalue, and `publish()` calls the move constructor inside. After a move, **the standard guarantees that a `unique_ptr` always becomes `nullptr`.**

`std::move` "moves nothing". What it really is, is a cast that means "treat as an rvalue".

For details, see section 5.2 of [5. Move and ownership](05_move_and_ownership.md).

</details>

### Question 6: The 3 arguments of std::bind

```cpp
std::bind(&MinimalSubscriber::topic_callback, this, _1)
```

What do `&MinimalSubscriber::topic_callback`, `this`, and `_1` each point to?

<details markdown="1">
<summary>Answer</summary>

1. `&MinimalSubscriber::topic_callback` — a pointer to a member function. The "&" is required.
2. `this` — the object to call the function on (the `MinimalSubscriber` instance).
3. `_1` — a placeholder. It means "put the 1st argument given at call time here".

In other words, it means "to the `topic_callback` member function of MinimalSubscriber, give the current object (`this`) and the argument given at call time (`_1`)".

For details, see section 7.2 of [7. Lambdas and std::bind](07_lambdas_and_std_bind.md).

</details>

### Question 7: Virtual destructor

```cpp
class Base {
public:
  virtual ~Base() {}
};

class Derived : public Base { ... };
```

What happens if there is no `virtual ~Base()`?

<details markdown="1">
<summary>Answer</summary>

Without `virtual`, when you delete through a pointer to the base class, **the destructor of the derived class is not called**. This is a resource leak.

```cpp
Base * p = new Derived();
delete p;  // Derived::~Derived() is not called (when there is no virtual)
```

**If a base class has a destructor, the iron rule is to make it `virtual`.**

In the drill, `rclcpp::Node` has a virtual destructor, so `MinimalPublisher` is also safe automatically.

For details, see section 3.5 of [3. Inheritance](03_inheritance.md).

</details>

### Question 8: Compile errors and link errors

```
Which of `undefined reference to 'f()'` and `multiple definition of 'f()'`
is a link error? What does each one mean?
```

<details markdown="1">
<summary>Answer</summary>

**Both are link errors.** A compiler message comes with a line number and an excerpt, but a linker message does not.

- `undefined reference to 'f()'` — **the definition is not found**. There is no implementation in the `.cpp`, or it is not linked in CMakeLists.txt.
- `multiple definition of 'f()'` — **there are definitions in several translation units**. You wrote the body in a `.hpp` and forgot `inline`.

For details, see section 1.5 of [1. How build and link work](01_how_build_and_link_work.md).

</details>

### Question 9: User-defined literals

```cpp
using namespace std::chrono_literals;

auto delay = 500ms;  // This is valid C++. Why?
```

Why can you write `500ms` just like that?

<details markdown="1">
<summary>Answer</summary>

`500ms` is a **user-defined literal**. The operator `operator""ms` is defined in the namespace `std::chrono_literals`, and `using namespace` brings it into the global scope, so you can write it.

```cpp
// Inside the C++ standard library
namespace std::chrono_literals {
  constexpr std::chrono::milliseconds operator""ms(unsigned long long count) {
    return std::chrono::milliseconds(count);
  }
}
```

`500ms` is converted automatically to `std::chrono::milliseconds(500)`.

For details, see section 12.1 of [12. chrono and time](12_chrono_and_time.md).

</details>

### Question 10: std::sort and operator<

```cpp
struct Point { int x; int y; };

std::vector<Point> v{{3, 4}, {1, 2}};
std::sort(v.begin(), v.end());  // Error. Why?
```

What is missing, so that this code does not compile?

<details markdown="1">
<summary>Answer</summary>

Because **`Point` has no `operator<` defined.**

`std::sort` calls `a < b` inside to compare elements. For a type you define yourself, you need to provide this operator.

```cpp
bool operator<(const Point & a, const Point & b) {
  return a.x < b.x;  // or some other comparison logic
}
```

If you add this, it compiles.

For details, see section 10.3 of [10. Operator overloading](10_operator_overloading.md).

</details>

## 15.2 Checklist by chapter

| Chapter | Can you explain | Where to check |
| --- | --- | --- |
| 1 | Do you understand the difference between `undefined reference` and `multiple definition`? | Section 1.5 of [1. How build and link work](01_how_build_and_link_work.md) |
| 2 | Why do we use the initializer list (`: member(value)`)? | Section 2.2 of [2. Classes and initialization](02_classes_and_initialization.md) |
| 3 | What is the `public` in `class X : public Base`? Why is `virtual ~Base()` needed? | Sections 3.1 and 3.5 of [3. Inheritance](03_inheritance.md) |
| 4 | The difference in position and meaning between `const` and `&` in `const T &` | [C++ Basics chapter 1](../cpp-basics/01_reading_declarations.md), [C++ Basics chapter 6](../cpp-basics/06_const.md) |
| 5 | What `std::move` really is, and the state of an object after a move | Section 5.2 of [5. Move and ownership](05_move_and_ownership.md) |
| 6 | Why it does not work if you do not keep the return value of `create_publisher()` as a member | Section 6.5 of [6. Smart pointers](06_smart_pointers.md) |
| 7 | What are the 3 parts of `std::bind(&Class::func, this, _1)`? | Section 7.2 of [7. Lambdas and std::bind](07_lambdas_and_std_bind.md) |
| 8 | Does `auto` drop references and `const`? The difference between `for (auto & x : items)` and `for (auto x : items)` | Section 8.2 of [8. auto and type deduction](08_auto_and_type_deduction.md) |
| 9 | What does the content of `<>` in `create_publisher<std_msgs::msg::String>` decide? | Section 9.1 of [9. Reading templates](09_reading_templates.md) |
| 10 | Do you write the `operator<` used by `std::sort` as a member or as a free function? The difference between `operator+` and `operator*` | Sections 10.2 and 10.3 of [10. Operator overloading](10_operator_overloading.md) |
| 11 | The difference between `std::string::c_str()` and `.data()`. How to scan to the end of a `std::vector` | [11. The standard library toolbox](11_standard_library_toolbox.md) |
| 12 | Why is `500ms` valid? What is `using namespace std::chrono_literals`? | Section 12.1 of [12. chrono and time](12_chrono_and_time.md) |
| 13 | Why can `future.wait_for(2s)` deadlock? | [13. Minimal concurrency](13_minimal_concurrency.md) |
| 14 | Why is there almost no `try`/`catch` in ROS 2 code? | [14. Error handling](14_error_handling.md) |

## 15.3 Where to go back when you are stuck

| Symptom | Related chapter |
| --- | --- |
| It compiles, but the link fails | 1. How build and link work (section 1.5) |
| A member variable stays `nullptr` | 6. Smart pointers (section 6.5) |
| `const &` appears in a function argument type, and you cannot read it | C++ Basics chapter 1 (Reading declarations) |
| You used `auto` and a copy happened | 8. auto and type deduction (section 8.2) |
| `std::sort` gives an error | 10. Operator overloading (section 10.3) |
| You are publishing, but nothing comes out | 6. Smart pointers (section 6.5) and 7. Lambdas and std::bind (section 7.2) |

## 15.4 Start the drill

When the C++ lecture is finished, start the drill.

```bash
./drill list
./drill watch
```

`drill list` shows all exercises. The C++ track (`cpp01` to `cpp12`) has **10 exercises on operators, templates, and tests**.

The ROS 2 track (`01` to `15`) has **15 exercises on nodes, communication, parameters, and actions**, and in each exercise you use the C++ features you learned in an implementation.

**When you run `./drill watch`, it watches the exercise files for changes and runs the tests automatically.** Go in order from the top.

---

Previous → [14. Error handling](14_error_handling.md)
