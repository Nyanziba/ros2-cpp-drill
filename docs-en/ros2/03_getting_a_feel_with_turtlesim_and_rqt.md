# ROS 2 Lecture 03: Getting a Feel with turtlesim and rqt

## Introduction

Last time, in [02_environment_setup](02_environment_setup.md), you built an environment where ROS 2 Jazzy runs. This time, you understand the ROS 2 concepts of nodes and topics by seeing them, without writing a single line of code. When you finish this lecture, you can move a turtle and check "what is happening now" in rqt.

## Lecture goals / how to proceed

- Audience: people learning this for the first time who have finished [02_environment_setup](02_environment_setup.md)
- Time needed: 30 to 40 minutes
- Prerequisites: basic terminal use (you can proceed by copy and paste)
- Goal: operate turtlesim and check how nodes and topics are connected in rqt_graph

## Using this as a course

### Things to prepare

- A machine with Ubuntu 24.04 + ROS 2 Jazzy Jalisco already set up (each person's laptop is fine)
- A screen large enough to open two or more terminals side by side (an external monitor helps)
- Whether you can install the `ros-jazzy-rqt*` packages depends on network speed, so if each machine installs them beforehand, they will not eat into the time on the day

### Oral exam

Q1. After you move the turtle with `ros2 topic pub`, what happens to the turtle if you press Ctrl+C in that terminal?

<details markdown="1"><summary>Model answer</summary>
pub does not send a message only once. By default, it keeps publishing repeatedly at a fixed rate (1 Hz). When you stop publishing with Ctrl+C, the velocity commands to the turtle stop coming, so the turtle stops on the spot. turtlesim is not implemented to keep moving by inertia.
</details>

Q2. The turtle does not react when you press keys in teleop_key. What should you check first?

<details markdown="1"><summary>Model answer</summary>
Check whether the terminal window where you started teleop_key has focus (is clicked and active). teleop reads keyboard events directly in that terminal process, so if another window (such as rqt or a browser) stays active, the input does not arrive.
</details>

Q3. What do the arrows shown in rqt_graph represent? Do the nodes call each other's functions directly?

<details markdown="1"><summary>Model answer</summary>
The arrows represent topic communication. Nodes do not call each other directly. They only publish/subscribe to topics via DDS (Data Distribution Service). Nodes do not need to know that the others exist. Communication works if the topic name and type match. This loosely coupled structure is at the center of the ROS 2 design.
</details>

### Time allocation guide

| Item | Time |
|---|---|
| Installing and starting turtlesim | 5 min |
| Operating with teleop | 5 min |
| Observing rqt and rqt_graph | 10 min |
| Looking inside cmd_vel | 10 min |
| Spawning with Service Caller | 5 min |
| Task and oral exam | 5 to 10 min |

## Main content

### Task 1: Install and start turtlesim

What you learn: ROS 2 packages are distributed through apt, and you can run them with `ros2 run`.

Preparation: The ROS 2 Jazzy apt repository has been set up in [02_environment_setup](02_environment_setup.md).

Content:

```bash
sudo apt update
sudo apt install ros-jazzy-turtlesim
```

When the installation is done, start the node.

```bash
ros2 run turtlesim turtlesim_node
```

A window opens with a turtle in it (a simple arrow-like shape, not a picture of a real turtle). This window is drawn by a node called `turtlesim_node`. Node names and topic names flow in the terminal as logs. This terminal is now occupied, so open a new terminal for the next operations.

Hint: The basic form is `ros2 run <package name> <executable name>`. The package name and the executable name are different things, and one package can have several executables.

### Task 2: Operate the turtle with teleop_key

What you learn: teleop is a node that turns keyboard input into topic messages and sends them.

Preparation: Keep the turtlesim_node from Task 1 running.

Content: Open a new terminal and run the following.

```bash
ros2 run turtlesim turtle_teleop_key
```

The turtle moves when you press the arrow keys. If it does not move, click the terminal window where you started teleop_key to give it focus, and try again. **teleop cannot receive key input unless the terminal window itself has focus.** Clicking the turtlesim drawing window does not work either.

When you stop it with Ctrl+C, the turtle stops receiving commands from that moment and stops.

Hint: Besides the arrow keys, you can change the rotation speed with `q`/`e` and the translation speed with `Q`/`E` (a list appears in the terminal output when teleop_key starts).

### Task 3: See how nodes connect with rqt and rqt_graph

What you learn: rqt_graph is a tool that shows the relations between nodes and topics in a ROS 2 system as a graph.

Preparation: Keep both turtlesim_node and turtle_teleop_key running.

Content: In a third terminal, run the following.

```bash
sudo apt install ros-jazzy-rqt ros-jazzy-rqt-graph ros-jazzy-rqt-common-plugins
rqt_graph
```

A window opens, and the relations between nodes and topics are drawn with arrows. You should see an arrow through the topic `/turtle1/cmd_vel` between the `/turtlesim` node and the `/teleop_turtle` node. The arrow goes one way, from `teleop_turtle` to `turtlesim`. It shows that teleop sends and turtlesim receives.

You can check the same content from the command line too.

```bash
ros2 node list
ros2 node info /turtlesim
ros2 topic list
```

When you type `ros2 node info /turtlesim`, you get a list of which topics that node subscribes to and publishes, and which services it provides. It is quicker to understand if you think of rqt_graph as just a picture of this text information.

Hint: If you do not press the refresh button at the top left of rqt_graph (the rotating arrow icon), newly started nodes may not appear.

### Task 4: See what /turtle1/cmd_vel really is

What you learn: Check the type and content of the topic that teleop sends. This has exactly the same structure as the `cmd_vel` you send to a real robot in later lectures.

Preparation: Keep turtlesim_node and turtle_teleop_key running.

Content: Check the type of the topic.

```bash
ros2 topic info /turtle1/cmd_vel
```

It shows the type `geometry_msgs/msg/Twist`. Let us look at the structure of this type.

```bash
ros2 interface show geometry_msgs/msg/Twist
```

The output looks like this.

```
Vector3  linear
        float64 x
        float64 y
        float64 z
Vector3  angular
        float64 x
        float64 y
        float64 z
```

It is a simple structure that expresses translational velocity (linear) and angular velocity (angular), each with three axes x, y, z. When you press up or down in teleop_key, linear.x changes, and when you press left or right, angular.z changes.

Let us peek at the values that actually flow.

```bash
ros2 topic echo /turtle1/cmd_vel
```

If you operate teleop in another terminal while this command runs, the values at the moment you press keys flow in the terminal. Nothing is shown when you press nothing (because teleop sends only when something changes).

**This topic name and type have exactly the same structure as the `cmd_vel` you later send to a real robot through Nav2 or move_base_flex.** turtlesim only moves a turtle on a plane, but the form of the communication is the same as on a real competition robot. What you saw here appears many times in other articles.

Hint: `ros2 topic echo` only peeks from the side at values that other nodes send. It does not send anything itself. If nobody is sending, nothing is shown.

### Task 5: Spawn a turtle with the rqt Service Caller

What you learn: Besides topics, there is a form of communication called a service, and you can call it from rqt with the GUI.

Preparation: Keep turtlesim_node running.

Content: Start rqt.

```bash
rqt
```

From the menu, choose "Plugins" → "Services" → "Service Caller". When you choose `/spawn` from the drop-down, the request fields of `turtlesim/srv/Spawn` (x, y, theta, name) appear as input boxes. Enter suitable coordinates (for example x=3.0, y=3.0, theta=0.0) and press the "Call" button. Another turtle appears on the turtlesim screen.

When you type `ros2 service list`, you see a list of services including `/spawn`. A topic is "one-way communication that keeps sending". A service is "request-response communication where you call and receive a result once". Here you can feel that services suit operations such as spawn, where you want to run something once and check the result.

Hint: If you press "Call" with the input boxes empty, it is called with default values (such as 0.0). If you try to spawn twice with the same name, you get an error (names cannot be duplicated).

## Advanced

All the arrows you saw in rqt_graph are topic communication. This time you only "looked and checked". In the next lecture, [04_nodes](04_nodes.md), we look at the structure of a node itself, and in the article after that, we cover how to publish/subscribe to these topics from your own program. There, get the feel of the `/turtle1/cmd_vel` you saw in turtlesim flowing from code you wrote yourself.

For services too, this time you only called them from the GUI, and in later lectures you implement servers and clients yourself.

## Summary

turtlesim looks like a toy, but the concepts of nodes, topics, and services you saw here are used consistently all the way to real robot control. Do not rush. Watch how the arrows in rqt_graph behave many times. If something is unclear, ask someone experienced or check the official documentation.

## References

- Matching official tutorial: [Introducing turtlesim and rqt (ROS 2 Jazzy)](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Introducing-Turtlesim/Introducing-Turtlesim.html)
- [Understanding topics](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.html)
- [Understanding services](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Services/Understanding-ROS2-Services.html)
- Previous: [02_environment_setup](02_environment_setup.md)
- Next: [04_nodes](04_nodes.md)
