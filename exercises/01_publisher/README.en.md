# Exercise 01: Publish to a topic [Beginner]

Write the `MinimalPublisher` from the official tutorial
[Writing a simple publisher and subscriber (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)
exactly as it is.

## What to do

Fill in the TODOs in `src/minimal_publisher.cpp`. The specification is the same as the official tutorial.

| Item | Value |
| --- | --- |
| Node name | `minimal_publisher` |
| Topic name | `topic` |
| Type | `std_msgs::msg::String` |
| QoS depth | 10 |
| Period | 500ms |
| Message text | `"Hello, world! " + std::to_string(count_++)` |
| Log | `Publishing: '<message text>'` |

The class declaration (`include/drill/minimal_publisher.hpp`) is provided. Think about
what goes into the member variables `publisher_` / `timer_` / `count_`.

## Run it

After the tests pass, you can run it by hand, just like the official tutorial.

```bash
source install/setup.bash
ros2 run drill_01_publisher talker
```

In another terminal:

```bash
ros2 topic echo /topic
ros2 topic hz /topic          # is it 2Hz?
ros2 node info /minimal_publisher
```

## Common pitfalls

- Always assign the return values of `create_publisher()` / `create_wall_timer()` to member variables.
  If you receive them in local variables, they are destroyed when the constructor returns, and nothing happens.
- Do not forget the `this` in `std::bind(&MinimalPublisher::timer_callback, this)`.
- `count_++` is **post-increment**. The first message is `Hello, world! 0`.

## Tests

```bash
./drill run 01
```

| Test | What it checks |
| --- | --- |
| `topicトピックにpublishしている` (publishes to the topic `topic`) | Whether the publisher and the timer are running |
| `本文がHello_worldと連番になっている` (the message text is Hello_world with a sequence number) | Building the message text and `count_++` |
| `おおよそ500ミリ秒周期でpublishしている` (publishes at about a 500 ms period) | The timer period |
| `公式と同じPublishingログを出している` (prints the same Publishing log as the official one) | The format of `RCLCPP_INFO` |

## References

- Official: [Writing a simple publisher and subscriber (C++)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html)
- Local example implementation: `/opt/ros/jazzy/share/examples_rclcpp_minimal_publisher/`
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
