# ROS 2 Lecture 07: Parameters

## Introduction

This article covers ROS 2 parameters. Parameters are the setting values of a node. When you finish this article, you can list and change the parameters of a running node, write them out to a YAML file, and load them the next time you start.

As a prerequisite, we assume you have finished [04_nodes](04_nodes.md) and [05_topics](05_topics.md). If you are not yet used to operating turtlesim, read [03_getting_a_feel_with_turtlesim_and_rqt](03_getting_a_feel_with_turtlesim_and_rqt.md) first.

## Lecture goals / how to proceed

- Time needed: about 30 minutes
- Audience: people learning this for the first time who have read Part 1 up to 04
- Prerequisites: basic operation of the `ros2 node` and `ros2 topic` commands
- Goal: type the whole `ros2 param` set (list/get/set/dump/load) yourself, and reach a state where you can change the setting values of a running node

## Using this as a course

Things to prepare:
- A machine with Ubuntu 24.04 + ROS 2 Jazzy already set up (finished [02_environment_setup](02_environment_setup.md))
- The turtlesim package (it should already be installed with `sudo apt install ros-jazzy-turtlesim`)
- A state where you can open two or more terminals (tmux or terminal tabs are both fine)

Oral exam (questions and model answers):

1. Q. What is the difference between a parameter and a topic? What kind of values would you make parameters on a competition robot?
   A. A topic is data that flows continuously (such as sensor values and cmd_vel), and a parameter is a setting value that a node has. The value does not change until it is changed explicitly. On a competition robot, you make control gains (Kp/Ki/Kd), maximum speed, and robot dimensions (wheel diameter, track width) into parameters. If you embed these in code, every adjustment at the test-run site needs a rebuild and redeploy, which wastes time.
2. Q. I changed a value with `ros2 param set`, but the node's behavior does not change. What is happening?
   A. If the node does not implement a callback for parameter changes (such as `add_on_set_parameters_callback`), the value is rewritten on the parameter server, but it is not reflected in the node's internal variables. To reflect a change made at runtime in the actual behavior, the node must implement a callback and read the value again. This implementation is covered in Part 2, [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md).
3. Q. How do you use the YAML saved with `ros2 param dump` the next time you start?
   A. Start with the `--params-file` option, like `ros2 run package_name executable_name --ros-args --params-file path_of_saved_YAML`. You can also load the same YAML from a launch file (we touch on using it in launch in 09).

Time allocation guide:
- Explaining the concept and command demo (5 min)
- Tasks with turtlesim (15 min)
- dump/load task and oral exam (10 min)

## Main content

### What is a parameter

A parameter is a setting value that a node has at startup. It is not data that flows continuously like a topic. It means a value that stays constant until it is changed explicitly, such as "Kp of a PID", "maximum speed", or "frame ID".

In ROS 1, there was a separate command `rosparam` and a separate parameter server, but in ROS 2 parameters belong to a node. If you want to see the parameters of a certain node, you need to specify that node in the command.

### Task 1: List parameters

What you learn: Check the parameters a node has with `ros2 param list`

Preparation: Start turtlesim.

```bash
ros2 run turtlesim turtlesim_node
```

In another terminal, get the list.

```bash
ros2 param list
```

The list of parameters that belong to the `/turtlesim` node is shown. `background_r`, `background_g`, and `background_b` are the RGB values of the background color. Parameters common to all nodes, such as `use_sim_time`, are mixed in as well.

Hint: In an environment where several nodes are running, you can narrow down by specifying the node name, like `ros2 param list /turtlesim`.

### Task 2: Get and change parameter values

What you learn: Read and write values with `ros2 param get` and `ros2 param set`

Preparation: Keep the turtlesim from Task 1 running.

Content: Get the current red component of the background color.

```bash
ros2 param get /turtlesim background_r
```

The default should be `69`. Let us change it while it is running.

```bash
ros2 param set /turtlesim background_r 255
```

If you look at the turtlesim window, the redness of the background should change the next time the screen updates (you can change `background_g` and `background_b` in the same way. If you set all of them to 255, the background becomes white).

Hint: `ros2 param set` detects the type automatically. It works with numbers, strings, and booleans, but if you pass a value that does not match the parameter's type, you get an error. To check the type, use `ros2 param describe /turtlesim background_r`.

Practice question: Write three commands that change the turtlesim background color to purple (about R=128, G=0, B=128).

<details markdown="1"><summary>Answer</summary>

```bash
ros2 param set /turtlesim background_r 128
ros2 param set /turtlesim background_g 0
ros2 param set /turtlesim background_b 128
```

</details>

> Column: Why parameterizing matters in competition robots
>
> If you hard-code control gains and maximum speed in the code, then at the test-run site, when you think "I want it to move a bit faster" or "it is vibrating, so I want to lower Kp", you need to fix the code, rebuild, and redeploy. If you parameterize them, one `ros2 param set` or rewriting one line of YAML is enough. This difference is big in on-site adjustment the day before the contest.

### Task 3: Write parameters to YAML and load them

What you learn: Persistence with `ros2 param dump` and `--params-file`

Preparation: Use the turtlesim whose background color you changed in Task 2.

Content: Write all current parameters out to YAML.

```bash
ros2 param dump /turtlesim > turtlesim_params.yaml
```

Look at the content with `cat turtlesim_params.yaml`. It is a YAML structure keyed by the node name, and the values you changed, such as `background_r`, are reflected as they are.

End turtlesim once (Ctrl+C), and restart it with the saved YAML specified.

```bash
ros2 run turtlesim turtlesim_node --ros-args --params-file turtlesim_params.yaml
```

Check that the background color has the changed value right from startup. There is also a command called `ros2 param load`, but it is for applying the content of YAML to a node that is already running. When you want to load at startup, use `--params-file`.

Hint: `--params-file` may ignore parameters that the node did not declare with `declare_parameter` at startup. A parameter that is declared in advance, like those of turtlesim, is reflected without problems.

## Advanced (bonus)

Even if you change a value at runtime with `ros2 param set`, it may not be reflected. Even if the value on the parameter server is rewritten, if the variable used inside the node does not detect the change, it has no effect on behavior. To reflect it, the node must implement a callback using `add_on_set_parameters_callback`, and receive the change and update its internal state. This implementation is covered in Part 2, [15_parameters_and_launch_in_practice](15_parameters_and_launch_in_practice.md). Here, just remember the fact that "set sometimes has no effect".

## Summary

Parameters are the basic means of separating code from tuning values in competition robot development. If you parameterize control gains and robot dimensions, adjustment at the test-run site becomes much faster. Next, go on to [08_actions](08_actions.md). If there is anything you do not understand, ask someone experienced or check the official documentation.

### Matching exercise

After you read this chapter, practice with the matching drill exercises.

- `06_parameters` — Use parameters inside a class
- `07_param_events` — Watch for parameter changes
- `08_params_yaml` — Manage parameters with YAML

```bash
./drill run 06
./drill run 07
./drill run 08
```

From the exercise side, you can come back to this chapter with `./drill read`.

## References

- [ROS 2 Documentation (Jazzy): Understanding parameters](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Parameters/Understanding-ROS2-Parameters.html)
- [06_services](06_services.md)
- [08_actions](08_actions.md)
