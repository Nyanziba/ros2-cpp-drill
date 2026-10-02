# ROS 2 Lecture 08: Actions

## Introduction

So far you have used topics and services. This article covers the third way to communicate: actions. After this article, you can send a long task that can be canceled from the command line, and receive its progress in real time.

Prerequisites: You have finished [07_parameters](07_parameters.md). You also need an environment where turtlesim runs (see [03_getting_a_feel_with_turtlesim_and_rqt](03_getting_a_feel_with_turtlesim_and_rqt.md)).

## Lecture goals

- You can explain that an action consists of three parts: goal, feedback, and result
- You can inspect an action server with `ros2 action list` and `ros2 action info`
- You can send a goal with `ros2 action send_goal` and receive progress with `--feedback`
- You can choose between topics, services, and actions to fit the requirement

The estimated time is 1 hour. The target audience is new students. You need to know the basic operations of topics and services.

## Using this as a lecture

What to prepare:

- An environment with Ubuntu 24.04 / ROS 2 Jazzy Jalisco installed
- The `ros-jazzy-turtlesim` package (you should already have it from `sudo apt install ros-jazzy-turtlesim`)
- An environment where you can place two or more terminals side by side (tmux is recommended but not required)

Oral quiz (questions and model answers):

Q1. How do you choose between topics, services, and actions?

A1. Use a topic for a continuous flow of data (sensor values, cmd_vel, and so on). Use a service for a one-shot job that finishes immediately (changing a setting, asking for a state). Use an action for a job that takes time to finish, where you want to know the progress on the way and may want to stop it (driving a path, moving an arm). There are two criteria: "does the job take time?" and "do you want to cancel it?"

Q2. What do the goal, feedback, and result of an action each carry?

A2. The goal is the request from the client to the server (example: "rotate by 1.57 radians"). The feedback is the continuous progress notification from the server to the client (example: "the angle remaining now"). The result is the final outcome that the server returns when the task finishes (example: "the angle actually rotated"). The goal and the result are sent only once, but the feedback is sent many times while the job runs.

Q3. Give one situation where you should use an action instead of a service.

A3. For example, driving a path. It takes several seconds to tens of seconds to reach the destination, you want to know the remaining distance while driving, and you want to stop (cancel) when you detect an obstacle. With a service, the caller stays blocked and only waits for the result, so it can handle neither progress nor cancellation.

Suggested time plan:

- Concept explanation (goal/feedback/result, comparison table): 15 minutes
- Investigation with `ros2 action list/info`: 10 minutes
- turtlesim rotation exercise: 25 minutes
- Oral quiz: 10 minutes

## Main text

### What is an action?

What you learn: The three-part structure of an action, and how it differs from topics and services

Preparation: None (you will use turtlesim in the next section)

Content:

A topic is a one-way flow of data, and a service is a synchronous one-shot request. An action has a structure like a combination of these two, and it is made of the following three communication elements.

- goal: The request the client sends to the server. These are the parameters that say "I want you to do this"
- feedback: The progress the server keeps sending to the client. It is sent many times until the job ends
- result: The final outcome the server sends to the client. It is sent only once, when the job finishes (or is canceled, or fails)

Internally, an action is built from topics and services. Sending the goal and getting the result use a mechanism similar to a service (accepting the goal, getting the result), and feedback is delivered over a topic. This is why, if you look at `ros2 topic list` while an action is running, you can see a topic such as `/turtle1/rotate_absolute/_action/feedback`.

There are two decisive differences from a service. One is whether there is feedback, and the other is whether you can cancel. With a service, you send a request and can only wait until the result comes back. With an action, you can receive progress on the way, and you can also cancel on the way.

### Choosing between topics, services, and actions

What you learn: Criteria for choosing a communication method

Content:

The table below summarizes the three.

| Communication method | Use | Response | Example |
|---|---|---|---|
| Topic | Continuous flow of data | None (publish only) | Sensor values, cmd_vel |
| Service | One-shot job that finishes immediately | Synchronous, once | Getting a parameter, asking for a state |
| Action | Job that takes time, or that you may want to cancel | Goal accepted → feedback (many times) → result | Driving a path, moving an arm |

There are two criteria: "does the job take time?" and "might you want to stop it on the way?" If the job takes no time and finishes immediately, use a service. If it takes time but you can leave it alone and look only at the result later, a design that publishes the state on a topic is also possible. But if you need both progress tracking and cancellation, an action is the only choice.

### Inspecting action servers with ros2 action list / info

What you learn: How to inspect running action servers

Preparation: Start turtlesim.

```bash
ros2 run turtlesim turtlesim_node
```

Content:

In another terminal, you can list the running actions.

```bash
ros2 action list
```

`/turtle1/rotate_absolute` is shown. You can see the details with `info`.

```bash
ros2 action info /turtle1/rotate_absolute -t
```

With `-t`, the action type (`turtlesim/action/RotateAbsolute`) is also shown. You can also check the number of action servers and clients here.

To check the type definition of the action (the contents of goal, feedback, and result), use `ros2 interface show`.

```bash
ros2 interface show turtlesim/action/RotateAbsolute
```

The output has the following structure (notice that it is split into three parts by `---`).

```
float32 theta
---
float32 delta
---
float32 remaining
```

From the top, these are the goal (target angle theta), the result (angle actually rotated, delta), and the feedback (remaining angle, remaining). This `---` separator is the common format of msg/srv/action files. An action differs from a service (which has two blocks, goal and result, in effect) because it has three blocks: goal, result, and feedback.

### Exercise: Rotate turtlesim

What you learn: Sending a goal and receiving feedback with `ros2 action send_goal`

Preparation: turtlesim is running (you should have started it in the previous section).

Content:

Send a goal to `/turtle1/rotate_absolute` and rotate the turtle to the given angle.

```bash
ros2 action send_goal /turtle1/rotate_absolute turtlesim/action/RotateAbsolute "{theta: 1.57}" --feedback
```

With `--feedback`, the feedback during the job (remaining, the remaining angle) keeps flowing to the terminal. Without it, nothing is shown after the goal is sent, and only the result (delta, the angle actually rotated) comes back at the end.

Hint: The unit of theta is radians. 1.57 is about 90 degrees (π/2). If you want a full turn, try a value around 6.28 (2π).

<details markdown="1"><summary>Answer (example of actual output)</summary>

```
Waiting for an action server to become available...
Sending goal:
     theta: 1.57

Goal accepted with ID: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

Feedback:
    remaining: 1.2999999523162842

Feedback:
    remaining: 0.9099999666213989

...(remaining approaches 0)...

Result:
    delta: -1.5700000524520823

Goal finished with status: SUCCEEDED
```

If the turtle rotates 90 degrees in place, you succeeded. The delta is negative because of the sign convention of turtlesim's rotation direction (the sign can change with your environment and the initial pose). If you can see remaining getting smaller step by step, the feedback is being received correctly.

</details>

Let's also try canceling. If you press `Ctrl+C` in the terminal that is sending the goal, the client stops waiting. But to cancel the goal that the server is processing, you must send a separate cancel request. The `ros2 action` subcommands have no explicit cancel from the command line, so at this stage it is enough to experience "watching the feedback and stopping the wait with Ctrl+C". How to implement cancel handling that actually stops the work on the server side is covered in the package development part ([16_implementing_an_action_server](16_implementing_an_action_server.md)).

## Going further

With the `rqt` Action plugin (the rqt_action family), you can send goals and watch feedback from a GUI. Try it once you are used to the command line.

This article does not cover the implementation side of actions (how to write a server, how to receive cancel requests). They are covered in detail in [16_implementing_an_action_server](16_implementing_an_action_server.md) in Part 2.

This action mechanism is actually used in autonomous navigation stacks. move_base_flex receives the request "drive this path" through the ExePath action, and keeps returning the remaining distance as feedback while driving. The goal/feedback/result structure maps directly to "path and parameters", "remaining distance", and "driving result".

## Wrapping up

You now have all three: topics, services, and actions. With this, you have touched the basic communication methods of ROS 2. If you do not understand something, ask a senior student.

Next is [09_launch_and_ros2_bag](09_launch_and_ros2_bag.md), which covers starting many nodes at once and recording and replaying logs.

### Matching exercise

After you read this chapter, practice with the matching drill.

- `10_action_server` — Build an action server

```bash
./drill run 10
```

From the exercise, you can return to this chapter with `./drill read`.

## References

- [ROS 2 Documentation (Jazzy): Understanding actions](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Actions/Understanding-ROS2-Actions.html)
- Previous article: [07_parameters](07_parameters.md)
- Next article: [09_launch_and_ros2_bag](09_launch_and_ros2_bag.md)
- Hub article: [01_start_here_course_hub](01_start_here_course_hub.md)
