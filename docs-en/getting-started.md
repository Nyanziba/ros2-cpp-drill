# Getting started

You can read the chapters alone, but **you only learn by solving the exercises.**
This page shows the steps from setting up the environment to solving the first exercise.

The exercises are fill-in-the-blank. Fill in the `TODO` in the source, and the tests decide pass or fail.
You do not need anyone to grade your work.

## Which way to install

| | Best for | Effort |
| --- | --- | --- |
| **Docker** | Anything other than Ubuntu (macOS / Windows / other distributions), or if you do not want to change your system | One `docker compose build` (10 to 20 minutes) |
| **Install directly** | You use Ubuntu 24.04 and will also install ROS 2 | You need to install ROS 2 Jazzy |

ROS 2 Jazzy has official packages **only for Ubuntu 24.04.**
For any other OS, choose Docker.

## Run with Docker

Install [Docker](https://docs.docker.com/get-started/get-docker/), then
clone this repository.

<!-- only: jazzy -->
```bash
git clone https://github.com/Nyanziba/ros2-cpp-drill.git
cd ros2-cpp-drill
docker compose build          # first time only. Takes 10 to 20 minutes
```
<!-- /only -->
<!-- only: lyrical -->
```bash
git clone https://github.com/Nyanziba/ros2-cpp-drill.git
cd ros2-cpp-drill
ROS_DISTRO=lyrical docker compose build          # first time only
```

**For the Lyrical version, put `ROS_DISTRO=lyrical` in front of every command that uses `docker compose`.**
Without it, the Jazzy image is used. In Windows PowerShell, run `$env:ROS_DISTRO = "lyrical"` once first.
<!-- /only -->

Check that it works.

<!-- only: jazzy -->
```bash
docker compose run --rm drill ./drill list
```
<!-- /only -->
<!-- only: lyrical -->
```bash
ROS_DISTRO=lyrical docker compose run --rm drill ./drill list
```
<!-- /only -->

If you see the list of exercises and your progress, it worked.

To get English output from drill and from the test failure messages, set `DRILL_LANG=en`, for example `DRILL_LANG=en docker compose run --rm drill ./drill list` (with a direct install, `DRILL_LANG=en ./drill list`).

**The container uses the source files on the host as they are.** Edit them in your usual editor.

<!-- only: jazzy -->
```bash
docker compose run --rm drill ./drill watch cppb01
```
<!-- /only -->
<!-- only: lyrical -->
```bash
ROS_DISTRO=lyrical docker compose run --rm drill ./drill watch cppb01
```
<!-- /only -->

This re-runs the tests every time you save.

!!! note "If your UID on Linux is not 1000"

    Create a `.env` file first. If you do not, files made by the container
    are owned by root, and you cannot save them in an editor on the host.

    ```bash
    printf 'DRILL_UID=%s\nDRILL_GID=%s\n' "$(id -u)" "$(id -g)" > .env
    ```

    Docker Desktop on macOS and Windows matches ownership automatically, so you do not need this.

## Install directly (Ubuntu 24.04)

<!-- only: jazzy -->
Follow the [official ROS 2 Jazzy instructions](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)
to install it, then do this.
<!-- /only -->
<!-- only: lyrical -->
Follow the [official ROS 2 Lyrical instructions](https://docs.ros.org/en/lyrical/Get-Started/Installation/Ubuntu-Install-Debs.html)
to install it, then do this.
<!-- /only -->

```bash
git clone https://github.com/Nyanziba/ros2-cpp-drill.git
cd ros2-cpp-drill
source /opt/ros/jazzy/setup.bash   # drill also finds it automatically if you do not source it
./drill list
```

If the packages that the exercises depend on are missing, install them.

<!-- only: jazzy -->
```bash
sudo apt install ros-jazzy-example-interfaces ros-jazzy-action-tutorials-interfaces
```
<!-- /only -->
<!-- only: lyrical -->
```bash
sudo apt install ros-lyrical-ament-cmake-gtest ros-lyrical-ament-cmake-pytest ros-lyrical-class-loader ros-lyrical-example-interfaces ros-lyrical-rclcpp-action ros-lyrical-rclcpp-components ros-lyrical-std-msgs
```
<!-- /only -->

## Solve the first exercise

**Start with `cppb01`.** It does not use ROS 2. It needs only `g++` and gtest,
so it builds in 2 to 3 seconds.

```bash
./drill watch cppb01
```

In another terminal (or your editor), open `exercises/cppb01_declarations/src/reader.cpp`,
fill in the `TODO`, and save. When the tests pass, delete this line near the top of the file
to move on to the next exercise.

```cpp
// I AM NOT DONE
```

If you get stuck, you can get hints. They go step by step, and the last one shows the shape of the code.

```bash
./drill hint cppb01
./drill solution cppb01   # sample solution
./drill reset cppb01      # return to the initial state
```

## When the editor shows red squiggles

When you open an exercise in VS Code, you may see this.

<!-- measure: env=static reason="a VS Code IntelliSense message, not command output, so it cannot be produced in Docker" -->
```
#include errors detected. Please update your includePath.
Squiggles are disabled for this translation unit.
```

**The repository includes `.vscode/`, so this should not appear if you opened the repository folder.**
It does not work if you opened only a subfolder, so open the top of the repository.

If `/opt/ros/jazzy` is not on the host (anything other than Ubuntu), the headers do not exist at all.
Run **"Dev Containers: Reopen in Container"** from the command palette.
This opens the folder inside the container and fixes it.

Only the headers generated from your own `.msg` / `.srv` files **keep the red squiggles until you build once.**
This is because they exist only in `install/`. Run `./drill run 03` once and they go away.

## Moving between the reading and the exercises

The reading (this site) and the exercises **match one to one, down to the chapter number.**

```bash
./drill read cppb01          # opens the matching chapter in less
./drill read --web cppb01    # opens the matching page of this site in a browser
```

To go the other way, follow "Matching exercise" at the end of each chapter.

**Read first, then solve.** The "Try it yourself" section in each chapter is written
on the assumption that you write down your prediction and then run it. The answers are folded,
so make your prediction before you open them.

## Command list

| Command | Description |
| --- | --- |
| `./drill list` | List of exercises and progress |
| `./drill run [ID]` | Build and test (if you omit ID, the next unfinished exercise) |
| `./drill watch [ID]` | Detect saves and re-run the tests automatically |
| `./drill hint ID` | Hints |
| `./drill doc ID` | URL of the matching official documentation |
| `./drill read [ID]` | Open the matching chapter (`--web` for the browser) |
| `./drill solution ID` | Sample solution |
| `./drill reset ID` | Return the exercise to its initial state |
| `./drill verify` | Test all exercises in order |

IDs are told apart by prefix. **`cppb` = C++ Basics, `cpp` = C++, digits only = ROS 2.**
Type them like `./drill run cppb06` / `./drill run cpp06` / `./drill run 01`.
A partial match such as `./drill run publisher` also works.

## In what order to go

```
cppb01 to cppb10  →  cpp01 to cpp12  →  01 to 15
(C++ Basics)         (C++)              (ROS 2)
```

**The order is designed to finish C++ first.** rclcpp depends heavily on `shared_ptr`, lambdas,
`std::move`, and templates. If you start ROS 2 while these are still unclear,
you cannot tell whether you are learning ROS or fighting the C++ syntax.

If you are confident in C++, start with `cpp01`. If `shared_ptr` or lambdas are unclear,
start with `cppb01`.

## When it does not work

**`ROS 2 not found`**
Run `source /opt/ros/jazzy/setup.bash`. With Docker, it is loaded
automatically inside the container, so you should not see this message.

**The build is slow / it crashes from lack of memory**
An rclcpp template can use close to 1 GB for a single `g++`.
drill limits this to 2 parallel jobs by default. If that is still too much, set it to 1.

```bash
DRILL_COMPILE_JOBS=1 ./drill run 01
```

**`colcon` finds the same package twice**
Check that the `COLCON_IGNORE` files, which exclude `install/` and `log/` from the search,
have not been deleted.
