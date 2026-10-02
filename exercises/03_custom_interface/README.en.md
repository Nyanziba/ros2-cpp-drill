# Exercise 03: Define and use a custom interface [Beginner]

Define the `tutorial_interfaces` package (`msg/Num.msg` and `srv/AddThreeInts.srv`) from the official tutorial
[Creating custom msg and srv files](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Custom-ROS2-Interfaces.html)
exactly as it is, and use it from C++ right where you define it, following the steps in
[Single package: define and use an interface](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Single-Package-Define-And-Use-Interface.html).

In the earlier exercises you used **ready-made types** such as `std_msgs::msg::String` and `example_interfaces::srv::AddTwoInts`.
This time you create the types themselves.

## What to do

Fill in three places.

1. `msg/Num.msg` — Define one field `num` of type `int64`.
2. `srv/AddThreeInts.srv` — Define `int64 a`, `int64 b`, `int64 c` on the request side
   and `int64 sum` on the response side (separated by `---`).
3. `src/num_publisher.cpp` — Implement `NumNode`, which actually uses the two types you defined.

| Item | Value |
| --- | --- |
| Node name | `num_node` |
| Topic name | `num` |
| Topic type | `drill_03_custom_interface::msg::Num` |
| Publish period | 500ms |
| Service name | `add_three_ints` |
| Service type | `drill_03_custom_interface::srv::AddThreeInts` |
| Service response | `response->sum = request->a + request->b + request->c;` |

`NumNode` has two jobs: "publish `Num`" and "provide the `AddThreeInts` service".
The official tutorial shows these two in separate chapters and separate examples
(publishing `Num` is the answer to a practice problem, and the service that uses `AddThreeInts` is in another chapter).
To keep the policy of one package and one concept per exercise, this exercise puts them
together in one node. The class declaration (`include/drill/num_node.hpp`) is provided, so
think about the body of the `NumNode` constructor and the two member functions.

## How to write `.msg` / `.srv`

A `.msg` file is just one "type name" pair per line.

```
int64 num
```

In a `.srv` file, the part above `---` is the request and the part below is the response.

```
int64 a
int64 b
int64 c
---
int64 sum
```

## Why put the interface definition and its user in the same package

As question Q3 of the oral quiz in the lecture notes [13_Custom interfaces](../../docs-en/ros2/13_custom_interfaces.md) says,
in real work it is common to split a **package for interface definitions**
(like `tutorial_interfaces`) from the **package of the nodes that use it**
(so that another package that only wants the types does not pick up unneeded dependencies).

This exercise deliberately does not split them. It uses the layout of the official tutorial
[Single package: define and use an interface](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Single-Package-Define-And-Use-Interface.html)
(everything in one package). There are two reasons.

- We want each exercise in the drill to fit in one package (to match the layout of the other exercises).
- We want you to experience "the CMake you write only when you use a type generated in the same package"
  (`rosidl_get_typesupport_target`).

If you later want to reuse the same types from several node packages, moving them to a
package only for interfaces, for the reason in Q3 of the oral quiz, is the real-world choice. This does not
contradict the lecture notes, but please understand that **this exercise itself uses the other layout that the official docs show**.

### Key points of CMakeLists.txt

```cmake
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Num.msg"
  "srv/AddThreeInts.srv"
)
ament_export_dependencies(rosidl_default_runtime)

# This is needed only when you use a type generated in the same package.
# For a type from another package, find_package(<pkg> REQUIRED) is enough.
rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp)

target_link_libraries(drill_03_custom_interface_node "${cpp_typesupport_target}")
```

The file paths you write in `rosidl_generate_interfaces` are exactly what gets built.
When you add a new `.msg` / `.srv`, always add it to this list too
(**if you forget, the file exists but is not built, and you get a "type not found" error**.
This is a common trap, also covered in the lecture notes).

## When the build passes and when it fails

This exercise is a little special. Even when `msg/Num.msg` and `srv/AddThreeInts.srv` contain
**only comments**, `rosidl_generate_interfaces` itself builds fine
(a `.msg` / `.srv` with no fields is legal as an "empty message".
It is treated like `std_srvs/srv/Empty`).

On the other hand, the grading test of this exercise (`test/test_exercise.cpp`, do not edit) contains code that
actually assigns values to `Num::num` and `AddThreeInts::Request::a`.
So before you write the `num` field in `msg/Num.msg`, **the build of the test itself
fails with a compile error**.

```
error: no member named 'num' in 'drill_03_custom_interface::msg::Num_<std::allocator<void> >'
```

This is intentional. It lets you experience the failure mode specific to custom interfaces:
"if you get a field name or type wrong, it does not compile".
The error message shows `num` (or the field name), so first suspect
`msg/Num.msg` / `srv/AddThreeInts.srv`.

## Run it

After the tests pass, you can run it by hand, just like the official tutorial.

```bash
source install/setup.bash
ros2 run drill_03_custom_interface num_node
```

In another terminal:

```bash
ros2 interface show drill_03_custom_interface/msg/Num
ros2 interface show drill_03_custom_interface/srv/AddThreeInts
ros2 topic echo /num
ros2 service call /add_three_ints drill_03_custom_interface/srv/AddThreeInts "{a: 1, b: 2, c: 3}"
```

## Common pitfalls

- Always assign the return values of `create_publisher()` / `create_wall_timer()` / `create_service()`
  to member variables. If you receive them in local variables, they are destroyed when the constructor
  returns, and nothing happens (the same trap as in 01 and 04).
- If you forget to add a new `.msg` / `.srv` to the list of file paths in
  `rosidl_generate_interfaces`, it is not built (it is already written in this exercise.
  Remember it for when you practice adding a new type yourself).
- The names of the generated headers change from CamelCase to snake_case.
  `Num.msg` → `<package name>/msg/num.hpp`, `AddThreeInts.srv` →
  `<package name>/srv/add_three_ints.hpp` (this is already written in `include/drill/num_node.hpp`,
  so if you just copy it, you do not need to worry).
- `add_three_ints` is a handler. Instead of `return`ing a value, you write to the members of the
  `response` (a `shared_ptr`) passed as an argument (the same as exercise 04).

## Tests

```bash
./drill run 03
```

| Test | What it checks |
| --- | --- |
| `NumMessageHasSingleInt64NumField` | The field name and type in `msg/Num.msg` (the compile itself verifies this) |
| `PublishesNumOnNumTopic` | Whether the publisher and the timer are running, and `count_++` |
| `ExposesAddThreeIntsService` | The definition in `srv/AddThreeInts.srv`, and whether `create_service` is stored in `service_` |
| `ReturnsSumOfThreeIntegers` | `response->sum = request->a + request->b + request->c;`, and the calculation with negative numbers |
| `LogsIncomingRequestInOfficialFormat` | The format of `RCLCPP_INFO` (the three-argument version from exercise 04) |

## References

- Official: [Creating custom msg and srv files](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Custom-ROS2-Interfaces.html)
- Official: [Single package: define and use an interface](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Single-Package-Define-And-Use-Interface.html)
- Lecture notes: [13_Custom interfaces](../../docs-en/ros2/13_custom_interfaces.md)
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
