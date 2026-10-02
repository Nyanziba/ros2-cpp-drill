# 11. The standard library toolbox

> **Goal of this chapter**: In the whole drill, **`std::string` appears 33 times and `std::vector` appears 17 times.**
> The "strings" and "containers" in this chapter are used in all of drills 01 to 15.
> Also, std::optional (a value that may or may not exist), std::clamp (limiting a value to a range), and enum class (a type-safe enumeration)
> appear in the core exercises of the drill.
> Without these tools, you can no longer tell whether you are reading ROS 2 code or fighting the C++ API.
> This chapter clears that up.

## 11.1 `std::string` — strings in C++

### Why use `std::string`

ROS 2 messages have string fields.

```cpp
// std_msgs/msg/String.msg
string data
```

In C++, you receive it as the `std::string` type.

```cpp
// solutions/01_publisher/src/minimal_publisher.cpp
auto message = std_msgs::msg::String();
message.data = "Hello, world! " + std::to_string(count_++);
```

Unlike C's `char *`, **`std::string` manages its length and memory automatically.**
The risk of buffer overflows and memory leaks drops a lot.

### `c_str()` — why printf-style formats need it

RCLCPP_INFO is a "logging macro", and it uses printf-like format specifiers.

```cpp
// The actual code of drill 01
RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
```

**`%s` expects a `const char *`.** `std::string` is a "class",
so if it is read as a pointer as it is, you read the address of the object (or other unintended memory).
This is **undefined behavior**. It may crash, print garbage, or, if you are lucky, work.

`.c_str()` returns the contents of the `std::string` as a `const char *`.

```cpp
std::string msg = "Hello";
printf("Message: '%s'\n", msg.c_str());  // OK
printf("Message: '%s'\n", msg);          // undefined behavior
```

This is not a compile error. It is a **runtime trap**.

### `.size()` and `.empty()`

```cpp
std::string msg = "Hello";
if (!msg.empty()) {
  std::cout << "Length: " << msg.size() << "\n";  // Length: 5
}
```

`.size()` returns the number of characters. `.empty()` is the same as `size() == 0`,
but for the reader, "checking the length" and "checking whether there is content" mean different things,
so they are written separately on purpose.

### Joining strings and `std::to_string`

A `std::string` and a `const char *` literal can be joined with `+`.

```cpp
int count = 42;
std::string greeting = "Count: " + std::to_string(count);
// greeting is "Count: 42"
```

**However, the left side of the first `+` must be a `std::string`.**

```cpp
"Hello" + std::string(" world");  // OK
"Hello" + " world";               // error (adding C strings to each other)
```

## 11.2 `std::vector` — a dynamic array

### Adding elements and capacity

```cpp
std::vector<int> v;
v.push_back(1);
v.push_back(2);
// v = [1, 2]
```

`.push_back()` adds an element to the end.
Inside, when the capacity runs out, it allocates new memory and **copies all elements**.

When you add many elements, reserve the capacity first with `.reserve()`.

```cpp
std::vector<int> v;
v.reserve(1000);    // reserve capacity for 1000 elements
for (int i = 0; i < 1000; ++i) {
  v.push_back(i);   // no memory reallocation happens in here
}
```

`.reserve()` only increases the capacity. It does not increase the number of elements.
To actually put elements in, use `.push_back()` or `.emplace_back()`.

### `.at()` vs `[]`

```cpp
std::vector<int> v{10, 20, 30};
std::cout << v.at(0);    // 10 (with range check)
std::cout << v[0];       // 10 (no range check)
```

`.at()` throws an exception (`std::out_of_range`) on out-of-range access.
`[]` does no range check, so it is fast, but when there is a bug it causes
undefined behavior (a crash, garbage output).

In the drill the range is known, so `[]` is usually fine.
But when you receive uncertain input, it is worth using `.at()`.

### Range-based for loop

```cpp
for (auto x : v) { }              // makes a copy (chapter 8)
for (auto & x : v) { }            // a reference you can modify
for (const auto & x : v) { }      // a read-only reference (recommended)
```

**`const auto &` is the default.** Use `auto &` only when you want to modify,
and `auto` only when you really want a copy.

## 11.3 `std::optional<T>` — a value that may not exist

Use it when the return value is "a value on success, no value on failure".

```cpp
// From exercises/04_service_server/test/test_exercise.cpp
std::optional<int64_t> call_add(
  const rclcpp::Node::SharedPtr & server, Probe & probe, int64_t a, int64_t b)
{
  // ... service call ...
  if (!success) {
    return std::nullopt;  // failure
  }
  return future.get()->sum;  // on success, return the value
}
```

The caller checks with `.has_value()`.

```cpp
auto result = call_add(server, probe, 20, 22);
if (result.has_value()) {
  std::cout << "Sum: " << result.value() << "\n";
} else {
  std::cout << "Failed\n";
}
```

Or you can take the value out with `*result` (this assumes `.has_value()` is `true`).

```cpp
std::cout << "Sum: " << *result << "\n";
```

### Comparison with sentinel values

In the days before `std::optional`, people chose a "value that means failure".

```cpp
// the old way
int64_t call_add(int64_t a, int64_t b)
{
  if (failed) {
    return -1;      // -1 means "failure"
  }
  return result;
}
```

But `-1` can also be a valid result of the calculation.
**You cannot tell "did it fail, or is the result -1?"**

`std::optional` makes that difference clear, and enforces it with the type.

```cpp
// the new way
std::optional<int64_t> call_add(int64_t a, int64_t b)
{
  if (failed) {
    return std::nullopt;  // no value
  }
  return result;          // there is a value
}
```

It is not a compile error if the caller does not check `.has_value()`,
but **whether the code checks for a value or uses it without checking is clear from the caller's side too.**

You can also give a default value with `.value_or()`.

```cpp
int64_t sum = call_add(server, probe, 10, 20).value_or(-1);
// sum on success, -1 on failure
```

## 11.4 `std::map` — key and value pairs

You can look up a value from a key quickly.

```cpp
std::map<std::string, int> speeds;
speeds["car"] = 100;
speeds["bike"] = 50;
std::cout << speeds["car"] << "\n";  // 100
```

### The trap of `operator[]` — a missing key is inserted

When the key is not in the map with `map[key]`, **a new element is created automatically with a default value.**

```cpp
std::map<std::string, int> config;
int value = config["unknown"];  // the key does not exist
// → the default value 0 is inserted
// → the size of config becomes 1
```

If you "just want to check", use `.find()` or `.count()`.

```cpp
auto it = config.find("unknown");
if (it != config.end()) {
  std::cout << it->second << "\n";
}

if (config.count("unknown")) {
  std::cout << "exists\n";
}
```

### `.at()` — reading with an existence check

If you access a key that does not exist with `.at()`, it throws an exception.

```cpp
try {
  int value = config.at("unknown");  // exception if the key does not exist
} catch (const std::out_of_range & e) {
  std::cout << "key not found\n";
}
```

## 11.5 `<algorithm>` — standard operations

### `std::max` / `std::min` / `std::clamp`

```cpp
// From exercises/11_node_test/src/velocity_limiter.cpp
const double safe_max_speed = std::max(0.0, max_speed);
const double clamped_delta = std::clamp(delta, -safe_max_delta, safe_max_delta);
```

`std::max(a, b)` is the larger of two values, and `std::min(a, b)` is the smaller one.

`std::clamp(value, min, max)` keeps `value` inside the range `[min, max]`.
It returns `min` if `value < min`, `max` if `value > max`, and `value` otherwise.

**`std::clamp` was added in C++17.** The drill uses `-std=c++17`, so this is fine.

### `std::sort` and `std::find`

```cpp
std::vector<int> nums{3, 1, 4, 1, 5};
std::sort(nums.begin(), nums.end());
// nums = [1, 1, 3, 4, 5]

auto it = std::find(nums.begin(), nums.end(), 4);
if (it != nums.end()) {
  std::cout << "Found at position " << std::distance(nums.begin(), it) << "\n";
}
```

`.begin()` and `.end()` specify the range.
If the returned iterator is `.end()`, it means "not found".

## 11.6 `enum class` — a type-safe enumeration

### The problem with a plain `enum`

```cpp
enum Color {
  RED = 0,
  GREEN = 1,
  BLUE = 2
};

int x = RED;  // no problem
```

The problem is that the values of an `enum` are converted to `int` automatically.

```cpp
enum Status {
  IDLE = 0,
  RUNNING = 1
};

enum Priority {
  LOW = 0,
  HIGH = 1
};

Status s = RUNNING;
Priority p = IDLE;

if (s == p) { }  // bug! it compares values of completely different types
```

Also, the names are scattered into the global scope, so name collisions happen easily.

### The fix with `enum class`

```cpp
enum class Status {
  IDLE,
  RUNNING,
  STOPPED
};

Status s = Status::RUNNING;    // the name is scoped
// int x = s;                  // error: different types
int x = static_cast<int>(s);   // OK if explicit
```

**`enum class` is:**
1. **Type-safe** — different enumeration types cannot be compared with each other
2. **Scoped** — it does not pollute the global namespace
3. **No implicit conversion** — it prevents conversion to and from `int`

In the drill, ROS 2 configuration objects (for example `rclcpp::CallbackGroupType`) are defined as `enum class`.
(See: `exercises/12_qos/test/test_exercise.cpp`)

## Try it yourself

Use the following code to check the pattern of drill 01 and each feature.

```cpp
// stdlib_toolbox.cpp
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <algorithm>

struct Message {
  std::string data;
  int count;

  explicit Message(std::string d, int c) : data(d), count(c) {}
  Message(const Message & o) : data(o.data), count(o.count)
  {
    std::cout << "  [Message copy]\n";
  }
};

std::optional<int> divide(int a, int b)
{
  if (b == 0) {
    return std::nullopt;
  }
  return a / b;
}

int main()
{
  std::cout << "=== std::string ===\n";
  std::string greeting = "Hello";
  greeting = greeting + " world";
  std::cout << "Greeting: '" << greeting << "'\n";
  std::cout << "Size: " << greeting.size() << "\n";
  std::cout << "Empty: " << (greeting.empty() ? "yes" : "no") << "\n";
  printf("Via printf: '%s'\n", greeting.c_str());

  std::cout << "\n=== std::to_string ===\n";
  int count = 42;
  std::string message = "Count: " + std::to_string(count);
  std::cout << message << "\n";

  std::cout << "\n=== std::vector ===\n";
  std::vector<Message> messages;
  messages.reserve(2);
  std::cout << "push_back 1st\n";
  messages.push_back(Message("hello", 0));
  std::cout << "push_back 2nd\n";
  messages.push_back(Message("world", 1));
  std::cout << "Iterating with const auto &\n";
  for (const auto & msg : messages) {
    std::cout << "  msg.data = " << msg.data << ", count = " << msg.count << "\n";
  }

  std::cout << "\n=== std::optional ===\n";
  auto result1 = divide(10, 2);
  std::cout << "divide(10, 2): ";
  if (result1.has_value()) {
    std::cout << *result1 << "\n";
  } else {
    std::cout << "failed\n";
  }
  auto result2 = divide(10, 0);
  std::cout << "divide(10, 0): " << result2.value_or(-1) << "\n";

  std::cout << "\n=== std::map ===\n";
  std::map<std::string, int> speeds;
  speeds["car"] = 100;
  speeds["bike"] = 50;
  std::cout << "car: " << speeds.at("car") << "\n";
  if (speeds.count("plane")) {
    std::cout << "plane: " << speeds["plane"] << "\n";
  } else {
    std::cout << "plane: not found\n";
  }

  std::cout << "\n=== std::algorithm ===\n";
  double value = 15.0;
  double limited = std::clamp(value, 0.0, 10.0);
  std::cout << "clamp(15, 0, 10) = " << limited << "\n";

  std::vector<int> nums{3, 1, 4, 1, 5};
  std::sort(nums.begin(), nums.end());
  std::cout << "Sorted: ";
  for (int n : nums) std::cout << n << " ";
  std::cout << "\n";

  std::cout << "\n=== enum class ===\n";
  enum class Status { IDLE, RUNNING };
  Status s = Status::RUNNING;
  std::cout << "Status: " << static_cast<int>(s) << "\n";

  return 0;
}
```

**Predict: What is the output order? In particular, how many times is "Message copy" printed, and when?**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic stdlib_toolbox.cpp -o stdlib_toolbox && ./stdlib_toolbox
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/3b41ez87G)

Measured result:

<details markdown="1"><summary>Answer (actual output)</summary>

```
=== std::string ===
Greeting: 'Hello world'
Size: 11
Empty: no
Via printf: 'Hello world'

=== std::to_string ===
Count: 42

=== std::vector ===
push_back 1st
  [Message copy]
push_back 2nd
  [Message copy]
Iterating with const auto &
  msg.data = hello, count = 0
  msg.data = world, count = 1

=== std::optional ===
divide(10, 2): 5
divide(10, 0): -1

=== std::map ===
car: 100
plane: not found

=== std::algorithm ===
clamp(15, 0, 10) = 10
Sorted: 1 1 3 4 5 

=== enum class ===
Status: 1

```

</details>

**Key points to check:**
1. Because capacity was reserved with `reserve()`, one `[Message copy]` is printed for each `push_back`
2. `messages` is looped over with `const auto &`, so no extra copy is printed
3. `divide(10, 0)` is `nullopt`, so `value_or(-1)` returns `-1`
4. `speeds["plane"]` does not exist, so it is checked with `count()`
5. `std::clamp(15, 0, 10)` limits 15 to 10

## Common pitfalls

**You passed a `std::string` directly to printf**

```cpp
std::string msg = "hello";
printf("Message: %s\n", msg);  // undefined behavior
```

When you run it, you get unpredictable output or a crash. In the environment where I measured it, garbage bytes were printed after `Message: ` (this changes from run to run). The compiler warning is as follows.

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// printf_string.cpp
#include <cstdio>
#include <string>

int main()
{
  std::string msg = "hello";
  printf("Message: %s\n", msg);  // undefined behavior
  return 0;
}
```

```bash
g++ -std=c++17 -Wall printf_string.cpp -o printf_string && ./printf_string
```

</details>

```
printf_string.cpp: In function ‘int main()’:
printf_string.cpp:8:21: warning: format ‘%s’ expects argument of type ‘char*’, but argument 2 has type ‘std::string’ {aka ‘std::__cxx11::basic_string<char>’} [-Wformat=]
    8 |   printf("Message: %s\n", msg);  // undefined behavior
      |                    ~^
      |                     |
      |                     char*
```

Add `.c_str()`.

```cpp
printf("Message: %s\n", msg.c_str());
```

**`"hello" + std::string(" world")` is a compile error**

```cpp
std::string s = "hello" + " world";  // error
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// concat_literals.cpp
#include <string>

int main()
{
  std::string s = "hello" + " world";  // error
  return 0;
}
```

```bash
g++ -std=c++17 concat_literals.cpp -o concat_literals
```

</details>

```
concat_literals.cpp: In function ‘int main()’:
concat_literals.cpp:6:27: error: invalid operands of types ‘const char [6]’ and ‘const char [7]’ to binary ‘operator+’
    6 |   std::string s = "hello" + " world";  // error
      |                   ~~~~~~~ ^ ~~~~~~~~
      |                   |         |
      |                   |         const char [7]
      |                   const char [6]
```

Make the left side a `std::string`.

```cpp
std::string s = std::string("hello") + " world";
```

**`vector[100]` did not crash**

Out-of-range access is undefined behavior, so it does not always crash.
It may happen to read valid memory, or it may be fine depending on the address space.
Because this makes debugging hard, use `.at()` when you need a reliable range check,
or check with `.size()` beforehand.

**`std::map[key]` created an element you did not intend**

```cpp
int v = config["unknown"];  // if the key does not exist, it is created with the default value 0
if (config.size() > 1) { }  // the size has increased
```

For read-only access, use `.find()` or `.count()`.

**`std::optional<int> x; int y = x;` is an error**

```cpp
std::optional<int> x = 5;
int y = x;  // error
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// optional_to_int.cpp
#include <optional>

int main()
{
  std::optional<int> x = 5;
  int y = x;  // error
  return 0;
}
```

```bash
g++ -std=c++17 optional_to_int.cpp -o optional_to_int
```

</details>

```
optional_to_int.cpp: In function ‘int main()’:
optional_to_int.cpp:7:11: error: cannot convert ‘std::optional<int>’ to ‘int’ in initialization
    7 |   int y = x;  // error
      |           ^
      |           |
      |           std::optional<int>
```

Check with `.has_value()`, take the value out with `*x`, or give a default value with `.value_or()`.

**You mixed `enum` and `enum class`**

```cpp
enum Color { RED };
enum class Status { RUNNING };

if (RED == Status::RUNNING) { }  // error
```

<details markdown="1"><summary>Full program that produced this output</summary>

```cpp
// enum_mix.cpp
enum Color { RED };
enum class Status { RUNNING };

int main()
{
  if (RED == Status::RUNNING) { }  // error
  return 0;
}
```

```bash
g++ -std=c++17 enum_mix.cpp -o enum_mix
```

</details>

```
enum_mix.cpp: In function ‘int main()’:
enum_mix.cpp:7:11: error: no match for ‘operator==’ (operand types are ‘Color’ and ‘Status’)
    7 |   if (RED == Status::RUNNING) { }  // error
      |       ~~~ ^~ ~~~~~~~~~~~~~~~
      |       |              |
      |       Color          Status
enum_mix.cpp:7:11: note: candidate: ‘operator==(Status, Status)’ (built-in)
    7 |   if (RED == Status::RUNNING) { }  // error
      |       ~~~~^~~~~~~~~~~~~~~~~~
enum_mix.cpp:7:11: note:   no known conversion for argument 1 from ‘Color’ to ‘Status’
enum_mix.cpp:7:11: note: candidate: ‘operator==(Color, Color)’ (built-in)
enum_mix.cpp:7:11: note:   no known conversion for argument 2 from ‘Status’ to ‘Color’
```

Use one kind of enumeration. In new code, use `enum class`.

## Where it appears in the drill

| Exercise | What from this chapter appears |
| --- | --- |
| 01 | `auto message = std_msgs::msg::String(); message.data = "Hello, world! " + std::to_string(count_++);` — joining std::string and std::to_string |
| 01 | `RCLCPP_INFO(..., "%s", message.data.c_str());` — c_str() is required |
| 02 | `const std_msgs::msg::String & msg` — receiving with const auto & (chapter 8) |
| 04, 05, 13 | `auto future = client_->async_send_request(request);` → check whether a value arrived with `future.wait_for(2s)` (chapter 12) |
| 11 | `std::max(0.0, max_speed)` and `std::clamp(delta, -safe_max_delta, safe_max_delta)` |
| 11 | `#include <algorithm>` — for std::clamp |
| 12 | QoS profile settings (key-value pairs like std::map) |
| 14 | Holding with `ConstSharedPtr msg` (chapter 6) |
| Tests | `std::optional<int64_t> call_add(...)` expresses whether there is a return value (04_service_server/test/test_exercise.cpp) |
| Tests | Collecting elements with `std::vector<T>`, `const auto &` in range-based for |

Exercise 11 (velocity_limiter) is a real example of `std::max` and `std::clamp`.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `cpp11_stl` — use the standard library

```bash
./drill run cpp11
```

From the exercise, you can come back to this chapter with `./drill read`.

## References

- [cppreference: std::string](https://en.cppreference.com/w/cpp/string/basic_string)
- [cppreference: std::vector](https://en.cppreference.com/w/cpp/container/vector)
- [cppreference: std::optional](https://en.cppreference.com/w/cpp/utility/optional)
- [cppreference: std::map](https://en.cppreference.com/w/cpp/container/map)
- [cppreference: <algorithm>](https://en.cppreference.com/w/cpp/algorithm)
- [ROS 2 coding conventions](../ros2_coding_conventions.md)

---

Previous → [10. Operator overloading](10_operator_overloading.md)
Next → [12. `std::chrono` and time](12_chrono_and_time.md)
