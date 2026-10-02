# Exercise 05: Write a service client [Beginner]

Write the client side (`add_two_ints_client`) of the official tutorial
[Writing a simple service and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Service-And-Client.html).

## Deviation from the official tutorial

The official tutorial writes all the client work (waiting for the service, sending the request, waiting for the result)
directly inside `main()`. In this exercise, so that the tests can use the class directly,
the code is grouped into a class `AddTwoIntsClient : public rclcpp::Node` and split into the following two methods.

- `bool wait_for_server(std::chrono::nanoseconds timeout)`
  — the body of the official `while (!client->wait_for_service(1s)) { ... }` (one iteration)
- `rclcpp::Client<AddTwoInts>::FutureAndRequestId send_request(int64_t a, int64_t b)`
  — the part of the official tutorial that "builds a request and calls `async_send_request`"

Whether to loop is the responsibility of `main()` (`src/client_main.cpp`).
`wait_for_server` itself is a function that returns after one try even when it does not find the server.

## What to do

Fill in the TODOs in `src/add_two_ints_client.cpp`.

| Item | Value |
| --- | --- |
| Node name | `add_two_ints_client` |
| Service name | `add_two_ints` |
| Type | `example_interfaces::srv::AddTwoInts` |

There are two places to fill in.

1. **Constructor**: `client_ = this->create_client<AddTwoInts>("add_two_ints");`
2. **`wait_for_server`**: Call `client_->wait_for_service(timeout)`, and return `true` if it is found.
   If it is not found, follow the same steps as the official tutorial: check `rclcpp::ok()`, print a log, and
   return `false`.

`send_request` is **already written**. `FutureAndRequestId` is a type with no default constructor
(copying is forbidden, only move is allowed), so you cannot write a TODO that "compiles even when its body is empty".
For that reason, this exercise leaves `send_request` out of the TODOs and makes
`create_client` and `wait_for_server` the two places to fill in. Read the body
(`request->a = a; request->b = b; return client_->async_send_request(request);`)
and understand how the request is filled in.

## Run it

After the tests pass, start exercise 04 (`server`) in another terminal, and then you can run it.

```bash
# Terminal 1
source install/setup.bash
ros2 run drill_04_service_server server

# Terminal 2
source install/setup.bash
ros2 run drill_05_service_client client 20 22
```

If a log such as `Sum: 42` appears in terminal 2, it works.

## Common pitfalls

- Always assign the return value of `create_client()` to `client_`. If you receive it in a local variable,
  it is destroyed when the constructor returns, and you cannot use it inside `send_request`.
- When it does not find the server, `wait_for_server` **does not throw an exception**; it only returns `false`.
  Looping and calling it many times is the job of `main()`.
- Use the return value of `client_->wait_for_service(timeout)` as it is.
  You do not need to rebuild `true`/`false` yourself.

## Tests

```bash
./drill run 05
```

The tests do not depend on the `server` of exercise 04. The test itself prepares the server role (a probe node) with
`create_service<AddTwoInts>("add_two_ints", ...)`.

| Test | What it checks |
| --- | --- |
| `add_two_intsのクライアントを作れている` (a client for add_two_ints is created) | The basic form of `create_client` and `wait_for_server` |
| `send_requestで応答のsumが正しい` (the sum in the response from send_request is correct) | Whether the contents of the request arrive correctly (41+1=42) |
| `負の数でも正しく計算できる` (calculates correctly even with negative numbers) | Whether negative `int64_t` values are handled as they are |
| `サーバがいないときwait_for_serverがfalseを返す` (wait_for_server returns false when there is no server) | Whether it returns the return value of `wait_for_service` as it is (it must finish within 1 second) |

## References

- Official: [Writing a simple service and client (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Service-And-Client.html)
- Local example implementation: `/opt/ros/jazzy/share/examples_rclcpp_minimal_client/`
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
