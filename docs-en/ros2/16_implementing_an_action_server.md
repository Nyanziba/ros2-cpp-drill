# ROS 2 Lecture 16: Implementing an Action Server

## Introduction

After this article, you will be able to write an action definition file yourself, implement an ActionServer and an ActionClient in Python, and handle receiving feedback and cancellation from start to finish.

The prerequisite is that you have finished reading [06_services](06_services.md). An action is a service with "progress reports" and "cancellation" added. If you do not understand the synchronous request/response of a service, you cannot see the difference.

## Goals

- Write an `.action` file (three sections: goal/result/feedback) yourself
- Implement `rclpy.action.ActionServer`, and use `publish_feedback` and `succeed` correctly inside `execute_callback`
- Implement the whole flow from `send_goal_async` to the goal response, feedback, and result with `rclpy.action.ActionClient`
- Receive a cancel request, and stop the work safely with `CancelResponse.ACCEPT` and `is_cancel_requested`

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy already set up ([02_environment_setup](02_environment_setup.md) finished)
- `ros-jazzy-example-interfaces` (it contains the Fibonacci action type. If it is not installed, run `sudo apt install ros-jazzy-example-interfaces`)
- A set of `ament_cmake` packages to build your own action definition (we assume you have a Colcon workspace at hand from [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md))
- Three terminals (server, client, and one for checking with `ros2 action`)

### Suggested time plan

- Explain the action as a mix of pub/sub and a service: 10 min
- The `.action` file and rosidl settings: 15 min
- ActionServer implementation (`execute_callback`): 20 min
- ActionClient implementation and checking feedback: 15 min
- Cancel handling and oral questions: 15 min

### Oral questions

**Q1. Explain the difference between a topic, a service, and an action in one sentence each.**

Model answer: A topic is asynchronous one-way delivery, a service is a synchronous request/response, and an action is asynchronous goal-oriented communication that gives progress reports (feedback) and cancellation for work that takes a long time. If you use a service for "long work", the client blocks and keeps waiting for the response, and that is where you need an action.

**Q2. What happens if you `return` in `execute_callback` without calling `goal_handle.succeed()`?**

Model answer: The action status does not change to `SUCCEEDED`. So even if the client receives a result, the state of the goal looks stuck at `EXECUTING`. You must call `succeed()` (or `abort()` on failure, or `canceled()` on cancellation) explicitly to settle the state. Forgetting this call is the most common mistake of this kind.

**Q3. What do you need to implement to accept cancellation?**

Model answer: Pass a `cancel_callback` to the constructor of `ActionServer`, and return `CancelResponse.ACCEPT` there, which accepts the cancel request itself. Actually stopping the work is the job of `execute_callback`. Check `goal_handle.is_cancel_requested` in the loop each time, and when it becomes true, call `goal_handle.canceled()` and then return. If you do not implement `cancel_callback`, the request is rejected by default (`CancelResponse.REJECT`).

## Main text

### What you learn: What is an action

What you learn: understand the problem that actions solve, and their internal structure of three channels.

Preparation: you must have finished [06_services](06_services.md).

Details:

A service blocks until the result returns after you call it. This does not suit work that takes several seconds to several minutes, such as robot navigation. The caller freezes, and you cannot express "I want to stop halfway". To solve these two problems, an action is a mechanism that puts three communication channels on top of a service.

- **Goal**: the target the client sends to the server (equivalent to the request of a service)
- **Result**: the final result when the work is done (equivalent to the response of a service)
- **Feedback**: progress that flows periodically during the work (equivalent to a topic publish)

So it is quick to understand an action as "a service + a topic" combined into one interface. In addition, it has as standard the state transitions: accepting/rejecting a goal, a cancel request, and executing/succeeded/failed.

> Column: `ros2 action list`, `ros2 action info`, and `ros2 action send_goal` are the counterparts of the command groups for topics and services. When you check a running action, get into the habit of typing these first.

### What you learn: Writing the action definition file

What you learn: understand the three-section structure of the `.action` file and the rosidl build settings.

Preparation: a Colcon workspace (for example `~/ros2_ws`) must be ready.

Details:

Following the official tutorial, we make an action that computes the Fibonacci sequence. Make an `action` directory in the package, and put `Fibonacci.action` in it.

```
mkdir -p ~/ros2_ws/src/custom_action_interfaces/action
```

```
# Fibonacci.action
int32 order
---
int32[] sequence
---
int32[] partial_sequence
```

The three sections separated by `---` correspond directly to Goal/Result/Feedback. From the top they are: "up to which term to compute (goal)", "the final sequence (result)", and "the sequence computed so far (feedback)". The field names and types are written in the same way as in a `.msg` file.

Add the dependencies to `package.xml`.

```xml
<buildtool_depend>rosidl_default_generators</buildtool_depend>
<depend>action_msgs</depend>
<member_of_group>rosidl_interface_packages</member_of_group>
```

Add the following to `CMakeLists.txt`.

```cmake
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "action/Fibonacci.action"
)
```

Build, and check that the interface is generated.

```bash
cd ~/ros2_ws
colcon build --packages-select custom_action_interfaces
source install/setup.bash
ros2 interface show custom_action_interfaces/action/Fibonacci
```

If the three sections are shown, it works. The build does not pass if you forget to write the dependency on `action_msgs`, so suspect this first when you get an error.

### What you learn: Implementing an ActionServer

What you learn: implement `execute_callback` with `rclpy.action.ActionServer`, and use `publish_feedback` and `succeed`.

Preparation: the `custom_action_interfaces` package must be built. Also, we have not yet made the Python package that holds the server and client, so we make it here.

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_python --license Apache-2.0 custom_action_python \
  --dependencies rclpy custom_action_interfaces
```

From here on, `ros2 run custom_action_python ...` refers to this package. Do not forget to register the nodes in `entry_points` of `setup.py`.

Details:

Write the server-side node.

```python
import time

import rclpy
from rclpy.action import ActionServer
from rclpy.node import Node

from custom_action_interfaces.action import Fibonacci


class FibonacciActionServer(Node):

    def __init__(self):
        super().__init__('fibonacci_action_server')
        self._action_server = ActionServer(
            self,
            Fibonacci,
            'fibonacci',
            self.execute_callback)

    def execute_callback(self, goal_handle):
        self.get_logger().info('Executing goal...')

        feedback_msg = Fibonacci.Feedback()
        feedback_msg.partial_sequence = [0, 1]

        for i in range(1, goal_handle.request.order):
            feedback_msg.partial_sequence.append(
                feedback_msg.partial_sequence[i] + feedback_msg.partial_sequence[i - 1])
            self.get_logger().info(f'Feedback: {feedback_msg.partial_sequence}')
            goal_handle.publish_feedback(feedback_msg)
            time.sleep(1)

        goal_handle.succeed()

        result = Fibonacci.Result()
        result.sequence = feedback_msg.partial_sequence
        return result


def main(args=None):
    rclpy.init(args=args)
    fibonacci_action_server = FibonacciActionServer()
    rclpy.spin(fibonacci_action_server)


if __name__ == '__main__':
    main()
```

To the constructor of `ActionServer`, you pass `self`, the action type, the action name (`fibonacci`), and `execute_callback`, which is the body of the work. When a goal is accepted, `execute_callback` is called. In it, the code computes the Fibonacci sequence one term at a time, and sends the progress to the client with `goal_handle.publish_feedback(feedback_msg)`. `time.sleep(1)` only imitates real computation time. Without it, there is little point in using an action (an action is too much for work that ends in an instant), so please understand it as a staging device for the lecture.

After the loop, call `goal_handle.succeed()` and then return the Result. **Forgetting this call is the most common bug.** If you write only the return value and forget the status transition, the client sees a half-done state: "the result came, but it is still treated as executing".

### What you learn: Implementing an ActionClient

What you learn: implement the asynchronous flow from `send_goal_async` to the goal response, feedback, and result.

Preparation: start the ActionServer in another terminal.

```bash
ros2 run custom_action_python fibonacci_action_server
```

Details:

The point is that the client cannot be written synchronously. Sending the goal, confirming that the goal was accepted, receiving feedback, and getting the result are all split into separate callbacks.

```python
import rclpy
from rclpy.action import ActionClient
from rclpy.node import Node

from custom_action_interfaces.action import Fibonacci


class FibonacciActionClient(Node):

    def __init__(self):
        super().__init__('fibonacci_action_client')
        self._action_client = ActionClient(self, Fibonacci, 'fibonacci')

    def send_goal(self, order):
        goal_msg = Fibonacci.Goal()
        goal_msg.order = order

        self._action_client.wait_for_server()

        self._send_goal_future = self._action_client.send_goal_async(
            goal_msg,
            feedback_callback=self.feedback_callback)

        self._send_goal_future.add_done_callback(self.goal_response_callback)

    def goal_response_callback(self, future):
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.get_logger().info('Goal rejected')
            return

        self.get_logger().info('Goal accepted')

        self._get_result_future = goal_handle.get_result_async()
        self._get_result_future.add_done_callback(self.get_result_callback)

    def get_result_callback(self, future):
        result = future.result().result
        self.get_logger().info(f'Result: {result.sequence}')
        rclpy.shutdown()

    def feedback_callback(self, feedback_msg):
        feedback = feedback_msg.feedback
        self.get_logger().info(f'Received feedback: {feedback.partial_sequence}')


def main(args=None):
    rclpy.init(args=args)
    action_client = FibonacciActionClient()
    action_client.send_goal(10)
    rclpy.spin(action_client)


if __name__ == '__main__':
    main()
```

The callbacks are called in the following order.

| Order | What is called | Role |
|---|---|---|
| 1 | `send_goal_async` | Sends the goal and returns a future |
| 2 | `goal_response_callback` | Checks whether the server accepted or rejected the goal |
| 3 | `feedback_callback` | Called several times during execution. Receives the progress |
| 4 | `get_result_callback` | Called once after execution is done. Receives the final result |

Run it.

```bash
ros2 run custom_action_python fibonacci_action_client
```

**Exercise**: Run the client, and check that the content of `feedback_callback` keeps flowing to the terminal every second. The server log should also show output at the same timing.

### What you learn: Implementing cancel handling

What you learn: accept the cancel request with `cancel_callback`, and stop the running work safely with `is_cancel_requested`.

Preparation: get the ActionServer implementation file ready for editing.

Details:

The server above does not pass a `cancel_callback` to the constructor of `ActionServer`, so cancel requests are rejected by default. First, add a callback that accepts cancellation.

```python
from rclpy.action import CancelResponse

def cancel_callback(self, goal_handle):
    self.get_logger().info('Received cancel request')
    return CancelResponse.ACCEPT
```

Pass it to the constructor.

```python
self._action_server = ActionServer(
    self,
    Fibonacci,
    'fibonacci',
    execute_callback=self.execute_callback,
    cancel_callback=self.cancel_callback)
```

`cancel_callback` only decides "whether to accept the cancel request". Actually stopping the work is the job of `execute_callback`. You need to check in the loop each time whether a cancel request has come.

```python
def execute_callback(self, goal_handle):
    self.get_logger().info('Executing goal...')

    feedback_msg = Fibonacci.Feedback()
    feedback_msg.partial_sequence = [0, 1]

    for i in range(1, goal_handle.request.order):
        if goal_handle.is_cancel_requested:
            goal_handle.canceled()
            self.get_logger().info('Goal canceled')
            return Fibonacci.Result()

        feedback_msg.partial_sequence.append(
            feedback_msg.partial_sequence[i] + feedback_msg.partial_sequence[i - 1])
        goal_handle.publish_feedback(feedback_msg)
        time.sleep(1)

    goal_handle.succeed()
    result = Fibonacci.Result()
    result.sequence = feedback_msg.partial_sequence
    return result
```

**Exercise**: Run the client with a larger `order` (about 15), and while it runs, check `ros2 action list` and `ros2 action info /fibonacci`, and send a cancel from another terminal. To cancel on the `ActionClient` side, keep the `goal_handle` you received in `goal_response_callback`, and call `goal_handle.cancel_goal_async()`. If `Goal canceled` appears in the server log, the cancellation works.

<details markdown="1"><summary>Answer</summary>

This is an example that adds a cancel call to the client side.

```python
def cancel_goal(self):
    self.get_logger().info('Canceling goal')
    future = self._goal_handle.cancel_goal_async()
    future.add_done_callback(self.cancel_done_callback)

def cancel_done_callback(self, future):
    cancel_response = future.result()
    if len(cancel_response.goals_canceling) > 0:
        self.get_logger().info('Goal successfully canceled')
    else:
        self.get_logger().info('Goal failed to cancel')
```

Note that you need to keep it in `goal_response_callback` with `self._goal_handle = goal_handle`. If you call `cancel_goal` during execution, `is_cancel_requested` becomes true at the next loop check in the server's `execute_callback`, and it stops through `goal_handle.canceled()`. It does not stop immediately, and can stop only at the granularity of the loop (here, every `time.sleep(1)`). This point matters for things like path following on a real robot.

</details>

## Going further

`move_base_flex`, which we use in Part 4, receives path following as an `ExePath` action. Asking the robot to "run this path" is exactly sending a goal. The progress that flows while it runs (where it is on the path now) is exactly feedback. And the request "I want to stop it while it runs" is exactly a cancel. Remember that the structure of the Fibonacci server we wrote here corresponds directly to real robot control such as path following.


## Conclusion

An action is a combination of a topic and a service, so it needs more code than a service. But what you must remember comes down to two things: "the three sections goal/result/feedback", and "settle the state with succeed/abort/canceled". Action servers that forgot to implement cancel handling are common, so after you write one, always get into the habit of testing a cancel halfway. If anything is unclear, ask someone experienced or check the official documentation.

Next, in [17_testing_and_debugging](17_testing_and_debugging.md), we apply testing and debugging methods to the nodes we have written so far.

### Matching exercise

After reading this chapter, practice with the matching drill.

- `10_action_server` — Build an action server

```bash
./drill run 10
```

From an exercise, you can come back to this chapter with `./drill read`.

## References

- [ROS 2 Documentation: Jazzy — Writing an action server and client (Python)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Writing-an-Action-Server-Client/Py-Action-Server-Client.html)
- [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md)
- [17_testing_and_debugging](17_testing_and_debugging.md)
