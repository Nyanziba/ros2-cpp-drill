# ROS 2 Lecture 05: Topics

## Introduction

When you finish this article, you understand how data flows between nodes using topics, and you can drive the `turtlesim` turtle by hand from the command line.

The prerequisite is that you have read [04_nodes](04_nodes.md). If you do not know "what a node is", the explanation of topics has nothing to stand on.

## Lecture goals

- Understand the pub/sub model of topics and explain it in your own words
- Actually type the `ros2 topic` commands (list/echo/info/hz/pub)
- Understand the difference between the two basic QoS settings (Reliable/Best Effort) and decide which one to use

## Using this as a course

### Things to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy Jalisco already set up ([02_environment_setup](02_environment_setup.md) done)
- The `ros-jazzy-turtlesim` package (if it is not installed, run `sudo apt install ros-jazzy-turtlesim`)
- A screen where you can open two or three terminals side by side (tmux or window splitting are both fine)

### Time allocation guide

- Explaining the pub/sub model: 10 min
- Command practice (list/echo/info/hz): 15 min
- Task of turning the turtle by hand: 15 min
- Explaining QoS and oral exam: 10 min

### Oral exam

**Q1. What is the biggest difference between a topic and a service?**

Model answer: A topic is asynchronous one-way delivery (pub/sub). The sender does not care whether there is a receiver or a response. A service is a synchronous request/response, and the caller waits for the result. "Data that keeps flowing", such as sensor values and `cmd_vel`, suits topics. "I want to ask for a computed result just once" suits services.

**Q2. Explain the command to drive the turtle with `ros2 topic pub` in both ways: sending the message once (one-shot) and sending continuously.**

Model answer: `ros2 topic pub /turtle1/cmd_vel geometry_msgs/msg/Twist "{linear: {x: 2.0}, angular: {z: 1.8}}"` sends the message once and ends. If you add the `--rate` option, as in `ros2 topic pub --rate 1 /turtle1/cmd_vel geometry_msgs/msg/Twist "{linear: {x: 2.0}, angular: {z: 1.8}}"`, it keeps sending once per second, and the turtle keeps moving until you press Ctrl+C. With a single send, the turtle moves a little and stops right away, so if you want it to draw a circle, you need continuous sending.

**Q3. Explain why Best Effort tends to be used for sensor topics such as LiDAR.**

Model answer: Sensor data keeps flowing at a high rate. Even if you miss one message, the next one comes soon, so you want the newer value rather than paying the cost of resending. With Reliable, buffers and acknowledgments for resending are needed, which increases delay and load. On the other hand, an important one-time command or a setting change should be delivered reliably with Reliable.

## Main content

### What you learn: What is a topic

What you learn: Understand the basics of the pub/sub model.

Preparation: You have finished up to [04_nodes](04_nodes.md).

Content:

ROS 2 nodes hardly ever call each other directly. Instead, they interact indirectly: they put messages into a named channel called a topic (publish), and the nodes that are interested receive them (subscribe). The sender (publisher) does not know who receives, and the receiver (subscriber) does not know who sends. This asynchronous, many-to-many structure supports ROS 2 distributed systems.

LiDAR scan values, IMU attitude, remaining battery, velocity commands to motors: most data that flows inside a robot is topics. Think of topics as the blood flow of a robot. If the blood stops, the robot does not move. If you look at the blood flow, you can roughly tell the state of the robot. The first step of debugging is to check "what is flowing on which topic now".

### What you learn: Peek at topics

What you learn: Check running topics with the `ros2 topic` command.

Preparation: Start turtlesim.

```bash
ros2 run turtlesim turtlesim_node
```

Content:

Open another terminal and first list which topics exist.

```bash
ros2 topic list
```

You should see `/turtle1/cmd_vel`, `/turtle1/pose`, `/turtle1/color_sensor`, and so on. The name alone does not tell you the type, so to see the type too, add `-t`.

```bash
ros2 topic list -t
```

To actually see what flows, use `echo`.

```bash
ros2 topic echo /turtle1/pose
```

If the turtle is not moving, nothing is shown, and that is also correct behavior. A topic that nothing is sent to has nothing flowing. Next, start teleop from another terminal and move the turtle with the arrow keys.

```bash
ros2 run turtlesim turtle_teleop_key
```

While you press the arrow keys, check that coordinates and velocities keep flowing in real time on the `echo` side.

You can see the detailed information of a topic (type, number of publishers, number of subscribers) with `info`.

```bash
ros2 topic info /turtle1/cmd_vel
```

```
Type: geometry_msgs/msg/Twist
Publisher count: 1
Subscriber count: 1
```

`turtle_teleop_key` is the publisher, and `turtlesim_node` is the subscriber. Just by following these numbers, you can see quite well which programs are connected and how.

When you want to know how often messages flow, use `hz`.

```bash
ros2 topic hz /turtle1/pose
```

`turtlesim_node` keeps publishing its own coordinates internally at a fixed rate (by default about 60 Hz), so values appear even when it is not moving.

### What you learn: Check the message type

What you learn: Check the structure of messages that flow on a topic with `ros2 interface show`.

Preparation: Nothing in particular.

Content:

What values to send to `cmd_vel` can be known by looking at the message type.

```bash
ros2 interface show geometry_msgs/msg/Twist
```

```
# This expresses velocity in free space broken into its linear and angular parts.

Vector3  linear
        float64 x
        float64 y
        float64 z
Vector3  angular
        float64 x
        float64 y
        float64 z
```

`Twist` is a simple structure that only has two `Vector3` fields: `linear` (translational velocity) and `angular` (angular velocity). For a robot that runs on the ground, the basic way is to use only `linear.x` (forward/backward) and `angular.z` (turning). `linear.y`, `angular.x`, and others are often ignored (turtlesim is the same).

Hint: When you do not know a message type, do not panic. Get into the habit of typing `ros2 interface show <type name>`. You do not need to memorize field names. Looking them up every time is enough.

### What you learn: Turn the turtle by hand

What you learn: Send messages directly to a topic with `ros2 topic pub`.

Preparation: turtlesim is running. You can end teleop.

Content:

Let us send a velocity directly to `cmd_vel` from the command line.

```bash
ros2 topic pub --rate 1 /turtle1/cmd_vel geometry_msgs/msg/Twist "{linear: {x: 2.0}, angular: {z: 1.8}}"
```

`--rate 1` means that it keeps sending this message once per second. We give a forward speed of 2.0 and an angular speed of 1.8 at the same time, so the turtle moves while drawing a circle. Stop it with Ctrl+C.

**Practice question**: Predict how the turtle moves if you type the same command without `--rate`, and run it to check. Also think about how to change the ratio of `linear.x` to `angular.z` to draw a circle with a larger radius.

<details markdown="1"><summary>Answer</summary>

Without `--rate`, `ros2 topic pub` sends the message once and ends. The turtle draws an arc for only a very short distance and stops. To keep it moving, you need to repeat sending with `--rate`.

The radius of the circle is roughly determined by the ratio of forward speed to angular speed. If you make the angular speed `angular.z` smaller, or the forward speed `linear.x` larger, the distance moved in the same time increases while the change of heading stays the same, so the circle gets bigger. Conversely, if you make `angular.z` larger, you get a smaller circle.

</details>

### What you learn: A taste of QoS

What you learn: Know that topics have communication quality settings (QoS: Quality of Service).

Preparation: Nothing in particular.

Content:

For a topic, you can set not only "what" flows but also "how" it flows. This is QoS (Quality of Service). Two commonly used settings are the following.

| Setting | Behavior | Where it is used |
|---|---|---|
| Reliable | Resends until it arrives. Does not allow dropped messages | `cmd_vel`, important commands used like a service |
| Best Effort | If it does not arrive, it stays lost. Does not resend | Sensors such as LiDAR/IMU, video |

Best Effort is the standard for sensor topics. The reason is as explained in oral exam Q3. For data that keeps flowing at a high rate, it is fine to miss one if the next one comes, so not paying the cost of resending gives lower latency. Conversely, an important command that is sent only once should be delivered reliably with Reliable.

To check the QoS of a running topic, use the `-v` (verbose) option.

```bash
ros2 topic info -v /turtle1/cmd_vel
```

Lines such as `Reliability: RELIABLE` are included in the output. If the QoS of the publisher and the subscriber do not match, communication may not connect. So when "the topic should be connected, but nothing appears in `echo`", suspect this `-v` output first.

> Column: QoS has several axes besides Reliability: History (whether to keep only the latest N messages or all of them), Depth (the number N), Durability (whether to give past messages to a subscriber that connects later), and so on. History and Depth are different policies, and the `10` in `create_publisher(..., 10)` is the Depth. We only touch on them here, but you will face them again when you do sensor integration on a real robot ([21_sensor_integration](21_sensor_integration.md)).

## Advanced

Behind topic pub/sub, a communication middleware called DDS (Data Distribution Service) is actually running. It is quicker to understand if you think of ROS 2 as a thin API laid over DDS.

DDS has a mechanism called discovery, with which nodes find each other. Thanks to this discovery, you can see other nodes and topics with `ros2 node list` and `ros2 topic list`. If ROS 2 from several environments runs on the same network, you may see each other's topics unintentionally. There is an environment variable `ROS_DOMAIN_ID` to avoid this, but here it is enough to know its name.

We go deeper in [05b_dds_discovery_and_ros_domain_id](05b_dds_discovery_and_ros_domain_id.md). **If several people work in the club room and you run into "I can see other people's nodes" or "my tests fail", read that article first.** Setting two environment variables solves it. While you work alone, you can go on to the next article ([06_services](06_services.md)).

## Summary

Topics are the most frequently used mechanism in ROS 2. You will use the commands you typed here (list/echo/info/hz/pub) again and again, so type them many times until your hands remember. If something is unclear, ask a senior member.

Next, in [06_services](06_services.md), we cover "request/response" communication, which is the counterpart of topics.

### Matching exercise

After you read this chapter, practice with the matching drill exercise.

- `12_qos` — Design a QoS profile

```bash
./drill run 12
```

From the exercise side, you can come back to this chapter with `./drill read`.

## References

- [ROS 2 Documentation: Jazzy — Understanding topics](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.html)
- [04_nodes](04_nodes.md)
- [06_services](06_services.md)
