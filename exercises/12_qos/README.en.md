# Exercise 12: Design a QoS profile [Advanced]

You build the pattern "deliver the last value even to a subscriber that starts later" (TRANSIENT_LOCAL)
from the official document
[About Quality of Service settings](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Quality-of-Service-Settings.html)
into both the publisher and the subscriber with your own hands.

## What is QoS

QoS (Quality of Service) is "a contract between the sender and the receiver". Even if the topic name and type
match, if the QoS does not fit together, they **silently do not connect**. No error
appears. You only get a state where, even if you look at the logs, it is publishing but nothing arrives.
That is why the best way to learn QoS is "first learn the specification, then create a state where they do not connect
yourself and experience how it breaks".

## List of QoS policies

| Policy | What it decides |
| --- | --- |
| History | How to count the messages a Publisher/Subscription keeps internally. `KEEP_LAST` (the latest depth messages) and `KEEP_ALL` (everything, as far as allowed). |
| Depth | The queue length when History is `KEEP_LAST`. When it overflows, the oldest ones are dropped first. |
| Reliability | The delivery guarantee. `RELIABLE` delivers even by resending. `BEST_EFFORT` is fine if it does not arrive (good for sensors, where a newer value is enough when it comes). |
| Durability | Whether to give past messages to a subscriber that comes later. `VOLATILE` (the default. Only what comes after the subscription starts) and `TRANSIENT_LOCAL` (the Publisher keeps the last value and delivers it to a subscriber that comes later). |
| Deadline | The expectation that "this topic should be updated at least at about this period". The application can be notified when it is broken. |
| Lifespan | The expiry time of a message. A message past it is dropped (even if it has not been delivered yet). |
| Liveliness | How to decide that a Publisher is alive. Used to detect a node going down. |

This exercise covers four of them: History / Depth / Reliability / Durability.

## When you need TRANSIENT_LOCAL and when you do not

**When you need it** (data that does not change, but is always needed by a node that starts later too)
- Maps (a static map such as `/map`. It does not change until it is rebuilt)
- Robot description (`/robot_description`. The URDF stays the same after startup)
- Static configuration values (calibration results, distributing initial parameters, and so on)

**When you do not need it** (data where new values keep coming, and old values have no value)
- Raw sensor data (LiDAR or camera. A value from 1 second ago has no meaning if you get it later)
- High-frequency odometry or control commands

If you make a sensor topic TRANSIENT_LOCAL, the Publisher side only pays the cost of
keeping past messages and gains nothing. Conversely, if you make a map VOLATILE,
a node that starts later can never receive that map.

## What happens when QoS is incompatible

**They silently do not connect.** It appears in `ros2 topic list` and in `ros2 node info`, and
`rclcpp::Node::count_publishers()` / `count_subscribers()` may even
return values greater than 0 (because discovery itself works). But no actual messages
arrive.

You check whether they are connected with `ros2 topic info -v <topic>`. It shows the actual QoS
(RELIABILITY / DURABILITY / ...) of each Publisher and
Subscription, so compare both. If they differ, that is why they do not connect.

## Ready-made QoS profiles

You do not have to build an `rclcpp::QoS` every time. Ready-made profiles are provided for common uses.

| Profile | Main settings | Use |
| --- | --- | --- |
| `rclcpp::SensorDataQoS()` | `BEST_EFFORT` + a small depth | Sensor data such as cameras and LiDAR, where low latency matters more than not dropping data |
| `rclcpp::SystemDefaultsQoS()` | The RMW defaults | When you have no particular preference (the same family as the 10 in `create_publisher(topic, 10)`) |
| `rclcpp::ServicesQoS()` | `RELIABLE` + `VOLATILE` | Internal topics of services and actions |
| `rclcpp::ParametersQoS()` | `RELIABLE` + `VOLATILE`, a larger depth | Things related to the parameter services |

When you want "latched delivery of maps and configuration values", as in this exercise, no ready-made profile
fits exactly, so you build an `rclcpp::QoS` yourself.

## What to do

Fill in the TODOs in `src/qos_nodes.cpp`.

| Item | Value |
| --- | --- |
| Node name of `LatchedPublisher` | `latched_publisher` |
| Node name of `LatchedSubscriber` | `latched_subscriber` |
| Topic name | `config` |
| Type | `std_msgs::msg::String` |
| QoS | `KeepLast(1)` + `TRANSIENT_LOCAL` + `RELIABLE` (the same for both the publisher and the subscription) |
| Publish log | `Published config: '<value>'` |
| Receive log | `Received config: '<value>'` |

The class declaration (`include/drill/qos_nodes.hpp`) is provided. The key point is to
**pass exactly the same QoS to both the publisher and the subscription**.
Making only one side `transient_local()` is meaningless.

## Run it

```bash
source install/setup.bash
ros2 run drill_12_qos qos_demo
```

`qos_demo` creates a publisher and publishes once, and then **waits 2 seconds** before
starting the subscriber. If `Received config:` still appears in the log,
that is proof that TRANSIENT_LOCAL is working.

In another terminal:

```bash
ros2 topic info -v /config
ros2 topic echo /config --qos-durability transient_local --qos-depth 1
```

With `ros2 topic info -v`, you can check the actual QoS (DURABILITY, RELIABILITY, and so on) of both the publisher and the subscription.
The default of `ros2 topic echo` is `VOLATILE`, so unless you add `--qos-durability transient_local`,
nothing is shown even if you run it after qos_demo has finished publishing
(this is also an example of "they do not connect because they are incompatible").

## Common pitfalls

- What you pass as the QoS argument of `create_publisher()` / `create_subscription()` is
  an `rclcpp::QoS` object. If you pass an integer such as `10`, you get the default QoS with depth 10
  (`VOLATILE` + `RELIABLE`), and it is not
  TRANSIENT_LOCAL.
- If you make only one of the publisher and the subscription `transient_local()`, they
  do not connect. Pass the same QoS to both.
- `actual_qos()` returns not the "requested QoS" but the "QoS that was actually adopted"
  (`get_actual_qos()`). Depending on the RMW, it can differ from the request, so
  the tests always look at this one.

## Tests

```bash
./drill run 12
```

| Test | What it checks |
| --- | --- |
| `publisherの実効QoSがTRANSIENT_LOCALかつRELIABLEでdepth1になっている` (the effective QoS of the publisher is TRANSIENT_LOCAL and RELIABLE with depth 1) | The contents of `actual_qos()` |
| `あとから起動した購読者にも過去にpublishした値が届く` (a subscriber that starts later also receives the value published in the past) | The essence of TRANSIENT_LOCAL (latched delivery) |
| `新しい値をpublishすれば購読者に届く` (if you publish a new value, it reaches the subscriber) | Whether the normal delivery path is not broken |
| `VOLATILEで購読すると過去の値は届かない` (subscribing with VOLATILE does not receive past values) | Confirming that durability is decided by "the setting the subscriber requested" |

## References

- Official: [About Quality of Service settings](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Quality-of-Service-Settings.html)
- Demo: [quality_of_service_demo](https://github.com/ros2/demos/tree/jazzy/quality_of_service_demo)
- Related local package: `/opt/ros/jazzy/share/quality_of_service_demo_cpp/`
- How it works: [docs-en/rclcpp_design_philosophy.md](../../docs-en/rclcpp_design_philosophy.md)
