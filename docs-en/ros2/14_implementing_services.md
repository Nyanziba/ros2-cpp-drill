# ROS 2 Lecture 14: Implementing Services

## Introduction

In [06_services](06_services.md) you used `ros2 service call` as the user of a service. This time, on the contrary, you implement a service server and a client yourself. After this article, you can write and build the `add_two_ints` service in Python, and you can explain both the correct way to write an asynchronous call and the way you must not write it (the way that deadlocks).

We use Python. You should already have the package layout of rclpy and how to write `setup.py` from [12_writing_pub_sub_in_python](12_writing_pub_sub_in_python.md), so you can read this as a continuation. The official tutorial also has a C++ version, but in most cases the body of a service server is a thin layer that "just calls higher-level logic with the received values", and it is common to write this layer in Python. If you want to write it in C++, see the `rclcpp` version of the tutorial.

We assume Ubuntu 24.04 / ROS 2 Jazzy.

## Lecture goals

- You can write both the server and the client with `create_service` and `create_client`
- You can implement the asynchronous call of a client (`call_async` + `spin_until_future_complete`)
- You can explain why calling a service synchronously inside a callback causes a deadlock
- You can check the behavior of your own service with `ros2 service call`

## Using this as a lecture

### What to prepare

- A terminal with Ubuntu 24.04 / ROS 2 Jazzy set up ([02_environment_setup](02_environment_setup.md) finished)
- The workspace from [12_writing_pub_sub_in_python](12_writing_pub_sub_in_python.md) is at hand (we assume `~/ros2_ws`)
- The `example_interfaces` package (the type of `add_two_ints` is included in it by default. You may need `sudo apt install ros-jazzy-example-interfaces`)

### Oral quiz (with model answers)

**Q1. What does `call_async` return? Can you take out the value of the response right after calling it?**

Model answer: `call_async` immediately returns a `Future` object. At the moment right after the call, the request has only been sent to the server, and the response has not come back yet. To take out the value, you must wait until the Future completes (until `future.done()` becomes true), for example with `spin_until_future_complete`, and then call `future.result()`.

**Q2. What happens if you call a service client of the same node synchronously inside a callback function (in the form that waits for the Future to complete by blocking, with `spin_until_future_complete`)?**

Model answer: It does not work. `spin_until_future_complete` is a function that spins the executor in a blocking way and waits for the Future to complete. But if the caller is already spinning that executor and you call it from inside a callback, the next spin that is needed to receive the response cannot be run. The rclpy of Jazzy and later detects this re-entry and guards against it, so instead of freezing silently, `RuntimeError: Executor is already spinning` is raised and the node terminates abnormally (the guard itself is `Executor._enter_spin` in `/opt/ros/jazzy/lib/python3.*/site-packages/rclpy/executors.py`). It is especially easy to step on with a single-threaded executor.

**Q3. What happens if you start only the client and call `call_async` while you have not implemented the server?**

Model answer: It does not crash immediately with an error. The request is sent, but because there is no server, the response never comes back, and the Future keeps waiting without completing. The correct way is to check that the server exists with `wait_for_service` before calling, and if you skip this, it easily becomes a state where "you cannot tell whether it is running or frozen".

### Suggested time plan

- Overall picture of implementing a service, and creating the package: 10 minutes
- Implementing the server and checking it (`ros2 service call`): 15 minutes
- Implementing the client and the asynchronous call: 15 minutes
- Explaining the deadlock trap and oral quiz: 15 minutes

## Main text

### Create the package

A service type named `add_two_ints`, with a request (`int64 a`, `int64 b`) and a response (`int64 sum`), is already defined in the `example_interfaces` package. This time we do not make a custom interface, and we use this existing type (for how to make your own `.srv`, see [13_custom_interfaces](13_custom_interfaces.md)).

Create a new Python package in `src` of the workspace.

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_python --license Apache-2.0 py_srvcli
```

Add the dependencies to `package.xml`.

```xml
<depend>rclpy</depend>
<depend>example_interfaces</depend>
```

### Write the server

Create `py_srvcli/py_srvcli/service_member_function.py`.

```python
# ~/ros2_ws/src/py_srvcli/py_srvcli/service_member_function.py
from example_interfaces.srv import AddTwoInts

import rclpy
from rclpy.node import Node


class MinimalService(Node):

    def __init__(self):
        super().__init__('minimal_service')
        self.srv = self.create_service(
            AddTwoInts, 'add_two_ints', self.add_two_ints_callback)

    def add_two_ints_callback(self, request, response):
        response.sum = request.a + request.b
        self.get_logger().info(
            'Incoming request\na: %d b: %d' % (request.a, request.b))

        return response


def main():
    rclpy.init()

    minimal_service = MinimalService()

    rclpy.spin(minimal_service)

    rclpy.shutdown()


if __name__ == '__main__':
    main()
```

The first argument of `create_service` is the service type, the second is the service name, and the third is the callback that receives and handles the request. The callback receives two objects, `request` and `response`, fills in the fields of `response`, and `return`s it. The structural difference is that the subscriber callback of a topic only received a value, while the callback of a service returns a value.

Register the executable in `entry_points` of `setup.py`.

```python
entry_points={
    'console_scripts': [
        'service = py_srvcli.service_member_function:main',
    ],
},
```

### Build and run the server

```bash
cd ~/ros2_ws
colcon build --packages-select py_srvcli
source install/setup.bash
ros2 run py_srvcli service
```

From another terminal, check the behavior with the `ros2 service call` you used in 06.

```bash
ros2 service call /add_two_ints example_interfaces/srv/AddTwoInts "{a: 3, b: 4}"
```

<!-- measure: env=ros files=src/py_srvcli/py_srvcli/service_member_function.py cmd="python3 src/py_srvcli/py_srvcli/service_member_function.py >/dev/null 2>&1 & S=$!; sleep 4; ros2 service call /add_two_ints example_interfaces/srv/AddTwoInts '{a: 3, b: 4}'; kill $S" filter="grep -v '^waiting for service'" -->
```
requester: making request: example_interfaces.srv.AddTwoInts_Request(a=3, b=4)

response:
example_interfaces.srv.AddTwoInts_Response(sum=7)
```

An `Incoming request` log should also appear in the terminal on the server side. With this, you can confirm "the service you made responds correctly" with only the command line. This time `ros2 service call` is itself the means of checking, and there is not enough here to set it as a separate exercise.

### Write the client

The side that calls the server is also written as a node. Create `py_srvcli/py_srvcli/client_member_function.py`.

```python
# ~/ros2_ws/src/py_srvcli/py_srvcli/client_member_function.py
import sys

from example_interfaces.srv import AddTwoInts
import rclpy
from rclpy.node import Node


class MinimalClientAsync(Node):

    def __init__(self):
        super().__init__('minimal_client_async')
        self.cli = self.create_client(AddTwoInts, 'add_two_ints')
        while not self.cli.wait_for_service(timeout_sec=1.0):
            self.get_logger().info('service not available, waiting again...')
        self.req = AddTwoInts.Request()

    def send_request(self, a, b):
        self.req.a = a
        self.req.b = b
        return self.cli.call_async(self.req)


def main():
    rclpy.init()

    minimal_client = MinimalClientAsync()
    future = minimal_client.send_request(int(sys.argv[1]), int(sys.argv[2]))
    rclpy.spin_until_future_complete(minimal_client, future)
    response = future.result()
    minimal_client.get_logger().info(
        'Result of add_two_ints: for %s + %s = %s' %
        (sys.argv[1], sys.argv[2], response.sum))

    minimal_client.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
```

The flow breaks down into the following three steps.

1. Make the client with `create_client`. Check with `wait_for_service` whether the server is up (if it is not up, it waits while printing a log every second)
2. Send the request with `call_async`. What comes back at this point is **not the result yet but a Future**
3. Spin the executor with `spin_until_future_complete` until the Future completes, and after it completes, take out the actual response with `future.result()`

Try what happens if you remove `wait_for_service`. If you run only the client without starting the server, as in oral quiz Q3, there is no error, and it just stops with no response. It is a good experiment to avoid the hasty conclusion "there is no error, so it is working".

Add it to `setup.py` too.

```python
entry_points={
    'console_scripts': [
        'service = py_srvcli.service_member_function:main',
        'client = py_srvcli.client_member_function:main',
    ],
},
```

Build and run both. Keep the server running, and run in another terminal.

```bash
colcon build --packages-select py_srvcli
source install/setup.bash
ros2 run py_srvcli client 2 3
```

<!-- measure: env=ros files=src/py_srvcli/py_srvcli/service_member_function.py,src/py_srvcli/py_srvcli/client_member_function.py cmd="python3 src/py_srvcli/py_srvcli/service_member_function.py >/dev/null 2>&1 & S=$!; sleep 4; python3 src/py_srvcli/py_srvcli/client_member_function.py 2 3; kill $S" filter="grep -v 'service not available'" -->
```
[INFO] [1790937390.907519089] [minimal_client_async]: Result of add_two_ints: for 2 + 3 = 5
```

### Calling synchronously inside a callback causes a deadlock

This is the most important trap of this lecture. `spin_until_future_complete` is a function that "waits for the Future to complete while spinning the executor". There is no problem if you call it outside another callback, that is, in a place such as `main`. But it is dangerous to write it as follows.

```python
class BadExample(Node):

    def __init__(self):
        super().__init__('bad_example')
        self.cli = self.create_client(AddTwoInts, 'add_two_ints')
        self.timer = self.create_timer(1.0, self.timer_callback)

    def timer_callback(self):
        req = AddTwoInts.Request()
        req.a = 1
        req.b = 2
        future = self.cli.call_async(req)
        rclpy.spin_until_future_complete(self, future)  # RuntimeError happens here
        self.get_logger().info(f'sum: {future.result().sum}')
```

`timer_callback` is already called by the spin of the executor. If you call `spin_until_future_complete` again inside it, the next spin is needed to receive the response, but you keep waiting without being able to move inside the current spin. With a single-threaded executor, while one callback is running, the other callbacks (subscription callbacks and the completion handling of the Future) are not run. In other words, if you wait synchronously for a service of the same node from inside a callback, that node itself can never get into a state where it can receive the answer. The rclpy of Jazzy and later detects this situation and raises `RuntimeError: Executor is already spinning`, so in practice it does not "freeze", but **it raises an exception on the spot and crashes**. This is kinder than hanging silently, but in any case this way of writing does not work.

The correct fix is one of the following.

- Inside the callback, only call `call_async` and keep the Future, and leave the completion check to another mechanism (register a callback with `future.add_done_callback`, or poll `future.done()` with another timer)
- Prepare a separate node, a separate executor, or a separate thread for the service client (use a `MultiThreadedExecutor`, or split it into another process)
- Review the design itself, "a design that needs to call a service synchronously from inside a callback". Check that you are not forcing a service to do what should be built as an action

> Column: With `add_done_callback`, you can register a function that is called automatically when the Future completes. You can receive the result without blocking, so this is the basic pattern when you want to call a service from inside a callback. You write it like `future.add_done_callback(self.response_callback)`, and take out `future.result()` inside `response_callback(self, future)`.

**Exercise**: Actually run the `BadExample` above and check that it crashes with an error, and then rewrite it into a form that works using `add_done_callback`.

<details markdown="1"><summary>Answer</summary>

You can check the broken version by starting the server and running `BadExample` with `rclpy.spin(node)`. At the first call of `timer_callback`, `RuntimeError: Executor is already spinning` is raised, and the process crashes with `[ros2run]: Process exited with failure 1`. Materials for Humble and earlier sometimes explain that it "freezes", but in Jazzy and later a guard is in place, so it fails right away with an error.

The skeleton of the rewritten version is as follows.

```python
def timer_callback(self):
    req = AddTwoInts.Request()
    req.a = 1
    req.b = 2
    future = self.cli.call_async(req)
    future.add_done_callback(self.response_callback)

def response_callback(self, future):
    self.get_logger().info(f'sum: {future.result().sum}')
```

`timer_callback` returns right after it calls `call_async`. When the response actually arrives, the executor calls `response_callback`. The difference from the freezing version is that there is no blocking wait inside the callback.

</details>

## Going further

The comparison table in article 06 (continuous data goes to topics, one-time operations go to services) should feel more real once you have implemented it. One more note from the implementation view: with a service, the caller needs to know beforehand how long the server-side processing takes. This time `add_two_ints` is an addition, so it finishes in an instant, but if it were processing that takes hundreds of ms, then in a design that waits synchronously, that thread of the client freezes completely during that time. Processing that takes seconds to minutes, processing where you want to know the progress, and processing you want to cancel should be built with actions, not services. This is the conclusion mentioned in the Going further section of 06, and it is covered in [16_implementing_an_action_server](16_implementing_an_action_server.md).

Also, the server this time received a request, calculated synchronously, and returned the `response` right away. If you make a multi-stage setup where the server calls another service inside it, the deadlock trap of this time applies to the server side as it is. Be careful not to call another service synchronously inside a server callback.


## Wrapping up

You implemented the service server and the client yourself, and covered the correct form of the asynchronous call and the reason why a synchronous call inside a callback causes a deadlock. Next is [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md), which covers `declare_parameter` and YAML injection from launch. If you do not understand something, ask someone experienced or check the official documentation.

### Matching exercises

After you read this chapter, practice with the matching drills.

- `04_service_server` — Build a service server
- `05_service_client` — Build a service client
- `13_executors` — Executors and callback groups

```bash
./drill run 04
./drill run 05
./drill run 13
```

From the exercise, you can return to this chapter with `./drill read`.

## References

- [ROS 2 Documentation (Jazzy): Writing a simple service and client (Python)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Py-Service-And-Client.html)
- Previous: [13_custom_interfaces](13_custom_interfaces.md)
- Next: [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md)
