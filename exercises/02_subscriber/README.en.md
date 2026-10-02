# Exercise 02: Subscribe to a topic [Beginner]

Write the `MinimalSubscriber` from the official tutorial
[Writing a simple publisher and subscriber (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)
exactly as it is.

## What to do

Fill in the TODOs in `src/minimal_subscriber.cpp`. The specification is the same as the official tutorial.

| Item | Value |
| --- | --- |
| Node name | `minimal_subscriber` |
| Topic name | `topic` |
| Type | `std_msgs::msg::String` |
| QoS depth | 10 |
| Callback argument | `const std_msgs::msg::String & msg` (a reference, not a pointer) |
| Log | `I heard: '<message text>'` |

The class declaration (`include/drill/minimal_subscriber.hpp`) is provided. Think about
what goes into the member variable `subscription_`.

## Run it

After the tests pass, you can run it by hand, just like the official tutorial.

```bash
source install/setup.bash
ros2 run drill_02_subscriber listener
```

In another terminal:

```bash
ros2 topic pub /topic std_msgs/msg/String "{data: 'Hello'}" --once
ros2 node info /minimal_subscriber
```

If `I heard: 'Hello'` appears in the terminal where `listener` is running, it works.

## Common pitfalls

- Always assign the return value of `create_subscription()` to `subscription_`.
  If you receive it in a local variable, it is destroyed when the constructor returns, and the callback is never called.
- The callback argument is `const std_msgs::msg::String & msg`. Do not confuse it with the official
  publisher side (the example that receives a pointer, such as `msg->data`).
- To use `_1` in `std::bind(&MinimalSubscriber::topic_callback, this, _1)`,
  you need `using std::placeholders::_1;`.

## Tests

```bash
./drill run 02
```

| Test | What it checks |
| --- | --- |
| `topicを購読してログに出している` (subscribes to topic and prints a log) | Whether the subscription is running and prints `I heard: '<message text>'` |
| `複数通受信しても毎回ログが出る` (prints a log every time, even for multiple messages) | Whether the subscription stops after the first message |
| `ノード名がminimal_subscriberになっている` (the node name is minimal_subscriber) | Setting the node name in the constructor |

## References

- Official: [Writing a simple publisher and subscriber (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)
- Local example implementation: `/opt/ros/jazzy/share/examples_rclcpp_minimal_subscriber/`
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
