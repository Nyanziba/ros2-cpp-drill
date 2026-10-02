# ROS 2 Lecture 20b: Using ros2_control (a differential-drive base as the example)

## Introduction

After this article, you can read an example workspace of a differential-drive robot and follow it from how to write your own hardware_interface, through controller settings, to checking the startup.

The prerequisite is [20_ros2_control_overview](20_ros2_control_overview.md). This article skips the explanation of the three-layer structure of controller_manager / controllers / hardware_interface, so read 20 first if it is not clear. This article is the practice part that follows the concept part. The goal is to get the viewpoint of someone who uses ros2_control, by reading real code.

## Lecture goals

- Explain the workspace layout of a robot project and the design decision of moving fully to ros2_control
- Using `ExampleChipmdHardware` as the example, read how the SystemInterface lifecycle (on_init → on_configure → on_activate → read/write loop) corresponds to the `<ros2_control>` tag of the URDF
- Understand the contents of `controllers.yaml` and the role of the spawner, and explain the startup order
- Run the steps to check the state with `ros2 control list_controllers` / `list_hardware_interfaces`

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy Jalisco set up (finished [02_environment_setup](02_environment_setup.md))
- The robot project workspace is already cloned (it runs with `use_mock_hardware:=true` even without a real robot)
- `ros-jazzy-ros2-control` and `ros-jazzy-ros2-controllers` (in a pixi environment they should come in through `pixi.toml`, so if they are missing, check that first)
- No real CAN/serial connection is needed. This lecture is done entirely with mock hardware

### Suggested time plan

- Summary of the workspace layout and ADR 0004: 10 min
- Lifecycle explanation while reading `robot_chipmd_hardware.cpp`: 20 min
- Explanation of `controllers.yaml` and the launch startup order: 15 min
- Exercise: start the mock and run `list_controllers` / `list_hardware_interfaces`: 15 min
- Oral questions: 10 min

### Oral questions

**Q1. Explain what read and write each do, using `ExampleChipmdHardware` as the example.**

Model answer: read reflects the latest feedback from the hardware into the state interfaces. `read()` in `robot_chipmd_hardware.cpp` reads `rx_buffer_` (a RealtimeBuffer of the encoder values received from the `/motor/state` topic), and if it has not timed out, updates `joint.state_position` / `joint.state_velocity`. write sends the values written to the command interfaces to the hardware. `write()` puts `joint.command_voltage` into `tx_buffer_`, and a 1 kHz timer on a separate thread (`publish_latest_command`) publishes it to `/board/chipmd/target` and passes it to the microcontroller over CAN. The important point is that read/write themselves do not call ROS communication directly. They only go through buffers prepared in advance that are real-time safe.

**Q2. What does the spawner spawn? Include the difference between `hardware_spawner` and `spawner`.**

Model answer: They are two kinds of Node provided by the `controller_manager` package. `hardware_spawner` takes a hardware component (for example `ExampleChipmdWheelSystem`) through configure → activate with `--activate`. `spawner` loads controllers (such as `joint_state_broadcaster` and `mobility_controller`) into controller_manager and activates them. In `_subsystem_bringup.py`, the controller spawner is started after hardware_spawner exits (`OnProcessExit`). This is because of the ordering constraint that "a controller cannot claim its interfaces until the matching hardware is active".

**Q3. Why is the initial state of all hardware components `unconfigured` in `base_controllers.yaml`?**

Model answer: If `hardware_components_initial_state` is `unconfigured`, nobody opens any device when only base.launch.py has been started. The launch of each subsystem (`mobility`/`hand`/`lift`) is designed to explicitly bring up only the components it uses, with `hardware_spawner --activate`. This avoids unwanted side effects, such as opening the hand's serial port (`/dev/feetech`) when you only want to bench-test the wheels.

## Main content

### The big picture of the workspace

The workspace in this example splits packages by area.

```text
src/
├── bringup/    robot_bringup (launch and controllers.yaml are gathered here)
├── comm/       robot_comm_board_router, robot_comm_can_gateway
├── control/    robot_mobility_controller, robot_lift_position_controller, ...
├── description/ robot_description (URDF/xacro)
├── hardware/   robot_hardware_chipmd, _solenoid, _sts3215
├── msgs/       robot_ros_msgs
├── operation/  robot_operation_teleop
├── perception/ robot_perception_lidar
└── utils/      robot_external_protocol_check
```

The rule of this repository is the layout `src/<area>/<package>`, with launch files and shared settings gathered in `robot_bringup` (from `README.md`). The packages in hardware/ are the implementations of your own hardware_interface, and the packages in control/ are the implementations of your own controllers. These two are the parts plugged in as ros2_control plugins.

This repository went through a design decision called `ADR 0004: Update to the full ros2_control migration architecture`. It says that the plan at first was a partial migration of only wheel/lift, but it was switched to a full migration that integrates all hardware components with ros2_control, "to improve the consistency and maintainability of the architecture". The earlier PoC implementation had complex paths such as cascade control and direct output of velocity commands. The production architecture removes them, and the Wheel path is unified into a single path:

```text
/cmd_vel → ExampleMobilityController → (wheel velocity references) → Wheel PID Controllers → (voltage command) → ExampleChipmdHardware
```

(`docs/adr/0004-ros2-control-migration.md`). The path that passed velocity commands straight to hardware_interface is abolished, and the decision is that a PID controller must always sit in between. There is an article that discusses whether to run PID "on the microcontroller or on the PC" (see [20_ros2_control_overview](20_ros2_control_overview.md)). This repository chose the design that puts PID (`wheel_pid_controller`) in a controller on the PC.

### Reading your own hardware_interface: ExampleChipmdHardware

`src/hardware/robot_hardware_chipmd/` is the `hardware_interface::SystemInterface` implementation for the ChipMD motor driver board (it manages 4 wheels and 1 lift axis). If you look at the header (`include/robot_hardware_chipmd/robot_chipmd_hardware.hpp`), the lifecycle methods are all lined up.

```cpp
hardware_interface::CallbackReturn on_init(
  const hardware_interface::HardwareComponentInterfaceParams& params) override;
std::vector<hardware_interface::StateInterface> export_state_interfaces() override;
std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;
hardware_interface::CallbackReturn on_activate(const rclcpp_lifecycle::State&) override;
hardware_interface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State&) override;
hardware_interface::CallbackReturn on_error(const rclcpp_lifecycle::State&) override;
hardware_interface::CallbackReturn on_shutdown(const rclcpp_lifecycle::State&) override;
hardware_interface::return_type read(const rclcpp::Time&, const rclcpp::Duration&) override;
hardware_interface::return_type write(const rclcpp::Time&, const rclcpp::Duration&) override;
```

(Source: `src/hardware/robot_hardware_chipmd/include/robot_hardware_chipmd/robot_chipmd_hardware.hpp`)

If you noticed that `on_configure` is not declared separately, good eye. This class does everything inside `on_init`: the joint consistency check and even the Pub/Sub setup.

`on_init` looks at `info_.joints` (the list of joints passed from the `<ros2_control>` tag of the URDF) one by one. It checks whether each joint has a `voltage` command interface, and whether it has a `position` or `velocity` state interface (`src/hardware/robot_hardware_chipmd/src/robot_chipmd_hardware.cpp`).

```cpp
data.has_voltage_command = has_interface(
  joint.command_interfaces, "voltage");
data.has_position_state = has_interface(
  joint.state_interfaces, hardware_interface::HW_IF_POSITION);
data.has_velocity_state = has_interface(
  joint.state_interfaces, hardware_interface::HW_IF_VELOCITY);

if (!data.has_position_state && !data.has_velocity_state) {
  RCLCPP_ERROR(...);
  return hardware_interface::CallbackReturn::ERROR;
}
if (!data.has_voltage_command) {
  RCLCPP_ERROR(...);
  return hardware_interface::CallbackReturn::ERROR;
}
```

In other words, this hardware enforces the contract "accept a voltage command and return position/velocity state" at the time of `on_init`. If you write a joint in the URDF that does not meet this contract, `on_init` returns `ERROR` and the startup of controller_manager stops. The URDF description that has to match here is `robot_hardware_real.xacro`, which we look at next.

```xml
<ros2_control name="ExampleChipmdWheelSystem" type="system">
  <hardware>
    <plugin>robot_hardware_chipmd/ExampleChipmdHardware</plugin>
    <param name="feedback_timeout_sec">0.5</param>
    <param name="startup_grace_timeout_sec">5.0</param>
    <param name="tx_publish_period_sec">0.001</param>
  </hardware>
  <joint name="front_left_wheel_joint">
    <command_interface name="voltage"/>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
  </joint>
  <!-- front_right / rear_left / rear_right are the same -->
</ros2_control>
```

(Source: `src/description/robot_description/urdf/robot_hardware_real.xacro`)

Values such as `feedback_timeout_sec` passed with `<param>` are read from `info_.hardware_parameters` with `std::stod` inside `on_init`. The URDF tags and the C++ code are tied together like this: "the name/command_interface/state_interface of `<joint>` become the validation conditions of JointData, and `<param>` flows in as hardware_parameters".

`export_state_interfaces` / `export_command_interfaces` create interface objects from the validated joints and hand them to controller_manager.

```cpp
std::vector<hardware_interface::StateInterface> ExampleChipmdHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  for (auto& joint : joints_) {
    if (joint.has_position_state) {
      state_interfaces.emplace_back(joint.name, hardware_interface::HW_IF_POSITION, &joint.state_position);
    }
    if (joint.has_velocity_state) {
      state_interfaces.emplace_back(joint.name, hardware_interface::HW_IF_VELOCITY, &joint.state_velocity);
    }
  }
  return state_interfaces;
}
```

The key point is that it passes a pointer to a member variable, like `&joint.state_position`. controller_manager comes to read the value through this pointer, so when `read()` updates `joint.state_position`, the new value reaches the controller side with nothing else needed. There is no copy and no message sending in between. This is the basic design of ros2_control interfaces.

`read()` and `write()` share the work like this.

- `read()`: the latest value that the callback of the subscriber for the `/motor/state` topic (encoder feedback from the ChipMD board) wrote into `rx_buffer_` (`realtime_tools::RealtimeBuffer`) is taken out from the real-time loop with `readFromRT()`. If it has not timed out, it is reflected in `joint.state_position` / `state_velocity`
- `write()`: `joint.command_voltage` written by the controller is gathered into a `TxCommand` with `build_tx_command()` and set into `tx_buffer_` (a `RealtimeThreadSafeBox`). The actual Publish is not done by `write()` itself. A separate 1 kHz wall timer, `tx_timer_`, does it

```cpp
hardware_interface::return_type ExampleChipmdHardware::write(
  const rclcpp::Time&, const rclcpp::Duration&)
{
  if (!tx_buffer_.try_set(build_tx_command(is_latched_fault_))) {
    RCLCPP_DEBUG_THROTTLE(...);
  }
  return hardware_interface::return_type::OK;
}
```

(Source: the same cpp)

read/write do not call ROS communication directly. Pub/Sub is separated into other paths, a callback and a timer. This design keeps the real-time loop of controller_manager (the `update_rate: 200` Hz described later) from being disturbed by the latency of ROS communication. read/write stay light: "only read and write the buffer and return at once". The actual network I/O is left to the other, non-real-time paths of the callback and the timer.

It also implements this mechanism: if feedback is lost for a certain time, `is_latched_fault_` is set, and from then on `read()` returns `hardware_interface::return_type::ERROR` to pass the error up to controller_manager. `on_error` / `on_deactivate` / `on_shutdown` all call `force_zero_output()`, which sets command_voltage to 0 and publishes once at once. By putting the "how to stop" in several lifecycle hooks in the same form, the design guarantees that the output becomes zero whichever path it stops through.

### Controller settings and startup

The controllers.yaml files are split under `src/bringup/robot_bringup/config/`. This directory is organized into subdirectories for each area (`hardware/`, `mobility/`, `manipulator/`, `navigation/`, `operation/`, `simulation/`, `udev/`). First, `hardware/base_controllers.yaml` is the foundation.

```yaml
controller_manager:
  ros__parameters:
    update_rate: 200

    hardware_components_initial_state:
      unconfigured:
        - ExampleChipmdWheelSystem
        - ExampleChipmdLiftSystem
        - ExampleSts3215Bus0
        - ExampleSts3215Bus1
        - ExampleSts3215Bus2
        - ExampleSolenoidSystem

    joint_state_broadcaster:
      type: joint_state_broadcaster/JointStateBroadcaster
```

(Source: `src/bringup/robot_bringup/config/hardware/base_controllers.yaml`)

The STS3215 hand is split into three components, `Bus0`/`Bus1`/`Bus2`, because the serial servos are spread over three buses. If you hang all the servos on one bus, the bandwidth is not enough, so hardware components are cut per physical bus. The principle "the granularity of a hardware component = the unit you want to configure/activate independently" applies here too.

`update_rate: 200` is the period of the real-time loop of controller_manager (200 Hz, that is, read → update → write every 5 ms). All components are `unconfigured` in `hardware_components_initial_state`, as explained in oral question Q3, so that base.launch.py alone starts nothing.

On top of this file, the per-subsystem `mobility/mobility_controllers.yaml` is layered.

```yaml
controller_manager:
  ros__parameters:
    mobility_controller:
      type: omni_wheel_drive_controller/OmniWheelDriveController
    wheel_pid_controller:
      type: pid_controller/PidController

mobility_controller:
  ros__parameters:
    # The wheels are in equal spacing, counter-clockwise order. First is front-left (+45 degrees from +X of base_link)
    wheel_offset: 0.7853981633974483
    wheel_names:
      - wheel_pid_controller/front_left_wheel_joint
      - wheel_pid_controller/rear_left_wheel_joint
      - wheel_pid_controller/rear_right_wheel_joint
      - wheel_pid_controller/front_right_wheel_joint
    robot_radius: 0.29698484809834996
    wheel_radius: 0.05
    open_loop: false
    position_feedback: false      # The PID exports the measured velocity, not the position
    enable_odom_tf: false         # odom -> base_link is owned by odometry_fusion

wheel_pid_controller:
  ros__parameters:
    dof_names:
      - front_left_wheel_joint
      - front_right_wheel_joint
      - rear_left_wheel_joint
      - rear_right_wheel_joint
    reference_and_state_interfaces:
      - velocity
    command_interface: voltage
    gains:
      front_left_wheel_joint:
        p: 0.04
        i: 0.0
        d: 0.0
        i_clamp_max: 5.0
        i_clamp_min: -5.0
        u_clamp_max: 0.25
        u_clamp_min: -0.25
        antiwindup_strategy: conditional_integration
      # front_right / rear_left / rear_right have the same gains
```

(Source: `src/bringup/robot_bringup/config/mobility/mobility_controllers.yaml`)

Please notice that **neither the upper stage nor the lower stage is written by us**. `mobility_controller` is the standard `omni_wheel_drive_controller/OmniWheelDriveController` from `ros2_controllers`, and `wheel_pid_controller` is also the standard `pid_controller/PidController`. Both the kinematics of the 4-wheel omni base and the PID computation work with a combination of standard controllers only.

This repository still has its own `ChainableControllerInterface` implementation, `src/control/robot_mobility_controller`, but no YAML refers to it now (only the plugin registration file `robot_mobility_controller.xml` is left). In the past, a custom controller did the cmd_vel → 4-wheel conversion, and it was replaced with the standard `omni_wheel_drive_controller`. Read this as a real example of the order **"first check whether a standard controller is enough, and write your own only for what is missing"**. On the other hand, if you leave an unused custom plugin without deleting it, later readers get confused about "which one is the real one".

The fact that `wheel_names` has a controller-name prefix, like `wheel_pid_controller/front_left_wheel_joint`, is the controller chain mentioned in [20_ros2_control_overview](20_ros2_control_overview.md). `omni_wheel_drive_controller` does not write to the command interfaces of the hardware directly. It writes to the reference interfaces that the preceding `wheel_pid_controller` exposes. Then `wheel_pid_controller` writes to the real hardware with `command_interface: voltage`. It is a two-stage structure: cmd_vel → target speed of each wheel → PID → voltage.

Be aware of the units before touching the gains. The error input of `wheel_pid_controller` is `velocity` (rad/s), but the output is a command interface called `voltage`, a pseudo voltage, clamped in a dimensionless range such as `u_clamp_max: 0.25`. The small value `p: 0.04` also does this unit conversion, so if you tweak it with the idea "the response is weak, so I will multiply it by 10", it saturates at once.

The startup order is summarized in `docs/BRINGUP_CHECKLIST.md` and `_subsystem_bringup.py`. The launch of each subsystem has two stages.

1. `hardware_spawner` starts the target hardware component (for example `ExampleChipmdWheelSystem`) with `--activate`, and completes configure → activate
2. It detects the exit of the `hardware_spawner` process (`OnProcessExit`), and `spawner` starts `joint_state_broadcaster` and the target controllers (`mobility_controller`, `wheel_pid_controller`, and so on)

```python
hardware_spawner = Node(
    package="controller_manager",
    executable="hardware_spawner",
    arguments=[*hardware_components, "--activate", "--controller-manager", CONTROLLER_MANAGER_NAME],
    output="screen",
)
controller_spawner = Node(
    package="controller_manager",
    executable="spawner",
    arguments=["joint_state_broadcaster", *controllers, "--controller-manager", CONTROLLER_MANAGER_NAME],
    output="screen",
)
spawn_controllers_after_hardware = RegisterEventHandler(
    OnProcessExit(target_action=hardware_spawner, on_exit=[controller_spawner]),
)
```

(Source: `src/bringup/robot_bringup/launch/_subsystem_bringup.py`)

A controller cannot claim its interfaces until the matching hardware component is active, so if you do not keep this order, the spawner fails because it cannot find the interfaces. Spawning `joint_state_broadcaster` inside the subsystem launch is also intentional. If you start several subsystems separately, it is spawned twice. So when you want to use everything at once, use `robot.launch.py` (the integrated entry point that activates all components and runs the spawner only once), not the individual launches.

### Run it and check

Even without a real robot, you can check the behavior of controller_manager with mock hardware. Follow the steps in `docs/BRINGUP_CHECKLIST.md`.

```bash
colcon build
source install/setup.bash
# Dry run (mock, no CAN, no IMU)
ros2 launch robot_bringup robot.launch.py \
    use_mock_hardware:=true start_board_io:=false start_dm_imu:=false
```

`use_mock_hardware:=true` alone is not enough. The default of `start_board_io` and `start_dm_imu` is `true`, so the CAN gateway and DM-IMU nodes try to open real devices and fail. When you have no real robot at hand, pass all three together (this command is written as "dry run (mock, no CAN)" in the docstring at the top of `robot.launch.py`).

In another terminal, look at the list of running controllers.

```bash
ros2 control list_controllers
```

The actual output in the mock setup is as follows.

```text
solenoid_command_controller robot_solenoid_command_controller/ExampleSolenoidCommandController  active
lift_position_controller    robot_lift_position_controller/ExampleLiftPositionController        active
hand_controller             robot_hand_controller/ExampleHandController                         active
mobility_controller         omni_wheel_drive_controller/OmniWheelDriveController                    active
wheel_pid_controller        pid_controller/PidController                                            active
joint_state_broadcaster     joint_state_broadcaster/JointStateBroadcaster                           active
```

If all six are `active`, it is as expected. The left column is the instance name, and the middle column is the `type` (= the class name of the plugin). You can read from each line that **only three controllers are our own (solenoid / lift / hand), and the two for the drive base are standard controllers**.

To look at the hardware component side, use these two.

```bash
ros2 control list_hardware_components
ros2 control list_hardware_interfaces
```

The output of `list_hardware_components` (an excerpt) looks like this.

```text
Hardware Component 1
	name: ExampleSts3215Bus0
	type: system
	plugin name: mock_components/GenericSystem
	state: id=3 label=active
	read/write rate: 200 Hz
	is_async: False
	command interfaces
		right_1_tip_joint/position [available] [claimed]
		right_0_tip_joint/position [available] [claimed]
		...
```

`read/write rate: 200 Hz` corresponds to `update_rate: 200` in `base_controllers.yaml`. `[claimed]` means that some controller holds that interface. If you do not make the controllers `active`, you see only `[available]`.

If you pick out the wheel-related part of `list_hardware_interfaces`, the shape of the controller chain becomes clear.

```text
command interfaces
	base_to_lift/voltage [available] [claimed]
	front_left_wheel_joint/velocity [available] [claimed]        ← mock hardware (voltage on the real robot)
	front_right_wheel_joint/velocity [available] [claimed]
	rear_left_wheel_joint/velocity [available] [claimed]
	rear_right_wheel_joint/velocity [available] [claimed]
	wheel_pid_controller/front_left_wheel_joint/velocity [available] [claimed]   ← exposed by the PID
	wheel_pid_controller/front_right_wheel_joint/velocity [available] [claimed]     reference interface
	wheel_pid_controller/rear_left_wheel_joint/velocity [available] [claimed]
	wheel_pid_controller/rear_right_wheel_joint/velocity [available] [claimed]
state interfaces
	front_left_wheel_joint/position
	front_left_wheel_joint/velocity
	...
```

Please notice that **there are two kinds of command interfaces**. `front_left_wheel_joint/velocity` without a prefix is the hardware interface, and the one with `wheel_pid_controller/` is the reference interface that `wheel_pid_controller` itself exposes. `mobility_controller` writes to the latter, and `wheel_pid_controller` writes to the former. The `wheel_pid_controller/front_left_wheel_joint` written in `wheel_names` of `mobility_controllers.yaml` was exactly this name. If you compare the YAML settings with this output, you can check by eye whether the chain is connected as intended.

Only `base_to_lift` stays `voltage` even in the mock. In the mock, only the four wheel joints are changed to `velocity`. The lift is position-controlled, so it does not need the integration of `GenericSystem`. Make a habit of comparing with the declaration in the real robot's `robot_hardware_real.xacro`. If the declaration and the actual interfaces disagree, you first suspect the URDF or the validation logic on the C++ side.

To send a command and see the reaction, publish `/cmd_vel`.

```bash
ros2 topic pub /cmd_vel geometry_msgs/msg/TwistStamped \
  "{header: {stamp: now, frame_id: base_link}, twist: {linear: {x: 0.1, y: 0.0, z: 0.0}, angular: {z: 0.0}}}" \
  --rate 20 --times 20
```

(Source: `docs/BRINGUP_CHECKLIST.md` L34)

Note that the type is **`TwistStamped`**, not `Twist`. `omni_wheel_drive_controller` accepts the type with a timestamp. If you send `Twist`, the subscriber's type does not match, so nothing happens, and no error appears either, which makes it hard to notice.

Because this is mock hardware, the motors do not really turn, but if you echo `/odom`, you can see the integration progress. When `linear.x: 0.3` was sent for about 4 seconds on my machine, `/odom` looked like this.

```yaml
    position:
      x: 1.8570142507553127
      y: -2.1468619681095987e-16
      z: 0.0
```

Only `x` grows and `y` is almost zero (`-2.1e-16` is floating-point error). So the whole path "cmd_vel → kinematics of the omni controller → PID chain → velocity of the mock hardware → state interface → wheel odometry" is connected. It is also good to check the period of `/joint_states`.

```
$ ros2 topic hz /joint_states
average rate: 199.998
	min: 0.005s max: 0.005s std dev: 0.00012s window: 201
```

As `update_rate: 200` says, it runs with a 5 ms period. If this number is unstable, it is a sign that the CPU is not enough for `update_rate`, or that something in read/write is blocking.

> Note: This direct publish to `/cmd_vel` is a step only for checking a controller alone with `robot.launch.py`. In `manual.launch.py` and `auto.launch.py`, `/cmd_vel` is owned by the mux (`robot_command_arbiter`), so you must not publish directly to the same topic (`docs/BRINGUP_CHECKLIST.md` L42-43). You would break the arbitration between manual commands and autonomous commands.

**Only if you have a real CAN connection**, switch to `use_mock_hardware:=false` and check that the drive base really moves with `/cmd_vel` (see section 4 of `docs/BRINGUP_CHECKLIST.md` for the steps and check items on the real robot).

**Exercise**: The gains of `wheel_pid_controller` in `mobility_controllers.yaml` have a limit, `u_clamp_max: 0.25`. Think about why it is `0.25` and not `[-1, 1]`, together with the role of `mobility_controller`.

<details markdown="1"><summary>Answer</summary>

`wheel_pid_controller` can output `voltage` as a pseudo voltage in `[-1, 1]` for the command interface. But `u_clamp_max` is cut down to `0.25`, which is a tuning that deliberately keeps the output limit of the PID alone low. If `u_clamp_max` were left at `1.0`, the PID would issue a full-power voltage command when the speed error is large, which leads to abrupt behavior and the risk of overcurrent. For the wheel velocity reference computed on the `mobility_controller` side, keeping the PID output low makes the output hard to run wild even if there is an error in the kinematics computation or the tires slip.

You can confirm the range `[-1, 1]` of the pseudo voltage in a comment in `config/hardware/mock_dynamics_overrides.yaml` ("the real ChipMD hardware requires a pseudo voltage command clamped to [-1, 1]"). Both `p: 0.04` and `u_clamp_max: 0.25` are provisional values tuned on the real robot, so read `docs/PID_TUNING.md` before touching them.

</details>

## Going further

### Test without hardware using mock_components

`robot_hardware_mock.xacro` uses `mock_components/GenericSystem` to replace ExampleChipmdWheelSystem and the others. What is interesting here is that the command interface of the wheels is changed from the real robot's `voltage` to `velocity`.

```xml
<!-- Wheels use a "velocity" command interface here instead of real
     hardware's "voltage", so mock_components/GenericSystem's
     calculate_dynamics can integrate it into position/velocity state
     (it only understands a "velocity"-named command). -->
<ros2_control name="ExampleChipmdWheelSystem" type="system">
  <hardware>
    <plugin>mock_components/GenericSystem</plugin>
    <param name="mock_sensor_commands">false</param>
    <param name="calculate_dynamics">true</param>
  </hardware>
  <joint name="front_left_wheel_joint">
    <command_interface name="velocity"/>
    <state_interface name="position"/>
    <state_interface name="velocity"/>
  </joint>
  ...
</ros2_control>
```

(Source: `src/description/robot_description/urdf/robot_hardware_mock.xacro`)

`calculate_dynamics` of `GenericSystem` only integrates a command interface named `velocity`, so with the real robot's `voltage` the joints do not turn on the mock. For this reason, a difference file, `config/hardware/mock_dynamics_overrides.yaml`, is prepared. It is loaded after `mobility_controllers.yaml` only when `use_mock_hardware:=true`. This is a realistic approach: instead of "sharing exactly the same controller settings between the real robot and the mock", only the parts that disagree are absorbed by a difference file. Using the same hardware component names for the real robot and the mock is also a trick to make this swap work without changing the descriptions in `base_controllers.yaml` or each launch.

This file overrides more than `command_interface`. This is the interesting part, so look at the contents.

```yaml
wheel_pid_controller:
  ros__parameters:
    command_interface: velocity
    enable_feedforward: true
    gains:
      front_left_wheel_joint:
        p: 0.0                  # 0.04 on the real robot
        u_clamp_max: 25.0       # 0.25 on the real robot
        feedforward_gain: 1.0
      # The other 3 wheels are the same
```

**It sets the P gain to 0 and uses only feedforward 1.0.** The reason is written in a comment in the file. `GenericSystem` mirrors the velocity command straight back into the feedback with no plant delay, so a loop with only P control oscillates alternately between the reference and zero. Also, the gains of the real robot (`p=0.04`, `u_clamp=±0.25`) are tuned so that the pseudo voltage does not saturate, so used as a velocity command they are too small to see any motion. So for the mock, the decision is that it is more natural to "pass the command straight through".

What you can learn here is that **the mock is not a place that aims to "run with the same settings as the real robot"**. The purpose of the mock is to check the launch wiring, the consistency of interfaces, and the flow of odometry. So it is the right decision to change the content of the control boldly for that purpose. Conversely, even if you tune gains on the mock, you cannot take them to the real robot.

> Column: This file has a comment saying "`rcl_yaml_param_parser` does not support YAML anchors/aliases, so the same block is repeated for each wheel without using `&`/`*`". Parameter YAML in ROS 2 is not read by a general YAML parser but by a dedicated C implementation, so convenient features such as anchors cannot be used. Copy-pasting the same block for four wheels is not laziness; it is a limit of the parser. If you actually pass a params file with anchors, it fails like this.
>
> ```
> what():  failed to initialize rcl: Couldn't parse params file: '--params-file ...'.
>          Error: Will not support aliasing at line 7
> ```
>
> `python3 -c "import yaml; yaml.safe_load(...)"` reads it without any problem, so you get stuck in the form "it is correct as YAML, but only ROS 2 does not accept it". If `Will not support aliasing` appears, this is the cause.

### Switch to the simulator with gz_ros2_control

The mock only makes "joints move numerically". It has no friction with the ground and no contact with objects. The third option is `robot_hardware_sim.xacro`, which is the Gazebo Harmonic integration using `gz_ros2_control`. It is selected with `use_sim_hardware:=true` (or `simulation.launch.py`).

```xml
<xacro:macro name="robot_hardware_sim" params="prefix enabled_hand:=all sts3215_bus_config">
  <ros2_control name="GazeboSimSystem" type="system">
    <hardware>
      <plugin>gz_ros2_control/GazeboSimSystem</plugin>
    </hardware>
    <joint name="front_left_wheel_joint">
      <command_interface name="velocity">
        <param name="min">-25.0</param>
        <param name="max">25.0</param>
      </command_interface>
      <state_interface name="position"/>
      <state_interface name="velocity"/>
      <state_interface name="effort"/>
    </joint>
    ...
  </ros2_control>

  <gazebo>
    <plugin filename="libgz_ros2_control-system"
            name="gz_ros2_control::GazeboSimROS2ControlPlugin">
      <parameters>$(find robot_bringup)/config/simulation/gazebo_controllers.yaml</parameters>
      <hold_joints>true</hold_joints>
    </plugin>
  </gazebo>
</xacro:macro>
```

(Source: `src/description/robot_description/urdf/robot_hardware_sim.xacro`)

The point to hold here is that **the three `<ros2_control>` definitions (real / mock / sim) expose the same joint names**. A comment at the top of the file also says "The controller facing wheel joint names remain identical to the real robot." Because the joint names are the same, you can move between the real robot, the mock, and Gazebo only by swapping the URDF side, without rewriting `base_controllers.yaml` or the subsystem launches. This is a real example of the idea "the URDF is a contract between hardware and software" from [19_writing_urdf](19_writing_urdf.md).

On the other hand, the sim uses a `velocity` command interface, not `voltage`, and reads a separate controller setting, `gazebo_controllers.yaml`. The physics engine of Gazebo cannot interpret a pseudo voltage, so the interface has to be swapped for the same reason as the mock.

Separately from `gz_ros2_control`, the suction of the suction cups is reproduced with a Gazebo-side plugin. `gz-sim-detachable-joint-system` is declared for each suction cup, and objects are attached and released with the topics `/simulation/suction/<channel>/attach` and `/detach`. This is an example of using a simulator-specific feature over topics, outside the frame of ros2_control.

> Note: ADR 0003 (`docs/adr/0003-do-not-vendor-sim-repo.md`) is the decision "do not make the simulator repository a submodule for now", and it is still `Accepted`. But as a result, the Gazebo integration went in the direction of **being implemented inside this repository itself**, not by pulling in another repository. The ADR's decision itself is still valid, but it does not mean "so the Gazebo integration is not implemented". When you read an ADR, always compare the date of the decision with the current state of the repository.

## Conclusion

Writing your own hardware_interface is the most laborious part of ros2_control. But once you have written it, the controller side (standard controllers such as `diff_drive_controller` and `pid_controller`) can be used in a swappable form. The reason this repository integrated all four systems (wheel/lift/hand/solenoid) into ros2_control is also this maintainability: "if you write only the hardware_interface, the rest rides on the common mechanism". If you do not understand something, ask someone experienced or check the official documentation.

Next is [21_sensor_integration](21_sensor_integration.md), which covers how to bring LiDAR and IMU into ROS 2.

## References

- [ros2_control official docs (control.ros.org): Writing a Hardware Component](https://control.ros.org/jazzy/doc/ros2_control/hardware_interface/doc/writing_new_hardware_component.html)
- [ros2_control official docs (control.ros.org) top](https://control.ros.org/jazzy/index.html)
- [20_ros2_control_overview](20_ros2_control_overview.md)
- [21_sensor_integration](21_sensor_integration.md)
