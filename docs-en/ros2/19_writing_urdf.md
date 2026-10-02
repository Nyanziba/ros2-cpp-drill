# ROS 2 Lecture 19: Writing URDF

## Introduction

After this article, you will be able to write the URDF of a two-wheeled cart yourself and display your own robot in RViz2.

The prerequisite is that you have finished reading [18_tf2_and_coordinate_frames](18_tf2_and_coordinate_frames.md). URDF is itself the input that automatically generates the robot's TF tree, so if you write URDF without knowing how TF works, you lose track of "what am I writing this for".

## Goals

- Explain in your own words why URDF is needed (automatic generation of the TF tree, visualization, and the premise of ros2_control)
- Write the three elements of a link (visual/collision/inertial) and the main joint types (fixed/continuous/revolute/prismatic)
- Write a minimal two-wheeled cart URDF yourself, and display it in RViz2 with robot_state_publisher and joint_state_publisher_gui
- Rewrite it with xacro, using properties and macros

## Using this as a lecture

### What to prepare

- An environment with Ubuntu 24.04 + ROS 2 Jazzy already set up ([02_environment_setup](02_environment_setup.md) finished)
- `ros-jazzy-joint-state-publisher-gui` and `ros-jazzy-xacro` (if they are not installed, run `sudo apt install ros-jazzy-joint-state-publisher-gui ros-jazzy-xacro`)
- An environment where RViz2 can start (a GUI must appear. For attendees over SSH, check X forwarding or VNC beforehand)
- A workspace for practice (finish [10_workspaces_and_colcon](10_workspaces_and_colcon.md) first)

### Suggested time plan

- Explain the purpose of URDF and its relation to TF: 10 min
- Explain the link/joint syntax: 15 min
- Exercise: write the two-wheeled cart URDF: 25 min
- robot_state_publisher + RViz2 display: 10 min
- Convert to xacro: 15 min
- Oral questions: 10 min

### Oral questions

**Q1. If you do not write a URDF and do not register the robot model in ROS 2, what can you no longer do?**

Model answer: The TF tree is not generated automatically, so you have to publish all the transformations from `base_link` to the sensors and wheels by hand. RViz2 cannot show the shape of the robot, and while debugging you cannot see "which way a sensor is facing". ros2_control builds the hardware interface on the premise of the `<joint>` information in the URDF, so without a URDF you cannot even load the controllers.

**Q2. What are visual, collision, and inertial inside `<link>` for? If you may omit only one, which one is it?**

Model answer: visual is for the appearance (drawing in RViz2 and Gazebo), collision is for collision detection (the navigation costmap and Gazebo's physics), and inertial is for mass and moment of inertia (the dynamics calculation of physics simulation). If you only run a real robot and do not run simulation, it works even if you omit inertial (some ros2_control plugins may print a warning, but it is not fatal). But if you run physics simulation in Gazebo, inertial is required.

**Q3. What is the difference between `continuous` and `revolute`? Which one should you use for a drive wheel?**

Model answer: `revolute` requires upper and lower bounds in `<limit>`, and it can rotate only within that range. `continuous` can rotate without limit. A drive wheel keeps going forward, so its rotation angle has no upper bound, and you use `continuous`. For something with a fixed range of motion, such as an arm joint, you use `revolute`.

## Main text

### What you learn: What is URDF written for

What you learn: understand that URDF is the premise of the TF tree, visualization, and ros2_control.

Preparation: you must have finished reading [18_tf2_and_coordinate_frames](18_tf2_and_coordinate_frames.md).

Details:

URDF (Unified Robot Description Format) is a file that describes the shape and joint structure of a robot in XML. It holds only the information "this robot has these parts (links), and they are connected by these joints", and it does not move anything by itself.

When you let `robot_state_publisher` read this URDF, the transformations (TF) between links are computed automatically from the joint information and published. In [18_tf2_and_coordinate_frames](18_tf2_and_coordinate_frames.md) we explained that "TF does not exist unless someone computes and publishes it". But computing by yourself every time the TF between fixed parts or between parts connected by joints is not realistic. If you write a URDF, you can leave this computation to ROS 2.

There are three things for which URDF is the premise.

- **TF tree**: `robot_state_publisher` generates TF automatically from the joint structure of the URDF
- **Visualization**: RViz2 reads the URDF and draws a 3D model of the robot. Even if TF is correct, without the shapes you only get a stick-figure display
- **ros2_control**: covered in [20_ros2_control_overview](20_ros2_control_overview.md). ros2_control reads the `<joint>` and `<ros2_control>` tags of the URDF and builds the hardware interface. If the joint names or ranges of motion in the URDF are wrong, the controllers do not work correctly

So URDF is not "decoration for the looks". It is the common input data that all the lectures from here on read.

### What you learn: The syntax of link

What you learn: be able to write visual/collision/inertial inside `<link>`.

Preparation: nothing in particular.

Details:

`<link>` represents one rigid part of the robot. Its content is divided into three parts.

```xml
<link name="base_link">
  <visual>
    <geometry>
      <box size="0.3 0.2 0.1"/>
    </geometry>
    <origin xyz="0 0 0" rpy="0 0 0"/>
    <material name="blue">
      <color rgba="0 0 0.8 1"/>
    </material>
  </visual>
  <collision>
    <geometry>
      <box size="0.3 0.2 0.1"/>
    </geometry>
    <origin xyz="0 0 0" rpy="0 0 0"/>
  </collision>
  <inertial>
    <mass value="1.0"/>
    <inertia ixx="0.01" ixy="0.0" ixz="0.0" iyy="0.01" iyz="0.0" izz="0.01"/>
  </inertial>
</link>
```

- `visual`: the appearance. Specify one of `box`/`cylinder`/`sphere`/`mesh` in `geometry`
- `collision`: the shape used for collision detection. You often give the same shape as visual, but you may also use a simplified shape (a cylinder or a box instead of a complex mesh) to make the computation lighter
- `inertial`: mass and moment of inertia. Used for the physics calculation of simulation

`origin` specifies position and orientation with `xyz` (translation, in meters) and `rpy` (roll-pitch-yaw, in **radians**). It is radians, not degrees, so if you want to rotate by 90 degrees you must write `1.5708`. Writing degrees here by mistake is very common, so get into the habit of checking with a calculator or `python3 -c "import math; print(math.pi/2)"`.

> Column: When you use `mesh`, specify it with the `package://` scheme, such as `<mesh filename="package://package_name/meshes/base.stl"/>`. If you write an absolute path or a relative path directly, the file is not found in another person's environment or in the install location after the build, and you get a load error. The same error appears when the package name has a typo, or when `package.xml` has no install setting for the meshes directory, so check both when you get the error.

### What you learn: The syntax of joint

What you learn: be able to write `<joint>`, which connects a link to a link.

Preparation: nothing in particular.

Details:

With links alone, the parts only float apart. You define the parent-child relation and the way of connection with `<joint>`.

```xml
<joint name="wheel_left_joint" type="continuous">
  <parent link="base_link"/>
  <child link="wheel_left_link"/>
  <origin xyz="0.0 0.15 -0.05" rpy="-1.5708 0 0"/>
  <axis xyz="0 0 1"/>
</joint>
```

- `parent`/`child`: which link is connected to which link. The `parent` specified here becomes the parent frame
- `origin`: the position and orientation of the origin of the child link, as seen from the coordinate frame of the parent link. **The origin of a joint decides "the initial position of the child"; it does not mean the joint moves around this point** (the rotation axis is decided by `axis` below)
- `axis`: the axis direction of rotation or translation (not needed when `type` is `fixed`)

`type` is mainly one of the following six.

| type | Meaning |
|---|---|
| fixed | Fixed. Does not move relative to the parent |
| continuous | Rotates around the axis without limit |
| revolute | Rotates around the axis, but has upper and lower limits in `<limit>` |
| prismatic | Slides along the axis direction |
| floating | Moves freely with 6 degrees of freedom (rotation + translation) |
| planar | Translates within a plane |

`revolute` and `prismatic` require `<limit>`.

```xml
<joint name="arm_joint" type="revolute">
  <parent link="base_link"/>
  <child link="arm_link"/>
  <origin xyz="0 0 0.1" rpy="0 0 0"/>
  <axis xyz="0 1 0"/>
  <limit lower="-1.57" upper="1.57" effort="10.0" velocity="1.0"/>
</joint>
```

`lower`/`upper` are in radians (in meters for `prismatic`), `effort` is the maximum torque (N·m), and `velocity` is the maximum angular velocity (rad/s). A drive wheel keeps rotating without limit, so use `continuous` and do not write `<limit>`.

### What you learn: Writing a minimal two-wheeled cart URDF

What you learn: combine the syntax so far to make one robot model that moves (is visible).

Preparation: create a `description` package in the workspace.

```bash
cd ~/ros2_ws/src
ros2 pkg create --build-type ament_cmake my_robot_description
mkdir my_robot_description/urdf
```

Details:

Create `my_robot_description/urdf/my_robot.urdf`. It has the cart body and the left and right wheels, as three links (the caster is added in the practice problem below).

```xml
<?xml version="1.0"?>
<robot name="my_robot">

  <link name="base_link">
    <visual>
      <geometry>
        <box size="0.3 0.2 0.1"/>
      </geometry>
      <origin xyz="0 0 0" rpy="0 0 0"/>
      <material name="blue">
        <color rgba="0 0 0.8 1"/>
      </material>
    </visual>
    <collision>
      <geometry>
        <box size="0.3 0.2 0.1"/>
      </geometry>
      <origin xyz="0 0 0" rpy="0 0 0"/>
    </collision>
    <inertial>
      <mass value="1.0"/>
      <inertia ixx="0.01" ixy="0.0" ixz="0.0" iyy="0.01" iyz="0.0" izz="0.01"/>
    </inertial>
  </link>

  <link name="wheel_left_link">
    <visual>
      <geometry>
        <cylinder radius="0.05" length="0.02"/>
      </geometry>
      <material name="black">
        <color rgba="0.1 0.1 0.1 1"/>
      </material>
    </visual>
    <collision>
      <geometry>
        <cylinder radius="0.05" length="0.02"/>
      </geometry>
    </collision>
    <inertial>
      <mass value="0.1"/>
      <inertia ixx="1e-4" ixy="0.0" ixz="0.0" iyy="1e-4" iyz="0.0" izz="1e-4"/>
    </inertial>
  </link>

  <link name="wheel_right_link">
    <visual>
      <geometry>
        <cylinder radius="0.05" length="0.02"/>
      </geometry>
      <material name="black">
        <color rgba="0.1 0.1 0.1 1"/>
      </material>
    </visual>
    <collision>
      <geometry>
        <cylinder radius="0.05" length="0.02"/>
      </geometry>
    </collision>
    <inertial>
      <mass value="0.1"/>
      <inertia ixx="1e-4" ixy="0.0" ixz="0.0" iyy="1e-4" iyz="0.0" izz="1e-4"/>
    </inertial>
  </link>

  <joint name="wheel_left_joint" type="continuous">
    <parent link="base_link"/>
    <child link="wheel_left_link"/>
    <origin xyz="0.0 0.15 -0.05" rpy="-1.5708 0 0"/>
    <axis xyz="0 0 1"/>
  </joint>

  <joint name="wheel_right_joint" type="continuous">
    <parent link="base_link"/>
    <child link="wheel_right_link"/>
    <origin xyz="0.0 -0.15 -0.05" rpy="-1.5708 0 0"/>
    <axis xyz="0 0 1"/>
  </joint>

</robot>
```

The `rpy="-1.5708 0 0"` in the wheel `origin` is for rotating the standard orientation of a cylinder (axis along Z) by 90 degrees so that its axis points along the Y axis (the left-right direction). If you omit it, you always run into the trouble that the wheel faces the front and does not turn, or is drawn in a strange direction.

**Practice problem**: Add a caster (`caster_link`, a sphere, fixed with a `fixed` joint at the front and below of `base_link`) to the URDF above.

<details markdown="1"><summary>Answer</summary>

```xml
<link name="caster_link">
  <visual>
    <geometry>
      <sphere radius="0.03"/>
    </geometry>
    <material name="grey">
      <color rgba="0.5 0.5 0.5 1"/>
    </material>
  </visual>
  <collision>
    <geometry>
      <sphere radius="0.03"/>
    </geometry>
  </collision>
  <inertial>
    <mass value="0.05"/>
    <inertia ixx="1e-5" ixy="0.0" ixz="0.0" iyy="1e-5" iyz="0.0" izz="1e-5"/>
  </inertial>
</link>

<joint name="caster_joint" type="fixed">
  <parent link="base_link"/>
  <child link="caster_link"/>
  <origin xyz="0.12 0 -0.07" rpy="0 0 0"/>
</joint>
```

The caster is not driven, so `fixed` is enough. A real caster rolls freely, but reproducing even that degree of freedom in simulation makes things complicated, so simple models often just use `fixed`.

</details>

### What you learn: Displaying it in RViz2

What you learn: visualize your own URDF with `robot_state_publisher` and `joint_state_publisher_gui`.

Preparation: you must have finished writing `my_robot.urdf`.

Details:

First, pass the URDF to `robot_state_publisher` and start it.

```bash
ros2 run robot_state_publisher robot_state_publisher --ros-args -p robot_description:="$(xacro ~/ros2_ws/src/my_robot_description/urdf/my_robot.urdf)"
```

It is fine to run a URDF that is pure XML (without xacro) through the `xacro` command too. A plain URDF is also a valid input for xacro.

When there are `continuous`/`revolute`/`prismatic` joints, TF is not complete unless someone publishes those joint angles. For the exercise, use `joint_state_publisher_gui` to move sliders and send dummy values.

```bash
ros2 run joint_state_publisher_gui joint_state_publisher_gui
```

Finally, start RViz2, set `Fixed Frame` to `base_link`, and add a `RobotModel` display. `Description Topic` can stay `/robot_description`.

```bash
rviz2
```

Move the wheel sliders, and check that only the wheels rotate in RViz2. The correct behavior is that the cart body does not move, and only the wheels turn around the axes of `wheel_left_joint`/`wheel_right_joint`.

Hint: if the RobotModel is not shown, it is often a mistake in `Fixed Frame`, or `Description Topic` does not match the name of the topic that is really published. Check with `ros2 topic list` that `/robot_description` appears.

> Column: `joint_state_publisher_gui` is only for exercises and debugging. On a real robot, the `ros2_control` controllers covered in [20_ros2_control_overview](20_ros2_control_overview.md) publish `/joint_states` from the real encoder values, so you do not use the GUI.

### What you learn: Using variables and macros with xacro

What you learn: gather the dimensions into properties and turn repeated parts into a macro.

Preparation: nothing in particular.

Details:

In the URDF above, the links and joints of the wheels were almost the same content repeated. In addition, dimensions such as the wheel radius and the tread (the distance between the left and right wheels) were hard-coded in several places, and when you change them later, you have to find and fix all of them. We solve this with xacro. Use the extension `.urdf.xacro`.

```xml
<?xml version="1.0"?>
<robot name="my_robot" xmlns:xacro="http://www.ros.org/wiki/xacro">

  <xacro:property name="wheel_radius" value="0.05"/>
  <xacro:property name="wheel_length" value="0.02"/>
  <xacro:property name="tread" value="0.3"/>

  <link name="base_link">
    <visual>
      <geometry>
        <box size="0.3 0.2 0.1"/>
      </geometry>
      <material name="blue">
        <color rgba="0 0 0.8 1"/>
      </material>
    </visual>
    <collision>
      <geometry>
        <box size="0.3 0.2 0.1"/>
      </geometry>
    </collision>
    <inertial>
      <mass value="1.0"/>
      <inertia ixx="0.01" ixy="0.0" ixz="0.0" iyy="0.01" iyz="0.0" izz="0.01"/>
    </inertial>
  </link>

  <xacro:macro name="wheel" params="prefix reflect">
    <link name="wheel_${prefix}_link">
      <visual>
        <geometry>
          <cylinder radius="${wheel_radius}" length="${wheel_length}"/>
        </geometry>
        <material name="black">
          <color rgba="0.1 0.1 0.1 1"/>
        </material>
      </visual>
      <collision>
        <geometry>
          <cylinder radius="${wheel_radius}" length="${wheel_length}"/>
        </geometry>
      </collision>
      <inertial>
        <mass value="0.1"/>
        <inertia ixx="1e-4" ixy="0.0" ixz="0.0" iyy="1e-4" iyz="0.0" izz="1e-4"/>
      </inertial>
    </link>

    <joint name="wheel_${prefix}_joint" type="continuous">
      <parent link="base_link"/>
      <child link="wheel_${prefix}_link"/>
      <origin xyz="0.0 ${reflect * tread / 2} ${-wheel_radius}" rpy="-1.5708 0 0"/>
      <axis xyz="0 0 1"/>
    </joint>
  </xacro:macro>

  <xacro:wheel prefix="left" reflect="1"/>
  <xacro:wheel prefix="right" reflect="-1"/>

</robot>
```

A value declared with `<xacro:property>` can be referred to like `${wheel_radius}`. When you want to change the tread or the wheel radius to match the real robot of a robot contest, you only fix these three lines and it is reflected in all places.

`<xacro:macro>` gathers the description that was almost identical for the left and right wheels into one. With `prefix` and `reflect` passed in `params`, it switches the link names (`wheel_left_link`/`wheel_right_link`) and the sign of the Y offset. The caller needs only two lines such as `<xacro:wheel prefix="left" reflect="1"/>`, and you no longer need to write every link by hand each time you change a dimension or add a wheel.

If you want to expand a xacro file and check the pure URDF, you can see it with the following command.

```bash
xacro my_robot.urdf.xacro > /tmp/my_robot_expanded.urdf
```

When you pass it to `robot_state_publisher`, either the expanded URDF or the `.xacro` file as it is will do. Both work as long as you run it through the `xacro` command.

> Column: When you write URDF, it is efficient to work while checking the arguments of each tag in the reference material.

## Going further

xacro can also use if branches (`<xacro:if>`) and expressions (arithmetic inside `${}`), so you can do things such as switching the model between simulation and the real robot. There is also a way to generate a URDF with meshes automatically from a SolidWorks or Fusion360 CAD model, with tools such as `sw_urdf_exporter` and `fusion2urdf`. Computing the inertia tensor of every link by hand is hard, so the realistic way is to take the values from the mass properties tool of the CAD, or let the exporter compute them. Here we only mention the names, and leave a deeper look to another article.


## Conclusion

URDF is the file that becomes the input of all the lectures from here on. The tread and the wheel radius in particular are numbers that both the differential drive controller in [20_ros2_control_overview](20_ros2_control_overview.md) and the odometry calculation in [22_thinking_about_localization](22_thinking_about_localization.md) use directly, so measure the dimensions of the real robot accurately before you enter them. If it is wrong by even 1 mm here, the odometry keeps drifting and hurts the accuracy of localization. If anything is unclear, ask someone experienced or check the official documentation.

Next, in [20_ros2_control_overview](20_ros2_control_overview.md), we cover the mechanism that uses this URDF to actually move motors.

## References

- [ROS 2 Documentation: Jazzy — URDF Main](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/URDF/URDF-Main.html)
- [ROS 2 Documentation: Jazzy — Using Xacro to Clean Up a URDF File](https://docs.ros.org/en/jazzy/Tutorials/Intermediate/URDF/Using-Xacro-to-Clean-Up-a-URDF-File.html)
- [18_tf2_and_coordinate_frames](18_tf2_and_coordinate_frames.md)
- [20_ros2_control_overview](20_ros2_control_overview.md)
