# ROS 2 Drill (ros2-drill)

**📖 Read the lecture notes here → <https://nyanziba.github.io/ros2-cpp-drill/en/>**
(Japanese version: <https://nyanziba.github.io/ros2-cpp-drill/>)
(Chapter navigation, track tabs, and full-text search. No installation needed.)

**English version:** All tracks are translated. Comments in the exercise source code and the test failure messages stay in Japanese.

**You can run the code examples in your browser.** The code examples in the C++ chapters
have links to Compiler Explorer, so you can follow along even without `g++`.
The answers to "**Predict: ...**" are folded. **Predict first, then open them.**

This is a **drill for ROS 2 / C++ training**, in the style of Rust's [rustlings](https://rustlings.rust-lang.org/).
You fill in the blanks in the source code, and the tests judge pass or fail, so nobody needs to grade your work.

## How to get this repository

**You need nothing to just read.** Open the public site above.
**To solve the exercises**, bring the repository to your machine.

```bash
git clone https://github.com/Nyanziba/ros2-cpp-drill.git
cd ros2-cpp-drill
```

`drill` is a single Python 3 file. No extra libraries are needed.
Set `DRILL_LANG=en` to get English output (Docker: `DRILL_LANG=en docker compose run --rm drill ./drill list`).

```bash
./drill list     # exercise list and progress
./drill run      # build and test the next unfinished exercise
```

### Which environment to use

Building the exercises needs `colcon` and `ament_cmake`. Choose one of the following.

| | Good for | Effort |
| --- | --- | --- |
| **Docker** | Anyone not on Ubuntu (macOS / Windows / other distros), or anyone who wants to keep their system clean | One `docker compose build` (10 to 20 minutes) |
| **Install directly** | Anyone on Ubuntu 24.04 who will also install ROS 2 | Requires installing ROS 2 Jazzy |

```bash
# With Docker
docker compose build
docker compose run --rm drill        # enter the container
./drill list
```

ROS 2 Jazzy has official packages **only for Ubuntu 24.04.**
On any other OS, choose Docker.
See [Getting started](docs-en/getting-started.md) for detailed steps.

### Where to start

**Not everyone has to do everything.** Do only the track you need.

```bash
./drill run cppb01     # first exercise of the C++ Basics track
./drill run c01        # first exercise of the C track
./drill run dp01       # first exercise of the Design Patterns track
./drill run 01         # first exercise of the ROS 2 track
```

You can move back and forth between the reading and the exercises.

```bash
./drill read cppb01          # open the matching chapter in the terminal
./drill read cppb01 --web    # open it in the browser
./drill hint cppb01          # hint
./drill solution cppb01      # sample solution
./drill reset cppb01         # return to the initial state
```

### Use it as your own teaching material

Fork the repository and replace `exercises/` and `docs/`, and it becomes your own drill.
`exercises.json` is the table that maps exercises to chapters, so edit that file.
For how to publish the site on your own GitHub Pages, see
[Fork it as your own site](#fork-it-as-your-own-site).

**The license is MIT.** You are free to use, modify, and redistribute it.

```bash
./drill watch
```

Your exercise is tested again every time you save. The reading (`docs/`) has 49 chapters in total across 3 tracks,
and the 35 exercises in `exercises/` **match the chapters that have an exercise, by chapter number**.

## The 5 tracks

**There is a C++ course before ROS 2. There is also a C course for microcontroller work,
and a Design Patterns course for designing libraries.**
The reading (`docs/`) and the exercises (`exercises/`) **match one-to-one, down to the chapter number**.

| Track | Reading | Exercises | Target |
| --- | --- | --- | --- |
| **C track** | [docs-en/c/](docs-en/c/README.md) | `c01` to `c12` | People who write microcontroller, drive-train, and CAN code. 12 chapters |
| **C++ Basics** | [docs-en/cpp-basics/](docs-en/cpp-basics/README.md) | `cppb01` to `cppb10` | People who get stuck on `const` and `static`. 10 chapters |
| **C++** | [docs-en/cpp/](docs-en/cpp/README.md) | `cpp01` to `cpp12` | Preparation for reading rclcpp. 15 chapters |
| **ROS 2** | [docs-en/ros2/](docs-en/ros2/01_start_here_course_hub.md) | `01` to `15` | 23 articles |
| **Design Patterns** | [docs-en/patterns/](docs-en/patterns/README.md) | `dp01` to `dp23` | People who design their own libraries. Meant to be read alongside Hiroshi Yuki's *Learning Design Patterns in Java* (in Japanese). 23 chapters |

**Not everyone has to do everything.**

| Where the person is heading | How to proceed |
| --- | --- |
| ROS 2 / autonomous navigation | `cppb01`... → `cpp01`... → `01`... (**you may skip the C track**) |
| Microcontrollers / drive train / CAN | `c01` to `c12` only |
| Designing libraries | `dp01` to `dp23` (while reading Hiroshi Yuki's *Learning Design Patterns in Java*) |

**The Design Patterns track is independent of the other tracks.** There is no required order.

**The C track, C++ Basics, C++, and Design Patterns do not use ROS 2.** They use only `ament_cmake` and gtest,
so each exercise builds in 2 to 3 seconds.

rclcpp depends heavily on `shared_ptr`, lambdas, `std::move`, and templates.
If you start ROS 2 with those still unclear, you cannot tell whether you are learning ROS
or fighting C++ syntax. That is why the course is built to finish C++ first.

For the big picture, see [Overview of the materials](docs-en/README.md).

## About the code and output in this material

**Every compile error and execution result in this material is real output from an actual run.**
Where my prediction and the real result differed, I changed the text to match the real result.
The environment is Ubuntu 24.04 / g++ 13.3.0 / ROS 2 Jazzy.

The "Try it yourself" section in each chapter is written on the assumption that you **predict first, then run**.
The places where you were wrong are your gaps, so do not skip them.

## Requirements

- ROS 2 Jazzy (`/opt/ros/jazzy`)
- Ubuntu 24.04 / g++ 13 / CMake 3.28
- Python 3 (for the runner. No extra libraries are needed)

**If you are not on Ubuntu, you can use Docker.** See the next section.

## Run on a non-Ubuntu OS (Docker)

ROS 2 Jazzy has official packages only for Ubuntu 24.04.
If the inside of the container is Ubuntu 24.04, it works the same way on
macOS, Windows, and other distros. It also works on arm64 (Apple Silicon).

```bash
docker compose build                          # first time only (10 to 20 minutes)
docker compose run --rm drill ./drill list
docker compose run --rm drill ./drill watch cppb01
docker compose run --rm drill                 # enter bash
```

**The container reads the source directly from the host.** Edit with your usual editor.
When you save, `watch` picks it up (`drill` only looks at mtime,
so it does not miss changes even on macOS or Windows bind mounts).

**On environments that lose the executable bit (for example, when the repository is on a Windows NTFS drive),**
type `python3 drill list` instead of `./drill`.

Build outputs are kept apart from the host's `build/`, `install/`, and `log/`
(named volumes). CMake bakes absolute paths and compiler paths into its cache,
so mixing them with a native build breaks it. To remove them, run
`docker compose down -v`.

**Only on Linux when your UID is not 1000**, create a `.env` file first.
If you do not, files generated in the container are owned by root, and you cannot save them in your host editor.

```bash
printf 'DRILL_UID=%s\nDRILL_GID=%s\n' "$(id -u)" "$(id -g)" > .env
```

You cannot use the name `UID`. It is a bash built-in variable that is not exported,
so compose cannot see it and always uses the default value.
Docker Desktop on macOS and Windows matches ownership automatically, so this is not needed there.

### GUI (turtlesim / rqt / RViz)

All exercises are gtest / pytest, so **you can finish every exercise without a GUI.**
The GUI is needed only for the reading (the ROS 2 chapters that run turtlesim).

```bash
docker compose build --build-arg WITH_GUI=1     # the image grows by more than 1 GB
docker compose --profile x11 run --rm drill-gui ros2 run turtlesim turtlesim_node
```

You need to allow X on the host side.

| Host | What to do |
| --- | --- |
| Linux | `xhost +local:docker` |
| macOS | Install XQuartz, turn on "Allow connections from network clients", then run `xhost +localhost` and set `DISPLAY=host.docker.internal:0` |
| Windows (WSL2) | Works as is if you have WSLg |

CI (`.github/workflows/docker.yml`) builds the image every time and checks both that
**unsolved exercises fail** and that **exercises with the solution applied pass**.

## Getting started

This is for when ROS 2 Jazzy is installed on Ubuntu 24.04. For other cases, use Docker above.

```bash
source /opt/ros/jazzy/setup.bash   # drill finds it automatically even if you did not source it
./drill list                       # exercise list and progress
./drill watch                      # work while watching the first unfinished exercise
```

Open the exercise source, fill in the `TODO`, and save. When the test passes,
delete the following line near the top of the file to move on to the next exercise.

```cpp
// I AM NOT DONE
```

## Commands

| Command | Description |
| --- | --- |
| `./drill list` | Exercise list and progress |
| `./drill run [ID]` | Build and test (without ID, the next unfinished exercise) |
| `./drill watch [ID]` | Detect saves and re-test automatically |
| `./drill hint ID` | Hint (shows even the shape of the code) |
| `./drill doc ID` | URL of the matching official documentation |
| `./drill read [ID]` | Open the matching lecture notes chapter (`--web` for the browser) |
| `./drill solution ID` | Show the sample solution |
| `./drill reset ID` | Return the exercise to its initial state |
| `./drill verify` | Test all exercises in order |
| `./drill completion [bash\|zsh]` | Print the tab completion script |

### Read the notes in your browser

By default, `./drill read` pipes the chapter into `less`. Long chapters are easier to read on the site,
so add `--web` to open it in the browser.

```bash
./drill read --web cppb06     # by default, opens the page on the public site
./drill read --build cppb06   # build the site locally, then open it (for offline use)
```

The destination is decided in this order.

| Order | Destination |
| --- | --- |
| 1 | `DRILL_DOCS_URL` (if you host the site yourself) |
| 2 | The local `site/` (opened with `file://`) |
| 3 | <https://nyanziba.github.io/ros2-cpp-drill/> |

If `mkdocs` is not installed, `--build` prepares it in `.venv-docs/` and then builds.

**You cannot open a browser inside Docker.** `site/` is created on the bind mount,
so open `site/cpp-basics/06_const.html` in a browser on the host
(the path is printed).

IDs are distinguished by prefix. **`cppb` = C++ Basics, `cpp` = C++, digits only = ROS 2**.
Type them like `./drill run cppb06` / `./drill run cpp06` / `./drill run 01`.
A partial match such as `./drill run publisher` also works.

For tab completion, add the following line to `~/.bashrc` (for zsh, use `completion/drill.zsh`).

```bash
source /path/to/ros2-drill/completion/drill.bash
```

## Editor (VS Code)

`.vscode/` is included. IntelliSense works as soon as you open the folder.
Without it, `rclcpp` and everything else gets red underlines, and you see this.

```
#include errors detected. Please update your includePath.
Squiggles are disabled for this translation unit.
```

**ROS 2 headers are one level deeper, at `/opt/ros/jazzy/include/<package name>/<package name>/...`.**
To resolve `#include "rclcpp/rclcpp.hpp"`, you need the directory of each package below
`/opt/ros/jazzy/include`, not `/opt/ros/jazzy/include` itself, so
the config picks them up recursively with `**`.

**Only the headers generated from your own `.msg` / `.srv` files keep red underlines until you build once.**
They exist only in `install/`. Pass `./drill run 03` and they go away.

### Dev Container if you are not on Ubuntu

If the host has no `/opt/ros/jazzy`, IntelliSense does not work.
`.devcontainer/` is included, so run **"Dev Containers: Reopen in Container"**
to open the project inside the container. It uses `compose.yaml` as is,
so you get the same environment that runs the drill.

## Read the notes as a site

### Public site

**<https://nyanziba.github.io/ros2-cpp-drill/en/>** (Japanese: <https://nyanziba.github.io/ros2-cpp-drill/>)

Every push to `main` triggers `.github/workflows/docs.yml`, which publishes the site automatically.
This is the entry point.

| | |
| --- | --- |
| Overview (entry point) | <https://nyanziba.github.io/ros2-cpp-drill/en/> |
| C++ Basics | <https://nyanziba.github.io/ros2-cpp-drill/en/cpp-basics/> |
| C++ | <https://nyanziba.github.io/ros2-cpp-drill/en/cpp/> |
| ROS 2 | <https://nyanziba.github.io/ros2-cpp-drill/en/ros2/01_start_here_course_hub.html> |

### Build the site locally

Use this when you want to read offline, or when you edit the notes and want to check them.

```bash
python3 -m venv .venv-docs          # on Ubuntu, run sudo apt install python3-venv first
.venv-docs/bin/pip install -r docs-requirements.txt
.venv-docs/bin/mkdocs serve         # http://127.0.0.1:8000
```

`mkdocs build --strict` turns broken links and heading anchor mismatches into errors.
CI also runs it, so a push that breaks a link fails.
You can also download the built site from the `docs-site` artifact in Actions.

### Fork it as your own site

Set `Settings > Pages > Source` to **GitHub Actions** and change `site_url` in `mkdocs.yml`
to your own URL. Then the site is published as is.
The English site is built in the same way with `mkdocs build -f mkdocs.en.yml`
(run it after the Japanese build; the output goes to `site/en/`).

## Scope of this material

**From the basics of C++ and ROS 2 to the point where you can read and write rclcpp code.**
The ROS 2 track covers up to Part 3: the basics, package development, TF2, URDF, `ros2_control`,
and sensor integration.

It does not cover the design of the autonomous navigation stack itself (implementing planners and controllers).
That depends heavily on each individual robot, so it is outside this material.

## Contributing

Reports and fixes are welcome. Requests are welcome too, such as "I want a chapter or an exercise about this". Please use the "Request" issue template.
Please read [CONTRIBUTING.en.md](CONTRIBUTING.en.md) before you open an issue or a pull request.
Everyone who takes part must follow the [Code of Conduct](CODE_OF_CONDUCT.en.md).

This is a personal project. I try to follow the latest specifications of ROS 2 and C++ as closely as I can, but I cannot keep up with everything. If you find something out of date, please tell me in an issue.

## License

MIT. See [LICENSE](LICENSE) for details.
