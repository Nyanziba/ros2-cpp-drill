# ROS 2 Lecture 18: TF2 and Coordinate Frames

## Introduction

After this article, you will understand why a robot has several coordinate frames at the same time, and you will be able to check how coordinate transformations really look with the `tf2` tools (`tf2_echo`, `view_frames`). You will also be able to publish one coordinate transformation yourself with `static_transform_publisher`.

The prerequisites are the content up to [17_testing_and_debugging](17_testing_and_debugging.md), and some experience handling sensors such as LiDAR and cameras through topics. TF2 is the foundation of sensor integration, so be sure to learn it before [21_sensor_integration](21_sensor_integration.md).

## Goals

- Explain in your own words why a robot needs several coordinate frames
- Understand the roles in the standard setup `map` → `odom` → `base_link` (REP 105)
- Understand the difference between static TF and dynamic TF, and run `static_transform_publisher` from the CLI
- Check the state of the TF tree with `tf2_echo` and `view_frames`
- Explain why `ExtrapolationException` happens in `lookup_transform`

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy already set up ([02_environment_setup](02_environment_setup.md) finished)
- The `ros-jazzy-turtlesim` package
- The `ros-jazzy-turtle-tf2-py` package (`sudo apt install ros-jazzy-turtle-tf2-py`. It contains the official TF2 demo for turtlesim)
- The `ros-jazzy-tf2-tools` package (it contains `view_frames`. `sudo apt install ros-jazzy-tf2-tools`)
- A PDF viewer (to see the `frames.pdf` that `view_frames` outputs. A GUI environment is assumed)

### Suggested time plan

- Explain why coordinate transformation is needed: 10 min
- Explain the roles of map/odom/base_link: 10 min
- Static TF and `static_transform_publisher` exercise: 10 min
- Run the turtle_tf2 demo and check with `tf2_echo`/`view_frames`: 15 min
- `lookup_transform` and time, and oral questions: 10 min

### Oral questions

**Q1. Why must you not use the coordinates of an obstacle detected by LiDAR directly to decide whether the robot moves forward?**

Model answer: The coordinates that LiDAR returns are values in a frame whose origin is the LiDAR itself, such as `laser` (or `lidar_link`). Its reference point and direction differ from the robot's body and from the frame of the whole space where the robot moves. For the robot to make a decision such as "move forward 0.5 m", it must first convert the obstacle position into a frame based on the robot or the environment, such as `base_link` or `map`. If you use the raw LiDAR coordinates without this conversion, the decision is off by the amount that the mounting position and direction of the LiDAR are off.

**Q2. The `odom` frame and the `map` frame both express "where the robot is now". What is the difference?**

Model answer: `odom` is a frame made only from the robot's own estimate, such as wheel odometry and IMU integration. Its value is guaranteed to change continuously and smoothly (it does not jump), but errors pile up over time, and it drifts away from the real position. `map` is a frame that expresses the "correct" position with respect to the whole environment, obtained by SLAM or localization. Its value may jump at the moment the position estimate is updated, for example by a loop closure, but in the long term it is more accurate than `odom`. The roles are divided like this: control that needs short-term smoothness (local driving control) uses `odom` as its reference, and processing that needs long-term correctness (global path planning) uses `map` as its reference.

**Q3. When I called `lookup_transform`, I got the error `ExtrapolationException: Lookup would require extrapolation into the future`. What is happening, and how do you deal with it?**

Model answer: The TF for the specified time has not yet reached the TF tree. In other words, you are asking it to "compute the transform for a future time that has not arrived yet". TF2 basically keeps the history of transforms from the past to the present in a buffer and interpolates between them, so it fails if you specify a time that has not been published yet, or conversely a time that is so old that it has already left the buffer. There are several ways to deal with it: use `rclpy.time.Time()` (no time specified, meaning "the latest available transform"), wait until the transform arrives with something like `wait_for_transform_async` instead of `lookup_transform`, or do not use the message's `header.stamp` as it is and move it closer to the current time.

## Main text

### What you learn: Why coordinate transformation is needed

What you learn: understand that several coordinate frames exist at the same time inside a robot.

Preparation: nothing in particular.

Details:

The coordinates where LiDAR says "there is an obstacle here" are values in a frame whose origin is the sensor, the LiDAR itself. If the mounting position of the sensor is off from the center of the robot, the "origin" that the LiDAR sees is off from the center of the robot by the same amount. For the robot to decide to "go forward while avoiding the obstacle", you must convert these coordinates into a frame based on the robot's body (`base_link`) or a frame based on the whole space where it moves around (`map`).

With only one LiDAR as a sensor, you could still manage by hand calculation. But a real robot carries cameras, an IMU, several wheel encoders, and so on, and each of them returns data in its own frame. If you converted all of this by hand, it would break down, so ROS 2 has a mechanism that manages in one place "which frame is in what relation to which frame". This is TF2 (the second generation of the Transform Framework).

TF2 manages the translation and rotation between frames as a tree structure. When you make a request such as "I want to convert a point in the `laser` frame to the `map` frame", it follows the tree and returns the computed result. You do not need to write each transformation yourself. You only need to ask TF2, "tell me the current transform from `laser` to `map`".

### What you learn: The standard frame setup (REP 105)

What you learn: understand the standard frame setup `map` → `odom` → `base_link`, and the role of each frame.

Preparation: nothing in particular.

Details:

ROS 2 has a standard convention for the naming and the parent-child relations of frames, defined in REP 105 (REP: ROS Enhancement Proposal). Many navigation stacks follow this convention, so even for a robot you see for the first time, you can read the rough structure by looking at the TF tree. The three central frames are as follows.

| Frame | Meaning | Characteristics |
|---|---|---|
| `map` | A fixed frame based on the whole environment | Accurate in the long term. But its value may jump when SLAM/localization updates |
| `odom` | A frame whose origin is the robot's position at start-up | Its value is continuous and smooth (it does not jump). Over time it drifts and moves away from the real position |
| `base_link` | A frame based on the robot's body itself | Fixed to the robot's center or center of gravity. The frames of sensors (such as `laser`) hang below it |

The parent-child order is `map` → `odom` → `base_link`. That is, `base_link` is expressed as a position relative to `odom`, and `odom` is expressed as a position relative to `map`.

The key point of this setup is why `odom` is placed between `map` and `base_link`. For local driving control (avoiding an obstacle right ahead, keeping a straight line), it is important that "the value is smooth and does not jump suddenly", and some drift is not a practical problem for a short time. On the other hand, for global path planning (which room you are in now, and which corridor you take to the goal), it is important that the value is "correct in the long term", and it does not matter if the value jumps a little. `odom` takes the former role and `map` takes the latter, so you can use the frame suited to each purpose.

> Column: Localization is the job of continuously computing how far `odom` is off as seen from `map` (the `map` → `odom` transform). `odom` → `base_link` can be computed directly from wheel odometry and so on, but `map` → `odom` can only be obtained by matching the map of the environment with the current sensor values. We cover this overview in detail in [22_thinking_about_localization](22_thinking_about_localization.md).

### What you learn: Publishing a static TF

What you learn: publish one fixed coordinate transformation with `static_transform_publisher`.

Preparation: prepare one terminal that does not use turtlesim.

Details:

A transformation that does not change while the robot moves, such as the position relation between the robot's body and the LiDAR, is handled as a "static TF". A static TF only needs to be published once, and if you use `static_transform_publisher` of the `tf2_ros` package, you can publish it from the CLI without writing a single line of code.

```bash
ros2 run tf2_ros static_transform_publisher --frame-id base_link --child-frame-id laser --x 0.1 --y 0.0 --z 0.2 --roll 0 --pitch 0 --yaw 0
```

This keeps publishing the transformation "as seen from `base_link`, `laser` is at a position 10 cm away in the x direction and 20 cm away in the z direction (no rotation)". `--frame-id` is the parent frame and `--child-frame-id` is the child frame. On a real robot, you decide these numbers by measuring with a tape measure where and at what direction you mounted the LiDAR on the body, or by reading them from the CAD drawing.

### What you learn: Running the turtle_tf2 demo

What you learn: run the official TF2 demo of turtlesim and observe a real example of dynamic TF.

Preparation: the `ros-jazzy-turtle-tf2-py` package must be installed.

Details:

`turtle_tf2_py` is the official TF2 demo that runs on turtlesim. Start it first.

```bash
ros2 launch turtle_tf2_py turtle_tf2_demo.launch.py
```

Two turtles (`turtle1` and `turtle2`) are shown. From another terminal, move `turtle1` with key input.

```bash
ros2 run turtlesim turtle_teleop_key
```

When you move `turtle1`, `turtle2` follows `turtle1` while keeping a certain distance. This is because the demo uses TF2 to compute "the position of `turtle2` relative to `turtle1`", and converts the result into a velocity command for `turtle2`. In other words, the frame of each turtle (`turtle1`, `turtle2`) and their parent frame `world` are managed as a TF tree, and the node that controls `turtle2` asks with `lookup_transform` at every cycle, "where is `turtle2` now, as seen from `turtle1`?".

This demo is a real example of dynamic TF. The frames of `turtle1` and `turtle2` keep being updated every time a turtle moves, so you cannot express them with a one-time publish like `static_transform_publisher`. For dynamic TF, you use `TransformBroadcaster` (a library API for Python/C++) and broadcast every time the position changes inside the node.

### What you learn: Checking with tf2_echo and view_frames

What you learn: check the state of the TF tree with `tf2_echo` and `view_frames`.

Preparation: keep the turtle_tf2 demo running.

Details:

When you want to know the transformation between two frames right now, `tf2_echo` is handy.

```bash
ros2 run tf2_ros tf2_echo turtle1 turtle2
```

The position and orientation of `turtle2` as seen from `turtle1` keep being shown at a fixed period. Move `turtle1` with `teleop`, and check that the numbers change.

When you want to see the whole TF tree together, use `view_frames`.

```bash
ros2 run tf2_tools view_frames
```

When you run it, a file named `frames.pdf` is created in the current directory. If you open it, you can see a tree diagram with `world` as the root and `turtle1` and `turtle2` hanging below it. It also shows when the transformation between each pair of frames was last published, and at what period it is published. If you open this PDF on a real robot, you should see a tree that branches from `map` → `odom` → `base_link` out to `laser` and `camera_link`. When you have the trouble "I feel that TF is not connected", get into the habit of producing this one page first. If a frame is isolated, or the parent-child relation differs from what you expected, that is the cause.

### What you learn: lookup_transform and time

What you learn: understand why `lookup_transform` takes a time, and how `ExtrapolationException` happens.

Preparation: nothing in particular.

Details:

TF2 transformations are managed not only for "now" but "at a given time". When you call `lookup_transform`, you pass a time in addition to the names of the source and target frames.

```python
transform = tf_buffer.lookup_transform(
    'map',            # Target frame
    'laser',          # Source frame
    rclpy.time.Time(),  # Time (Time() means "the latest available transform")
)
```

The reason a time is needed is that TF keeps the history of transformations from the past to the present in a buffer, and returns the transformation closest to the specified time by interpolating. A sensor message has the time when the sensor value was acquired in `header.stamp`. So when you want to convert "with the TF at the moment that sensor value was acquired", it is accurate to pass `header.stamp` as it is.

A common place to get stuck here is `ExtrapolationException`. If the specified time is a future time that has not reached the buffer yet, or conversely a time older than the retention period of the buffer (a few seconds by default), TF2 decides that "it cannot interpolate; it would need extrapolation" and throws this exception. It tends to happen when a sensor message arrives a little late because of network or processing delay, and you try to convert with its timestamp. First, pass `rclpy.time.Time()` to change the request to "the latest transform is fine". If that solves the problem, you can conclude that the delay was the cause. If you strictly need the transformation "at the moment of that timestamp", implement it to wait until the transformation arrives, with something like `wait_for_transform_async`.

## Going further

Localization is, in the end, "someone keeps publishing the `map` → `odom` transform". `odom` → `base_link` can be computed directly from wheel odometry and so on, but `map` → `odom` can only be obtained by matching the map of the environment with the sensor values. The real "someone" is an AMCL or SLAM node, and when they are not running, the `map` frame does not exist, and `odom` is the only frame you can rely on. We cover this overview concretely in [22_thinking_about_localization](22_thinking_about_localization.md).


## Conclusion

TF2 is one of the most behind-the-scenes mechanisms in robot software, but almost every function, such as navigation, sensor integration, and arm control, stands on top of TF2. If you get into the habit here of checking "which frames are connected how right now" with `view_frames`, debugging in later articles will be faster. If anything is unclear, ask someone experienced or check the official documentation.

Next, in [19_writing_urdf](19_writing_urdf.md), we cover how to describe the shape and frame structure of a robot in XML.

## References

- [ROS 2 Documentation: Jazzy — Introducing tf2](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Tf2/Introduction-To-Tf2.html)
- [ROS 2 Documentation: Jazzy — Writing a static broadcaster](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Static-Broadcaster-Py.html)
- [ROS 2 Documentation: Jazzy — Writing a tf2 broadcaster](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Broadcaster-Py.html)
- [ROS 2 Documentation: Jazzy — Writing a tf2 listener](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Tf2/Writing-A-Tf2-Listener-Py.html)
- [REP 105 -- Coordinate Frames for Mobile Platforms](https://www.ros.org/reps/rep-0105.html)
- [17_testing_and_debugging](17_testing_and_debugging.md)
- [19_writing_urdf](19_writing_urdf.md)
