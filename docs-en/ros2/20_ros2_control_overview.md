# ROS 2 Lecture 20: ros2_control Overview

## Introduction

After this article, you can draw the overall structure of ros2_control (controller_manager, controllers, hardware_interface). You can also discuss in your own words the design decision of how much PID computation to leave to the real robot or the microcontroller.

The prerequisite is that you have finished [19_writing_urdf](19_writing_urdf.md). ros2_control starts from the `<ros2_control>` tag in the URDF, so without a URDF it is hard to see what you are doing.

## Lecture goals

- Explain the roles of controller_manager, controllers, and hardware_interface
- Understand the concepts of command interface and state interface, and apply them to your own hardware
- Discuss from both sides the real design decision: "run PID on the microcontroller, or leave it to a controller on the PC?"
- Run the diffbot example from ros2_control_demos with mock hardware

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy Jalisco set up (finished [02_environment_setup](02_environment_setup.md))
- The `ros-jazzy-ros2-control` and `ros-jazzy-ros2-controllers` packages (`sudo apt install ros-jazzy-ros2-control ros-jazzy-ros2-controllers`)
- A workspace to clone `ros2_control_demos` into (finished [10_workspaces_and_colcon](10_workspaces_and_colcon.md))
- Gazebo is not needed. This demo runs on mock hardware, so you do not have to wait for a simulator to be installed

### Suggested time plan

- Overview of controller_manager / hardware_interface: 15 min
- Diagrams and examples of command/state interfaces: 10 min
- Exercise: run the diffbot demo: 20 min
- Discussion: where should PID run?: 15 min

### Oral questions

**Q1. What is the difference in role between controller_manager and a controller (such as diff_drive_controller)?**

Model answer: controller_manager reads the `<ros2_control>` tag in the URDF and manages which controller is tied to which hardware interface. It is the "command center". It runs a real-time loop at a fixed period and executes read → update → write. The controller itself (such as diff_drive_controller) holds the specific computation logic inside that period, for example "receive cmd_vel and convert it to speed commands for the left and right wheels". controller_manager is in charge of swapping, starting, and stopping controllers, but it does not contain the control algorithm itself.

**Q2. Explain the difference between command interface and state interface, using your own hardware as an example.**

Model answer: A command interface is a value the controller passes to the hardware to say "I want you to move like this" (for example, a joint's target velocity or target torque). A state interface is a value the hardware returns to the controller to say "this is the current state" (for example, the current position or velocity read from an encoder). For your own hardware, this maps as follows: `write()` of the hardware_interface plugin sends the command interface values to the microcontroller over CAN, and `read()` writes the encoder values returned from the microcontroller into the state interfaces.

**Q3. There are two designs: running PID on the microcontroller, or leaving PID to a ros2_control controller on the PC. Which one does your team use, and why?**

Example of a model answer: In the design with PID on the microcontroller, the microcontroller receives a speed command (equivalent to cmd_vel), watches the encoder feedback, finishes the whole PID computation inside the microcontroller, and outputs to the motor driver. The control loop runs at a fixed period regardless of the communication rate, so the motor response is less likely to degrade even if CAN communication is somewhat delayed or drops messages. In the design with PID on the PC, gains can be managed in one place as ROS 2 parameters, and you can swap the simulation controller and the real-robot controller with the same configuration. However, the PID period is tied to the ROS 2 communication rate and the controller_manager loop period, so it can be a disadvantage when real-time requirements are strict.

## Main content

### Topic: The big picture of controller_manager and hardware_interface

Topic: Get an overview of what ros2_control is responsible for.

Preparation: You have finished [19_writing_urdf](19_writing_urdf.md).

Content:

ros2_control is a framework for "moving the joints and motors of a robot in the standard ROS 2 way". At the center is a node called controller_manager. It runs a real-time loop at a fixed period and repeats read (read the state from the hardware) → update (run the controllers' computation) → write (write commands to the hardware).

There are three main components.

- controller_manager: the command center that manages the link between controllers and hardware_interface, and runs the loop
- Controllers: plugins that hold specific control logic, such as `diff_drive_controller` (converts a differential-drive velocity command into left and right wheel commands) and `joint_trajectory_controller` (trajectory following for arms and similar)
- hardware_interface: the contact point with the real robot (or a simulator, or mock). You implement the part that talks to the real motors and encoders over CAN or serial yourself

The contact point with your own hardware is hardware_interface. You write a plugin here that inherits `SystemInterface` or `ActuatorInterface`, get encoder values and so on in `read()`, and send command values to the microcontroller in `write()`. In other words, you do not need to write the upper parts such as controller_manager and diff_drive_controller anew. Only hardware_interface is implemented for each robot.

### Topic: command interface and state interface

Topic: Understand the form of the values exchanged between controller_manager and hardware_interface.

Preparation: None.

Content:

Command interfaces and state interfaces are declared for each joint inside the `<ros2_control>` tag in the URDF.

```xml
<ros2_control name="DiffBotSystem" type="system">
  <hardware>
    <plugin>diffbot_base/DiffBotSystemHardware</plugin>
  </hardware>
  <joint name="left_wheel_joint">
    <command_interface name="velocity"/>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
  </joint>
  <joint name="right_wheel_joint">
    <command_interface name="velocity"/>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
  </joint>
</ros2_control>
```

A command interface is the "I want you to move like this" value passed from the controller to the hardware. A state interface is the "this is the current state" value returned from the hardware to the controller. In the example above, each of the left and right wheels has a velocity command interface and position/velocity state interfaces. diff_drive_controller receives cmd_vel and writes a value into this velocity command interface, and `write()` of hardware_interface sends that value to the actual motor driver.

The interface types (position/velocity/effort) must match what the controller requires, or they cannot be combined. `diff_drive_controller` requires a velocity command interface, so if the hardware side only provides effort (torque), they will not connect. Complaining that "it does not work" without checking this correspondence is a common pitfall.

### Topic: Run the diffbot demo on mock hardware

Topic: Clone the diffbot example from ros2_control_demos and check that it works without a real robot and without Gazebo.

Preparation: `ros-jazzy-ros2-control` and `ros-jazzy-ros2-controllers` are installed.

Content:

Clone the demos under src in your workspace.

```bash
cd ~/ros2_ws/src
git clone -b jazzy https://github.com/ros-controls/ros2_control_demos.git
cd ~/ros2_ws
colcon build --packages-up-to ros2_control_demo_example_2
source install/setup.bash
```

The diffbot example (`ros2_control_demo_example_2`) runs on mock hardware, so you need neither a real robot nor Gazebo. The URDF only specifies a mock hardware plugin inside the `<ros2_control>` tag, so you can check the behavior of controller_manager and diff_drive_controller as it is.

```bash
ros2 launch ros2_control_demo_example_2 diffbot.launch.py
```

After it starts, check the running controllers in another terminal.

```bash
ros2 control list_controllers
```

If `diff_drive_controller` is `active`, you can send cmd_vel and check the behavior.

```bash
ros2 topic pub --rate 10 /cmd_vel geometry_msgs/msg/TwistStamped \
  "{twist: {linear: {x: 0.3}, angular: {z: 0.1}}}"
```

Be careful about the topic name. The default of `diff_drive_controller` is `~/cmd_vel` (that is, `/diffbot_base_controller/cmd_vel`). But `diffbot.launch.py` in `ros2_control_demos` passes `--controller-ros-args "-r ~/cmd_vel:=/cmd_vel"` to the spawner and remaps it, so the topic actually subscribed to is `/cmd_vel`. If you publish to `/diffbot_base_controller/cmd_vel`, nobody receives it. When in doubt, the reliable way is to check the subscribers with `ros2 topic info -v /cmd_vel`.

Because this is mock hardware, nothing really moves. But if you echo `/dynamic_joint_states` or `/joint_states`, you can see the values written to the command interfaces reflected as they are in the state interfaces and returned. The purpose of this exercise is to get a feel for this loop: "the value computed by the controller goes to hardware_interface and comes back".

**Exercise**: Run `ros2 control list_hardware_interfaces` and check which command interfaces and state interfaces are defined for `left_wheel_joint` and `right_wheel_joint`.

<details markdown="1"><summary>Answer</summary>

When you run `ros2 control list_hardware_interfaces`, velocity is listed under command interfaces, and position/velocity under state interfaces, for both wheels. This matches the declaration in the `<ros2_control>` tag of the diffbot URDF (in the description package of `ros2_control_demo_example_2`). Instead of preparing a real robot, you can first read this declaration on the URDF side, and it tells you in advance which values are expected to flow.

</details>

### Topic: The question of where to run PID

Topic: Compare two designs: PID on the microcontroller, and PID in a ros2_control controller on the PC.

Preparation: None.

Content:

In the design with PID on the microcontroller, the PC only sends a speed command equivalent to cmd_vel to the microcontroller over CAN. The actual PID computation (how to shrink the error between the target speed and the encoder value) is done on the microcontroller. When you introduce ros2_control, the design changes depending on where you put this computation.

With PID on the microcontroller, a control loop with a fixed period runs close to the motor driver, so the control loop itself does not stop even if CAN communication is somewhat disturbed. For gain tuning, you need to rewrite the microcontroller firmware, or build a separate mechanism that sends parameters over serial.

In the design where a ros2_control controller on the PC takes over the PID computation (for example, putting a controller in charge of speed control after `diff_drive_controller`, or using a controller plugin for PID), gains can be managed in one place as ROS 2 parameters, through `ros2 param set` or YAML. You can also swap the real-robot controller and the simulation controller through the same interface. Another advantage is that you can look at the gains with `ros2 control list_controllers` or rqt while debugging. But in this design, the controller_manager loop period and the communication rate with the microcontroller (for CAN, the time for a send-and-receive round trip) become the lower limit of the control period. Because the path from reading a sensor value to sending a command goes through ROS 2 communication, the delay is larger than in a design that stays inside the microcontroller, and the effect is bigger when communication gets congested.

Neither is simply correct. It depends on the control period you need and how much margin the microcontroller-PC communication has. For a slow mechanism with margin, you can choose to move toward the PC side and take the ease of parameter management. For a drive system that needs fast response, you can also keep PID on the microcontroller as before and use ros2_control only to unify the command path from the upper layers.

## Going further

This article does not give a conclusion on whether to adopt ros2_control on your own robot. Here are the axes you can use to decide.

- Is the URDF already complete for all links and joints of the real robot? (The cost of preparing it is directly the cost of adopting ros2_control)
- Compare the required control period with the measured latency of CAN/serial
- How much you care about swapping with a simulation (Gazebo/MuJoCo)
- How thin you want the microcontroller firmware to be (the thinner it is, the more responsibility and real-time requirements go to the PC side)
- How much experience the team has with managing gains as ROS 2 parameters


## Conclusion

ros2_control is a mechanism that assumes a URDF exists. If you touch it without that prerequisite, you will get stuck on "why does it not work?", so keep the order and start from [19_writing_urdf](19_writing_urdf.md). If you do not understand something, ask someone experienced or check the official documentation.

Next is [21_sensor_integration](21_sensor_integration.md), which covers how to bring LiDAR and IMU into ROS 2.

## References

- [ros2_control official docs (control.ros.org)](https://control.ros.org/jazzy/index.html)
- [ROS 2 Documentation: Jazzy — ros2_control pages](https://docs.ros.org/en/jazzy/p/ros2_control/)
- [ros2_control_demos (GitHub)](https://github.com/ros-controls/ros2_control_demos)
- [19_writing_urdf](19_writing_urdf.md)
- [21_sensor_integration](21_sensor_integration.md)
