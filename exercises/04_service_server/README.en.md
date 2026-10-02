# Exercise 04: Write a service server [Beginner]

Write the server side of the official tutorial
[Writing a simple service and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Service-And-Client.html)
exactly as it is.

## What to do

Fill in the TODOs in `src/add_two_ints_server.cpp`. The specification is the same as the official tutorial.

| Item | Value |
| --- | --- |
| Node name | `add_two_ints_server` |
| Service name | `add_two_ints` |
| Type | `example_interfaces::srv::AddTwoInts` |
| Response | `response->sum = request->a + request->b;` |
| Log 1 | `Incoming request\na: %ld b: %ld` (`request->a`, `request->b`) |
| Log 2 | `sending back response: [%ld]` (`response->sum`) |

You can check the contents of `AddTwoInts` like this.

```
$ ros2 interface show example_interfaces/srv/AddTwoInts
int64 a
int64 b
---
int64 sum
```

The part above `---` is the request (`AddTwoInts::Request`), and the part below is the response (`AddTwoInts::Response`).

The class declaration (`include/drill/add_two_ints_server.hpp`) is provided. Think about two things: inside `add()`,
you write to `response` (you do not return it with `return`), and in the constructor
you store the return value of `create_service` in `service_`.

### Differences from the official tutorial

The official tutorial calls `rclcpp::Node::make_shared("add_two_ints_server")` directly inside `main()`,
and writes `add` as a free function that does not belong to a class. In this exercise, so that the tests can create the
node directly and check it, the code is grouped into a class named `add_two_ints_server` that inherits from `rclcpp::Node` (the same form as the official
`examples_rclcpp_minimal_service` package). This is the only intentional deviation from the official tutorial, including the fact that `main()` is split into a separate file
(`src/server_main.cpp`).
The logic (the service name, the wording of the logs, and the contents of the response) is exactly the same as the official one.

## Run it

After the tests pass, you can run it by hand, just like the official tutorial.

```bash
source install/setup.bash
ros2 run drill_04_service_server server
```

In another terminal:

```bash
ros2 service list                     # can you see /add_two_ints?
ros2 interface show example_interfaces/srv/AddTwoInts
ros2 service call /add_two_ints example_interfaces/srv/AddTwoInts "{a: 20, b: 22}"
```

If the `Incoming request` log appears in the server terminal, and a response such as
`response:\nexample_interfaces.srv.AddTwoInts_Response(sum=42)` appears in the caller terminal,
it works.

## Common pitfalls

- Always assign the return value of `create_service()` to `service_`. If you receive it in a local variable,
  it is destroyed when the constructor returns, and the service disappears (the same trap as the Publisher/Timer in 01).
- `add` is a handler. Instead of `return`ing a value, you write to the members of the
  `response` (a `shared_ptr`) passed as an argument.
- `_1` and `_2` in `std::bind(&AddTwoIntsServer::add, this, _1, _2)` are
  `std::placeholders::_1` / `_2`. If you forget `using std::placeholders::_1;`,
  you get a compile error.
- `request->a` and `request->b` are `int64_t`. Use `%ld` in the log format
  (the same as the official tutorial).

## Tests

```bash
./drill run 04
```

| Test | What it checks |
| --- | --- |
| `ExposesAddTwoIntsService` | Whether `create_service` is stored in `service_` |
| `ReturnsSumOfTwoIntegers` | `response->sum = request->a + request->b;` |
| `HandlesZeroAndNegativeNumbers` | Calculation at boundary values (0, negative numbers) |
| `RespondsToConsecutiveCalls` | Whether it can handle multiple requests |
| `LogsSameIncomingRequestAsOfficial` | The format of `RCLCPP_INFO` |

## References

- Official: [Writing a simple service and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Service-And-Client.html)
- Local example implementation: `/opt/ros/jazzy/share/examples_rclcpp_minimal_service/`
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
