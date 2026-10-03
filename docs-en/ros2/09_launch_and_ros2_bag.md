# ROS 2 Lecture 09: launch and ros2 bag

## Introduction

In the lectures so far, you started nodes one by one with `ros2 run`. From now on, we stop doing that. This lecture covers `launch`, which starts many nodes together, and `ros2 bag`, which records and replays all the communication. After this lecture, you can start many nodes with one command, record the communication while it runs, and replay it later. This is the last article of Part 1.

## Lecture goals / how to run the lecture

- Audience: new students who have finished [08_actions](08_actions.md)
- Time: 55 to 70 minutes
- Prerequisites: starting nodes with `ros2 run`, and the basics of topics and cmd_vel (covered in 03 and 05)
- Goal: You can start many nodes with a Python launch file. You can record topics with ros2 bag, and replay them to reproduce the same motion. You can also build bag recording into a launch file, so that "if you start it, it is always recorded"

## Using this as a lecture

### What to prepare

- A terminal with Ubuntu 24.04 + ROS 2 Jazzy set up
- `ros-jazzy-turtlesim` (you should already have it from 03)
- The `ros-jazzy-rosbag2*` packages (normally included in the desktop install. Check beforehand that `ros2 bag --help` works)
- A screen where you can open three or more terminals side by side

### Oral quiz

Q1. Compared with running `ros2 run` in each terminal without a launch file, what is the advantage of a launch file?

<details markdown="1"><summary>Model answer</summary>
You can start many nodes with one command, and you can describe topic remapping and parameter injection in one file. A real robot usually has more than 10 nodes, so opening 10 terminals and starting them by hand is not realistic. It also reduces missed starts and mistakes in the start order.
</details>

Q2. Explain the difference between `ros2 bag record -a` and recording with topics specified, and when to use each.

<details markdown="1"><summary>Model answer</summary>
`-a` records all running topics. For analysis after a test run where you do not know what caused the problem, `-a` is safe (if the topic you want to see later was not recorded, you are stuck). On the other hand, if you know the target topics from the start, or if there are large topics such as images and point clouds, recording with the topic names specified keeps the file size and the later analysis effort small.
</details>

Q3. Does the turtle move differently between the cmd_vel replayed by `ros2 bag play` and the cmd_vel that was actually sent from teleop?

<details markdown="1"><summary>Model answer</summary>
Basically, the same motion is reproduced. A bag records the messages that flowed on a topic together with their timestamps, and on playback it sends the messages at the same intervals as when they were recorded. turtlesim moves by looking only at the value of cmd_vel, so it does not distinguish whether the sender is teleop or bag playback. However, if some processing depends on a topic that was not recorded (such as the internal state of another node), that part is not reproduced.
</details>

### Suggested time plan

| Item | Time |
|---|---|
| Explaining why launch is needed | 5 minutes |
| Writing and running the turtlesim + mimic launch file | 15 minutes |
| ros2 bag record/info/play | 15 minutes |
| cmd_vel record and replay exercise | 10 minutes |
| Building bag recording into launch (Exercise 5) | 15 minutes |
| Oral quiz | 5 to 10 minutes |

## Main text

### Exercise 1: Why do we need launch?

What you learn: When the number of nodes grows, starting them one by one with `ros2 run` breaks down. launch puts this into one file.

Preparation: None.

Content: In the lectures so far, for turtlesim you started `turtlesim_node` and `turtle_teleop_key` in separate terminals. Two is still possible by hand. But a real robot usually has more than 10 nodes when started on the real machine: a motor control node, sensor drivers, a state estimation node, a path planning node, a path following node, a state management node, and so on. Opening 10 terminals and starting them one by one in the right order breeds missed starts, wrong start orders, and typos.

launch is a mechanism where you describe the list of nodes to start, the remappings, and the parameters in a Python file (or XML/YAML), and start them all at once with `ros2 launch`. This time we use the Python format. The official ROS 2 launch API is complete in Python, so it is easy to build complex start logic such as conditions and variable expansion.

Hint: A launch file is "a configuration file about how to start nodes". It is not a place to write the control logic of the robot. The logic goes inside each node.

### Exercise 2: Start two turtlesims and a mimic node with launch

What you learn: Write the smallest Python launch file and start many nodes with one command.

Preparation: You have not created a workspace yet, so any working directory is fine (making a proper package is covered from [10_workspaces_and_colcon](10_workspaces_and_colcon.md) on).

Content: Create a working directory and put the launch file there.

```bash
mkdir -p ~/ros2_lecture/launch
cd ~/ros2_lecture/launch
```

Create `mimic_launch.py` with the following content. This is the example from the official ROS 2 tutorial itself. It starts two turtlesims and connects a `mimic` node, which makes one turtle (turtlesim2) copy the motion of the other (turtlesim1).

```python
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='turtlesim',
            namespace='turtlesim1',
            executable='turtlesim_node',
            name='sim',
        ),
        Node(
            package='turtlesim',
            namespace='turtlesim2',
            executable='turtlesim_node',
            name='sim',
        ),
        Node(
            package='turtlesim',
            executable='mimic',
            name='mimic',
            remappings=[
                ('/input/pose', '/turtlesim1/turtle1/pose'),
                ('/output/cmd_vel', '/turtlesim2/turtle1/cmd_vel'),
            ],
        ),
    ])
```

The function name `generate_launch_description()` is fixed. `ros2 launch` calls this function and receives the list of nodes (`LaunchDescription`). One `Node` corresponds to the start settings of one node. By separating names with `namespace`, the names do not collide even if you start two of the same `turtlesim_node` at once. `remappings` rename topics. Here, they connect the `/input/pose` and `/output/cmd_vel` that the mimic node uses by default to the topic names that actually exist on the turtlesim1 and turtlesim2 sides.

Run it.

```bash
ros2 launch ~/ros2_lecture/launch/mimic_launch.py
```

Two turtlesim windows open. In another terminal, connect teleop to the turtlesim1 side.

```bash
ros2 run turtlesim turtle_teleop_key --ros-args --remap __ns:=/turtlesim1
```

When you move one turtle (the turtlesim1 side) with the arrow keys, the other turtle (the turtlesim2 side) makes the same motion. What teleop sends is `/turtlesim1/turtle1/cmd_vel`, but the mimic node reads `/turtlesim1/turtle1/pose`, converts it, and sends it to `/turtlesim2/turtle1/cmd_vel`, so the motion is passed on. Check that three nodes started with one command (`ros2 launch`).

Hint: If you run `ros2 node list`, you can see the three nodes `/turtlesim1/sim`, `/turtlesim2/sim`, and `/mimic`. Check that the namespace comes before the node name.

### Exercise 3: Record with ros2 bag

What you learn: `ros2 bag record` records the messages of the given topics (or all topics) into a file.

Preparation: Keep turtlesim1, turtlesim2, and mimic from Exercise 2 running (you can stop teleop).

Content: Move to the directory for recordings and start recording. First, the way to record all topics.

```bash
mkdir -p ~/ros2_lecture/bags
cd ~/ros2_lecture/bags
ros2 bag record -a -o all_topics_bag
```

`-a` means "target all running topics", and `-o` gives the name of the output directory. Stop recording with Ctrl+C.

Note that **the default recording format is `mcap`** (`ros2 bag record --help` shows `-s {mcap,sqlite3} ... defaults to 'mcap'`). Many materials for Humble and earlier, and many web articles, assume that a `.db3` (sqlite3) file is created, so do not panic if the extension of the output file is different. If you really want to keep sqlite3, add `-s sqlite3`.

To record with topics specified, do the following.
```bash
ros2 bag record -o cmd_vel_only_bag --topics /turtlesim1/turtle1/cmd_vel
```

Check the contents of the recorded bag.

```bash
ros2 bag info cmd_vel_only_bag
```

It shows the recording duration, the number of messages, and the list of recorded topic names and types.

Hint: If you record with `-a`, the file size grows fast in an environment with images or point clouds. If you know what to record, specifying topics is safe. Right after a test run where you do not know what happened, you should use `-a` to avoid the case where the topic you want to see later was not recorded.

### Exercise 4: Record cmd_vel and replay it

What you learn: If you replay the recorded cmd_vel with `ros2 bag play`, the same motion as at recording time is reproduced.

Preparation: Keep turtlesim1, turtlesim2, and mimic running. Also start teleop so that you can control it.

Content: Record only cmd_vel.

```bash
cd ~/ros2_lecture/bags
ros2 bag record -o teleop_cmd_vel --topics /turtlesim1/turtle1/cmd_vel
```

After recording starts, move the turtle freely with teleop in another terminal for about 10 seconds. Press the up, down, left, and right keys a few times, and check that the turtle moved. When you finish moving it, press Ctrl+C in the recording terminal to stop recording.

To reset the position of the turtle, restart turtlesim or call the `/reset` service.

```bash
ros2 service call /reset std_srvs/srv/Empty
```

Stop teleop (Ctrl+C), and replay the recorded bag.

```bash
ros2 bag play teleop_cmd_vel
```

The turtle moves along the same path as when you controlled it with teleop. Even if you are not clicking on the teleop window, you can check with `ros2 topic echo /turtlesim1/turtle1/cmd_vel` that messages keep flowing on cmd_vel.

As an exercise, check the following by yourself.

1. While `ros2 bag play` is running, run `ros2 topic hz /turtlesim1/turtle1/cmd_vel` and check that it is replayed at about the same publish rate as at recording time
2. Try replaying at double speed, like `ros2 bag play -r 2.0 teleop_cmd_vel`, and check that the turtle moves faster

<details markdown="1"><summary>Answer</summary>
1. teleop sends messages only when a key is pressed, so `hz` depends on how often you pressed keys. If a value close to the key frequency at recording time also appears on playback, it means the replay follows the timestamps.
2. The `-r` (rate) option is the playback speed multiplier. With 2.0, the time interval between recorded messages becomes half, and the turtle traces the same path in less time. The shape of the motion (the path) does not change.
</details>

Hint: A bag only records the raw messages that flowed on topics and their timestamps. On playback, bag play does not restore the recorded node (teleop in this case). It only sends the messages to the topics again as they were recorded. If you understand this, it is easier later to judge what a bag can reproduce and what it cannot.

### Exercise 5: Build bag recording into launch

What you learn: Start bag recording from a launch file with `ExecuteProcess`, so that "if you start it, recording always runs".

Preparation: `mimic_launch.py` from Exercise 2, and the `ros2 bag` commands from Exercises 3 and 4, both work.

Content:

Until now, you started nodes with launch and typed `ros2 bag record` **by hand in another terminal**. This way of working has a structural problem.

**You forget to record.**

Right before a test run, you are busy, and "start it, then run it" is all you can do. Then, after a problem happens, you notice "I did not record a bag". The Wrapping up section says "always run a bag in test runs", but **a countermeasure that relies on human discipline fails.** If you build it into launch, it is always recorded when you start.

`ros2 bag record` is a command, not a node, so you use `ExecuteProcess` instead of `Node`. Add a fourth element to `mimic_launch.py` from Exercise 2. Create it with the file name `mimic_record_launch.py`.

```python
from launch import LaunchDescription
from launch.actions import ExecuteProcess
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='turtlesim',
            namespace='turtlesim1',
            executable='turtlesim_node',
            name='sim',
        ),
        Node(
            package='turtlesim',
            namespace='turtlesim2',
            executable='turtlesim_node',
            name='sim',
        ),
        Node(
            package='turtlesim',
            executable='mimic',
            name='mimic',
            remappings=[
                ('/input/pose', '/turtlesim1/turtle1/pose'),
                ('/output/cmd_vel', '/turtlesim2/turtle1/cmd_vel'),
            ],
        ),
        ExecuteProcess(
            cmd=[
                'ros2', 'bag', 'record',
                '-o', 'mimic_bag',
                '--topics',
                '/turtlesim1/turtle1/cmd_vel',
                '/turtlesim1/turtle1/pose',
            ],
            output='screen',
        ),
    ])
```

The difference between `Node` and `ExecuteProcess` is simple. `Node` is an action made for starting a ROS node by giving "a package name and an executable name". `ExecuteProcess` is a general action that starts "any command" given as a `cmd` list. `ros2 bag record` is a CLI command, not a ROS node, so we use the latter.

Run it.

```bash
cd ~/ros2_lecture/bags
ros2 launch ~/ros2_lecture/launch/mimic_record_launch.py
```

Lines from the recorder are mixed into the log. `[ros2-4]` means the output of the fourth element (`ExecuteProcess`).

<!-- measure: env=static reason="needs turtlesim_node running, and the Docker image has no turtlesim" -->
```
[ros2-4] [INFO] [rosbag2_recorder]: Starting recording to 'mimic_bag'
[ros2-4] [INFO] [rosbag2_recorder]: Listening for topics...
[ros2-4] [INFO] [rosbag2_recorder]: Recording...
[ros2-4] [INFO] [rosbag2_recorder]: Subscribed to topic '/turtlesim1/turtle1/pose'
```

Leave it for about 10 seconds, and then press **Ctrl+C once** in the launch terminal. The recorder also shuts down properly.

<!-- measure: env=static reason="needs turtlesim_node running, and the Docker image has no turtlesim" -->
```
[ros2-4] [INFO] [rosbag2_recorder]: Pausing recording.
[mimic-3] [INFO] [rclcpp]: signal_handler(SIGINT/SIGTERM)
[turtlesim_node-2] [INFO] [rclcpp]: signal_handler(SIGINT/SIGTERM)
[turtlesim_node-1] [INFO] [rclcpp]: signal_handler(SIGINT/SIGTERM)
[ros2-4] [INFO] [rosbag2_recorder]: Recording stopped
[ros2-4] [INFO] [rosbag2_recorder]: Event publisher thread: Exited
```

**Ctrl+C is passed from launch to all processes, and the bag is closed correctly.** Check it.

```bash
ros2 bag info mimic_bag
```

<!-- measure: env=static reason="needs turtlesim_node running, and the Docker image has no turtlesim" -->
```
Files:             mimic_bag_0.mcap
Bag size:          43.7 KiB
Duration:          8.992216267s
Messages:          563
Topic information: Topic: /turtlesim1/turtle1/pose | Type: turtlesim/msg/Pose | Count: 563
```

563 messages of `/turtlesim1/turtle1/pose` were recorded. **`/turtlesim1/turtle1/cmd_vel` does not appear, even though we set it as a recording target.** This is because we did not start teleop, so not a single message flowed on that topic.

This behavior is worth remembering. **A topic that does not appear in `ros2 bag info` may not have "failed to record" but may simply "not have been flowing in the first place".** Before you panic with "there is no cmd_vel" when you look at the bag after a test run, suspect whether the publisher side was running.

As an exercise, check the following by yourself.

1. Run the same launch with teleop running, and check that `cmd_vel` is also recorded
2. Run it a second time without changing `-o mimic_bag`, and check what happens

<details markdown="1"><summary>Answer</summary>

1. In the Topic information of `ros2 bag info`, a line `/turtlesim1/turtle1/cmd_vel | Type: geometry_msgs/msg/Twist` is added. The Count increases by the number of key presses in teleop, so unlike `pose` (which flows at a fixed rate), the Count is small.

2. Recording does not start, because of an error.

<!-- measure: env=static reason="needs turtlesim_node running, and the Docker image has no turtlesim" -->
```
[ERROR] [ros2bag]: Output folder 'mimic_bag' already exists.
```

**If an output folder with the same name already exists, a bag fails instead of overwriting it.** This is correct behavior as an accident prevention, but once you build it into launch, "the second start may run quietly without recording". In real operation, put a timestamp in the output name.

```python
from launch.substitutions import LocalSubstitution
# Or build the name with datetime on the Python side
```

The straightforward way is to build the name as Python inside the launch file. `generate_launch_description()` is an ordinary Python function, so this works as it is.

```python
import datetime

stamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
# ... cmd=['ros2', 'bag', 'record', '-o', f'run_{stamp}', '--topics', ...]
```

</details>

Hint: Here are three points where you can get stuck in real operation.

**The output path is relative to the directory where you ran `ros2 launch`.** It is not the place where the launch file is. If you write a relative path such as `-o mimic_bag`, the place where the bag is created changes with where you typed `ros2 launch`. If you are unsure, use an absolute path.

**Add `--topics`.** The style that lists topic names as positional arguments (`ros2 bag record -o foo /topic_a`) also works, but depending on the version it shows a deprecation warning.

<!-- only: jazzy -->
In Jazzy, the following warning appears.
<!-- /only -->
<!-- only: lyrical -->
In this version no warning appears, so the following output is empty.
<!-- /only -->

<!-- measure: env=ros cmd="timeout -s INT 3 ros2 bag record -o /tmp/positional_bag /chatter" filter="grep WARN" -->
```
[WARN] [ros2bag]: Positional "topics" argument deprecated. Please use optional "--topics" argument instead.
```

**You cannot pause with SPACE when you go through launch.** The terminal is not connected, so it shows `stdin is not a terminal device. Keyboard handling disabled.`. If you want fine control over the start and stop of recording, typing it by hand is better. Building it into launch is for the case "record everything from start to end".

## Going further

The launch files this time were the smallest form, with only the remappings written directly. In real projects, you use `launch.substitutions` and `DeclareLaunchArgument` to pass arguments at start (for example, using the same launch file for the simulator and for the real machine), and you branch with `IfCondition`. This is covered in [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md).

For bags too, this time we did only recording and simple playback, but `ros2 bag play` can also exclude specific topics from playback or seek to a given time. See the official documentation for details.

### Switching recording on and off with a launch argument

In Exercise 5 we built in recording without any condition. In real operation, you want to switch it: "record only when debugging" or "turn it off when disk space is a concern". You can do this by combining `DeclareLaunchArgument` and `IfCondition`.

```python
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration

    DeclareLaunchArgument('record', default_value='true'),
    ExecuteProcess(
        condition=IfCondition(LaunchConfiguration('record')),
        cmd=['ros2', 'bag', 'record', '-a', '-o', 'run'],
        output='screen',
    ),
```

Switch it at start.

```bash
ros2 launch mimic_record_launch.py record:=false
```

**The key point is to make the default value `true`.** If you make it "add `record:=true` when you want to record", then you will forget to add it and have an accident where nothing was recorded. If you make it "add `record:=false` explicitly only when disk space is tight", then when you forget, it falls to the safe side (it is recorded). This idea is covered in detail in [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md).

### Starting it as a node instead of ExecuteProcess

`ros2 bag record` is a CLI command, but what is inside rosbag2 is a ROS node. It is also published as components.

```bash
ros2 component types | grep rosbag
```

<!-- measure: env=ros -->
```
rosbag2_transport
  rosbag2_transport::Player
  rosbag2_transport::Recorder
```

If you put `rosbag2_transport::Recorder` into a component container, you can run the recording in the same process as other nodes. You can then do optimizations such as putting it in the same process as a sensor driver and recording images **without copying** (intra-process communication).

However, all settings are passed as parameters, and the convenience of `-a` and `--topics` is lost. **At first, `ExecuteProcess` is enough.** Remember this when recording large topics becomes a performance problem.

## Wrapping up

With this, Part 1 (ROS 2 basics) is complete. You have touched the concepts that are the foundation for using ROS 2: nodes, topics, services, parameters, actions, launch, and bags.

Let me stress one point about robot competitions. After a test run, a situation like "something was wrong in that run, but I do not know the cause" will surely happen. If the bag was kept, you can replay it later with `ros2 bag play` and analyze it offline. On the other hand, for a test run that ended without keeping a bag, there is no way to reproduce the problem when it happens.

**And this is not solved by "being careful".** As you did in Exercise 5, build the recording into the bringup launch with `ExecuteProcess`. If you make it so that starting always records, you do not need to remember on the day of the test run.

Next we enter Part 2, package development. So far you only combined existing nodes, but from now on you write nodes yourself. Let's start with how to make a workspace in [10_workspaces_and_colcon](10_workspaces_and_colcon.md). If something is unclear, ask someone experienced or check the official documentation.

### Matching exercise

After you read this chapter, practice with the matching drill.

- `09_launch` — Write launch in Python / XML / YAML

```bash
./drill run 09
```

From the exercise, you can return to this chapter with `./drill read`.

## References

- Matching official tutorial: [Creating a launch file (ROS 2 Jazzy)](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/Launch/Creating-Launch-Files.html)
- [Recording and playing back data (ROS 2 Jazzy)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Recording-And-Playing-Back-Data/Recording-And-Playing-Back-Data.html)
- Previous: [08_actions](08_actions.md)
- Next (Part 2): [10_workspaces_and_colcon](10_workspaces_and_colcon.md)
