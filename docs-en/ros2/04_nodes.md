# ROS 2 Lecture 04: Nodes

## Introduction

Last time, we ran turtlesim and looked at how nodes and topics connect on the screen. This time we focus on the unit called a "node" itself. When you finish this article, you can explain to others what a node is, and you can build and use the `ros2 run` and `ros2 node` commands yourself.

## Lecture goals / how to proceed

- Time needed: about 40 minutes (assume a little under 1 hour for self-study)
- Audience: people learning this for the first time who have finished [03_getting_a_feel_with_turtlesim_and_rqt](03_getting_a_feel_with_turtlesim_and_rqt.md)
- Prerequisites: ROS 2 Jazzy is installed, and the turtlesim package works (`ros2 run turtlesim turtlesim_node` succeeds)

## Using this as a course

Things to prepare:
- A PC (one per person) with Ubuntu 24.04 + ROS 2 Jazzy installed
- The `turtlesim` package (installed with `sudo apt install ros-jazzy-turtlesim`. It may be missing in an environment with only ros-base, so check beforehand)
- An environment where you can work with two or more terminals side by side (tmux or tabs are both fine)

Oral exam (questions and model answers as sets):

1. Q. "You can put every function in one node. Why split them?"
   A. Splitting responsibilities lets you test, restart, and replace things per node. For example, you can keep the localization node running while you stop and fix only the path planning node. On the other hand, if you put everything in one node, all functions stop the moment an exception happens somewhere.
2. Q. "The node I started does not appear in `ros2 node list`. What do you check?"
   A. First, check whether that node is really still running (that it did not crash and exit). Next, check that it is not running with a different ROS_DOMAIN_ID, or on a machine across the network. Even on the same machine, if the environment variable `ROS_DOMAIN_ID` differs, the nodes cannot see each other.
3. Q. "Can the name set by `--remap __node:=xxx` differ from the name in `ros2 node list`?"
   A. Yes. remap is a command-line option that overwrites the name at node creation. If the name is fixed in code inside the package (for example `Node("fixed_name")`), it may be ignored. Get people into the habit of running it and checking with `ros2 node info`.

Time allocation guide:
- Explaining the concept of a node: 5 min
- Demo of `ros2 run` / `ros2 node list` / `ros2 node info`: 10 min
- Remapping task: 15 min
- Oral exam and discussion of splitting responsibilities: 10 min

## Main content

### Task 1: Start a node and check it

What you learn: A node is a process that keeps running with one responsibility. The steps to start a node with `ros2 run` and check that it exists with `ros2 node list`.

Preparation: Open two terminals.

Content:

A "node" in ROS 2 is one process that runs a computation. Reading images from a camera, sending command values to a motor, and estimating the robot's position are each started as independent processes and connected through topics and services. Since it is a process, from the OS's point of view it is the same as an ordinary executable. If you look with `ps aux`, it exists as a process with a PID.

In the first terminal, start the turtlesim node.

```bash
ros2 run turtlesim turtlesim_node
```

A window with the turtle opens. This terminal is occupied until this process ends (killed with Ctrl+C).

In the second terminal, look at the list of nodes that are running now.

```bash
ros2 node list
```

<!-- measure: env=static reason="turtlesim_node の起動が要り、Docker イメージに turtlesim も無い" -->
```
/turtlesim
```

You can see that one node named `/turtlesim` is running. The leading `/` shows the global namespace (we do not cover namespaces in this article. Using namespaces to tell several robots apart is covered separately as an advanced topic).

To see more detail, use `ros2 node info`.

```bash
ros2 node info /turtlesim
```

The output lists the Publishers, Subscribers, Services, and Actions of this node. You can read directly that the turtlesim node subscribes to `/turtle1/cmd_vel`, publishes `/turtle1/pose`, and provides services such as `/turtle1/teleport_absolute`. The content of the picture you saw in rqt_graph last time can be checked from the command line at the same level of detail.

Note: Give `ros2 node info` the node name as the full name with the leading `/`. If you omit the `/` and type `ros2 node info turtlesim`, you get the error `Unable to find node 'turtlesim'` and it does not work (checked on Jazzy). If you copy the output of `ros2 node list` as it is, it has the `/`.

### Task 2: Change the node name with remapping

What you learn: Node names and topic names can be overwritten at startup (remapping). The idea of starting several nodes from one package under different names.

Preparation: End the node from Task 1 once with Ctrl+C.

Content:

Let us start a node from the same `turtlesim_node` executable, with only the name changed.

```bash
ros2 run turtlesim turtlesim_node --ros-args --remap __node:=my_turtle
```

When you check with `ros2 node list`, it is now registered as `/my_turtle` instead of `/turtlesim`.

```bash
ros2 node list
```

<!-- measure: env=static reason="turtlesim_node の起動が要り、Docker イメージに turtlesim も無い" -->
```
/my_turtle
```

`--ros-args --remap __node:=new_name` is the command-line option that overwrites the node name. `__node` is a reserved name that ROS 2 treats specially. With it, you can swap only the node name without changing any source code.

This is useful when you want to start several nodes of the same package and avoid name collisions. This is easy to misunderstand, but **ROS 2 does not stop two nodes with the same name from existing.** If you actually start two nodes with the same name, both keep running normally, and they appear twice in `ros2 node list`. The only thing you get is the following warning.

<!-- measure: env=static reason="turtlesim_node を同名で 2 つ起動する必要があり、Docker イメージに turtlesim も無い" -->
```
WARNING: Be aware that there are nodes in the graph that share an exact name, which can have unintended side effects.
/turtlesim
/turtlesim
```

Because it is not an error, it is hard to notice, and resolving the communication partner becomes unstable, which leads to behavior with an unknown cause. That is exactly why you need to give different names on your side. In the lecture that moves several turtles (planned for the advanced part), you use this `--remap __node:=` with a different value for each turtle.

You can remap topic names in the same way. Let us also try remapping cmd_vel.

```bash
ros2 run turtlesim turtlesim_node --ros-args --remap __node:=my_turtle --remap /turtle1/cmd_vel:=/my_turtle/cmd_vel
```

When you check with `ros2 node info /my_turtle`, the subscribed topic name should have changed to `/my_turtle/cmd_vel`. Keep in mind that node names and topic names are managed separately.

<details markdown="1"><summary>Answer (how to check that the remap worked)</summary>

Run `ros2 node info /my_turtle`. If the line `/my_turtle/cmd_vel: geometry_msgs/msg/Twist` appears in the `Subscribers:` section of the output, the remap succeeded. If the original `/turtle1/cmd_vel` remains, review the option name and the format after `--remap` (the colon-separated format `old_name:=new_name`).

</details>

### How to think about splitting responsibilities in a competition robot

So far we talked about one node, turtlesim. A real robot splits its functions into several nodes. For example, think of a setup where localization, path planning, and motor commands are separate nodes.

Why split them? The reason is that you can "test and restart them independently". If the localization node also goes down while you are fixing the path planning algorithm, you lose the robot's pose itself during debugging, and it becomes hard to isolate the cause. If you split the nodes, you only need to press `Ctrl+C` on the path planning node, fix it, and restart it. The localization node keeps running, so right after fixing you can move straight to checking the path planning behavior.

Another reason is replaceability. When you want to change path planning from A\* to another planner, change the controller from Lookahead to MPPI, or replace localization with one for the real robot, you can swap only the target if the interface is the same. This is the practical side of the "loose coupling" you saw until last time.

On the other hand, if you split too much, the overhead of communication between nodes and the management of startup order become troublesome. Putting everything in one node and splitting everything apart are both extremes. In real robot development, the practical balance is to cut at units that you want to verify independently or let fail independently.

## Advanced

The basic ROS 2 model of one node per process actually has an exception. With a mechanism called a **component (component node)**, you can put several nodes in the same process. If you put them in the same process and enable intra-process communication, data between nodes can be passed without copying. This reduces the load when you send large data such as images or LiDAR point clouds.

One caution here. **Making something a component does not remove copies automatically.** `use_intra_process_comms` in `rclcpp::NodeOptions` defaults to `false` (`/opt/ros/jazzy/include/rclcpp/rclcpp/node_options.hpp`). So unless you enable it explicitly, communication goes through DDS even if the nodes are in the same process. The official `composition_demo_launch.py` does not enable it either, so the talker/listener started by that launch file communicate over DDS although they are in the same process. To enable it, pass it into `NodeOptions` when you load into the container.

```python
ComposableNode(
    package='composition',
    plugin='composition::Talker',
    name='talker',
    extra_arguments=[{'use_intra_process_comms': True}],   # It has no effect without this
)
```

When you load from the CLI, pass it like `ros2 component load /ComponentManager composition composition::Talker -e use_intra_process_comms:=true`. In other words, a component is a mechanism that **satisfies the precondition** of "putting things in the same process", and zero copy itself is a separate opt-in.

This time we stop at "such a mechanism exists". The implementation of components, such as the `rclcpp_components` package and how to write a ComposableNode, is a theme big enough to be an article of its own, so we plan to cover it in a separate article.
## Summary

Once you can start, check, and remap nodes, next we go into topics, the communication between nodes. If you have not fully understood what we covered so far, your understanding of the next articles will be shallow. Reread once and check whether you can explain the output of `ros2 node info` in your own words. If something is unclear, ask someone experienced or check the official documentation.

### Matching exercise

After you read this chapter, practice with the matching drill exercises.

- `14_zero_copy` — Zero copy (unique_ptr and intra-process communication)
- `15_composition` — Make it a component (RCLCPP_COMPONENTS)

```bash
./drill run 14
./drill run 15
```

From the exercise side, you can come back to this chapter with `./drill read`.

## References

- Official tutorial: [Understanding ROS 2 nodes](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Nodes/Understanding-ROS2-Nodes.html)
- Previous: [03_getting_a_feel_with_turtlesim_and_rqt](03_getting_a_feel_with_turtlesim_and_rqt.md)
- Next: [05_topics](05_topics.md)
- Related: ROS 2 concepts (the overall picture of nodes, topics, services, and actions)
