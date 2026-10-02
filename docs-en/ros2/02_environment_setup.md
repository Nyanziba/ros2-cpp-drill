# ROS 2 Lecture 02: Environment Setup (Ubuntu 24.04 + ROS 2 Jazzy)

Previous: [01_start_here_course_hub](01_start_here_course_hub.md)

## Introduction

When you finish this article, ROS 2 Jazzy Jalisco will run on your PC (or WSL2), and you will have checked that the bundled sample nodes can talk to each other. ROS 2 depends strongly on the OS, so if you get stuck here, all the later lectures stop. Do not rush. Go one step at a time and check as you go.

## Lecture goals / how to proceed

- Audience: people touching ROS 2 for the first time
- Prerequisites: basic Linux command-line use (`cd`, `ls`, `sudo`, and so on)
- Time needed: about 30 minutes if you already have the environment, 1 to 2 hours (including download time) if you start from installing Ubuntu
- How to proceed: run the steps in the text from the top, and finish the last check alone. If you get stuck, look at "Common pitfalls" first, and if it is still not fixed, ask someone experienced or check the official documentation.

## Using this as a course

### Things to prepare

- A USB drive with the Ubuntu 24.04 installer, or a Windows machine for WSL2 (check beforehand whether the PC you will use can handle it)
- An internet connection (`apt install` downloads close to several GB. Avoid times when your Wi-Fi is slow)
- Run through the same steps once on your own PC for checking. Steps sometimes change when versions go up

### Oral exam (with model answers)

Q1. I opened a new terminal, and the `ros2` command suddenly says "command not found". Why?

Model answer: You did not add the source line to `.bashrc`, or you added it but did not run `source ~/.bashrc` (a new terminal should read it automatically, so it is likely that you forgot to add it at all). It also happens when you added it to another file such as `.bash_profile` instead of `.bashrc`.

Q2. `talker` is running, but the messages do not reach `listener` in another terminal. What do you suspect?

Model answer: First, check that `source /opt/ros/jazzy/setup.bash` was done in both terminals. Next, check that `ROS_DOMAIN_ID` has the same value in both terminals (if you set the environment variable in only one, they are in different domains and do not communicate). Also include whether the network is reachable (on the same PC it is basically fine, but with WSL2 or Docker, network settings may be involved).

Q3. Give one advantage and one disadvantage of using ROS 2 on WSL2.

Model answer: The advantage is that you can use ROS 2 for Linux while keeping the Windows environment. The disadvantage is that GUI tools (rqt, rviz2, Gazebo, and so on) tend to display unstably, and network settings are more likely to cause trouble than on native Linux.

### Time allocation guide

- Preparing the Ubuntu environment: 0 to 60 minutes (0 minutes if you already have one, about 60 minutes for a new install)
- Installing ROS 2: 15 minutes (you can copy and paste the commands, but there is waiting for downloads)
- Checking that it works: 5 minutes
- Helping people who are stuck: depends on the situation. GPG key errors and WSL2 GUI problems often need individual help, so allow extra time

## Main content

### Task 1: Prepare Ubuntu 24.04

What you learn:
- ROS 2 is OS-dependent software
- What native Linux, WSL2, and Docker are each good and bad at

Preparation:
- A USB drive for installation, or a Windows machine (for WSL2)

Content:

The OS supported by ROS 2 Jazzy Jalisco is Ubuntu 24.04 (Noble Numbat). First, prepare your environment in the following order of priority.

1. **Native Ubuntu 24.04 (recommended)**. Install it directly on the PC, or set up dual boot. GUI tools (such as rviz2 and Gazebo) run most stably. In robot-competition debugging on real hardware, native is often required in the end, so it is a good idea to get used to it early.
2. **Ubuntu 24.04 on WSL2 (second choice)**. For people who want to keep using Windows. You can develop on the command line without problems, but the GUI display (WSLg) is sometimes unstable. See "Common pitfalls" below for details.
3. **Docker (for Mac)**. macOS cannot install ROS 2 directly, so you work in an Ubuntu 24.04 Docker container. You need extra X11 forwarding settings, so for lectures that use the GUI a lot, borrowing a Linux machine can be faster.

Hint: Installing native Ubuntu is easy if you follow the official Ubuntu installer. If you are unsure about partitioning, ask someone experienced to check.

### Task 2: Install ROS 2 Jazzy with apt

What you learn:
- Why the locale setting is needed
- What the Ubuntu universe repository is
- How the apt GPG key and the official ROS 2 repository registration work

Preparation:
- An Ubuntu 24.04 environment (native, WSL2, or Docker) is running
- An internet connection

Content:

We follow the official Debian package steps. Open a terminal and run the following in order.

First, check and set the locale. ROS 2 assumes a UTF-8 locale, so if it is broken, you get odd behavior such as in log output.

```bash
locale  # Check. See whether LANG and the LC_ALL group are UTF-8

sudo apt update && sudo apt install locales
sudo locale-gen en_US en_US.UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
export LANG=en_US.UTF-8

locale  # Check again
```

Next, enable the Ubuntu universe repository. Some of the dependency packages needed to install ROS 2 are in it.

```bash
sudo apt install software-properties-common
sudo add-apt-repository universe
```

Then register the official ROS 2 apt repository. From Jazzy on, the method that uses the `ros2-apt-source` package is recommended, so we follow it.

```bash
sudo apt update && sudo apt install curl -y

export ROS_APT_SOURCE_VERSION=$(curl -s https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest | grep -F "tag_name" | awk -F\" '{print $4}')
curl -L -o /tmp/ros2-apt-source.deb "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.$(. /etc/os-release && echo $VERSION_CODENAME)_all.deb"
sudo apt install /tmp/ros2-apt-source.deb
```

This package sets up both the GPG key and the apt source list. The old method of adding the key to `apt-key` by hand is deprecated now, so do not copy the steps of old articles and blogs.

Finally, update the package list and install ROS 2 itself (the Desktop version).

```bash
sudo apt update
sudo apt upgrade  # Upgrade here to avoid dependency mismatches with existing packages

sudo apt install ros-jazzy-desktop
```

`ros-jazzy-desktop` includes rviz2, turtlesim, and the demo nodes. It uses several GB of disk space, so be careful. If you want development tools (such as colcon), install the following too.

```bash
sudo apt install ros-dev-tools
```

Hint: If `apt install` seems not to finish for a long time, the most likely cause is just a slow connection. Even if there is no response, wait a few minutes before you decide.

### Task 3: Add source to ~/.bashrc

What you learn:
- How ROS 2 environment variables are loaded by sourcing in the shell
- The fact that "nothing works if you forget to source", which is the basis for the idea of overlay/underlay

Preparation:
- `ros-jazzy-desktop` has been installed in Task 2

Content:

The ROS 2 command (`ros2`) and the package paths are set as shell environment variables. So you cannot use it just by installing it. You need to load (source) `/opt/ros/jazzy/setup.bash`.

Typing it every time is inefficient, so add it to `~/.bashrc`. Then it is sourced automatically every time you open a new terminal.

```bash
echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

Note that we use `echo` and `>>` (append, not overwrite). A single `>` erases the contents of `.bashrc`. **We never want to destroy a `.bashrc` by accident, so get into the habit of checking the current contents with `cat ~/.bashrc` before you append.**

To check that the line was added, open a new terminal and run the following.

```bash
echo $ROS_DISTRO
# If it shows jazzy, you are OK
```

### Task 4: Check that it works with talker/listener

What you learn:
- See with your own eyes how communication between nodes (DDS) actually works
- A basic feel for operating ROS 2 with several terminals

Preparation:
- You have finished up to Task 3
- You can open two terminals (tabs or windows are both fine)

Content:

ROS 2 comes with demo pub/sub nodes. Prepare two terminals and check.

In the first terminal, start the sender (talker).

```bash
ros2 run demo_nodes_cpp talker
```

If logs like `Publishing: 'Hello World: 1'` start to flow every second, the start was successful.

In the second terminal, start the receiver (listener).

```bash
ros2 run demo_nodes_cpp listener
```

If you see output that matches the talker's log, such as `I heard: [Hello World: 1]`, communication succeeded. When you have confirmed this, Task 4 is cleared. End both terminals with `Ctrl+C`.

### Task: talker/listener works in both terminals

This is the goal of this article. When you have confirmed that the logs above flow in both terminals without problems, report to whoever is teaching you, if there is someone.

<details markdown="1"><summary>Answer (steps to check if you are stuck)</summary>

1. In both terminals, check that `echo $ROS_DISTRO` shows `jazzy` (you may not have sourced)
2. Look carefully at the log in the first terminal to see whether `ros2 run demo_nodes_cpp talker` started without errors
3. Run `ros2 topic list` and check that `/chatter` appears (it should appear if talker is running)
4. If all of the above is fine, check that `ROS_DOMAIN_ID` is the same in both terminals (see the Advanced section below)

</details>

## Common pitfalls

- **Forgetting to source**: This is the most frequent trouble. "It worked until a moment ago, but `ros2` does not work in a new terminal" is almost always this. Check again what you added to `.bashrc`.
- **GPG key errors**: If you register the key with the old steps (such as `apt-key add`) without using the `ros2-apt-source` package, you get deprecation warnings or failures on Ubuntu 24.04. Use the steps in this article (installing `ros2-apt-source_*.deb`).
- **WSL2 GUI**: If the rviz2 or turtlesim windows do not appear or are all black, it may be a WSLg bug. You may need to update WSL on the Windows side or update the GPU driver. This course is mostly command-line, so the effect is small, but it matters in later articles when you use rqt or rviz2.

## Advanced

### About ROS_DOMAIN_ID

By default, `ros2 topic list` and `talker`/`listener` communicate as everyone on the same network belonging to the same "domain". So if several people start `talker` at the same time on the same Wi-Fi, your nodes may appear mixed with each other.

To avoid this, you can separate domains with the environment variable `ROS_DOMAIN_ID`.

```bash
export ROS_DOMAIN_ID=42  # Choose any number in the range 0 to 101
```

If you write this in `.bashrc`, all nodes you start on your PC belong to this domain number, and they no longer mix with other nodes. Note that the value must be the same among nodes on the same PC and on the same robot.

This is important in real operation. When several robots are on the same network, if you forget to match `ROS_DOMAIN_ID`, communication gets mixed up and leads to unexpected behavior. When you run several robots, use a different domain number for each, or keep the test machine and the robots separated.

## Summary

Now you have an environment where ROS 2 Jazzy runs. In the next article, you use a simulator called turtlesim to touch what nodes and topics are, and get a feel for them. If any part still does not work, ask someone experienced or check the official documentation before you move on.

Next: [03_getting_a_feel_with_turtlesim_and_rqt](03_getting_a_feel_with_turtlesim_and_rqt.md)

## References

- [Ubuntu (deb packages) — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)
- [Configuring ROS 2 environment — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Configuring-ROS2-Environment.html)
- [Understanding ROS 2 nodes (source of the talker/listener demo) — ROS 2 Documentation: Jazzy](https://docs.ros.org/en/jazzy/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Nodes/Understanding-ROS2-Nodes.html)
