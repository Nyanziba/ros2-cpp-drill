# Slides

These are the slides for presenting this material. There are **3 versions, by length**.
You can open them right in the browser (the same link also downloads them). (The slides are in Japanese.)

| Length | Slides | PDF |
| --- | --- | --- |
| **3 min** (lightning talk) | 6 | [slides-3min.pdf](https://nyanziba.github.io/ros2-cpp-drill/発表資料/slides-3min.pdf) |
| **5 to 10 min** | 18 | [slides-10min.pdf](https://nyanziba.github.io/ros2-cpp-drill/発表資料/slides-10min.pdf) |
| **15 min** | 26 | [slides-15min.pdf](https://nyanziba.github.io/ros2-cpp-drill/発表資料/slides-15min.pdf) |

All three share the same backbone: **solving exercise `01_publisher` for real**.
The order is: fail while it is unsolved → `hint` → make a mistake on purpose → pass → `watch` → run on a real machine.

<iframe src="https://nyanziba.github.io/ros2-cpp-drill/発表資料/slides-10min.pdf#view=FitH" width="100%" height="520"
        style="border:1px solid #d3d7e0; border-radius:6px;"
        title="Slides (5 to 10 min version)"></iframe>

If it does not display well in your environment, open [slides-10min.pdf](https://nyanziba.github.io/ros2-cpp-drill/発表資料/slides-10min.pdf) directly.

## What the slides cover

### Motivation

In the year I started ROS 2, I could not read `create_publisher` in the official tutorial.
What stopped me was not ROS but templates, `shared_ptr`, and `std::bind`.
The next year, when I moved to the teaching side, a different problem appeared. **Even if I gave exercises,
I could not tell pass or fail unless I looked myself.** The same question came as many times as there were people.

So I made **exercises that need no grader**. I brought to ROS 2 the form that Rust's
[rustlings](https://rustlings.rust-lang.org/) used.

### All 35 exercises, and the progress

![Output of ./drill list](https://nyanziba.github.io/ros2-cpp-drill/発表資料/images/01_list.png)

`[11章]` is the chapter of the reading that matches. `./drill read 01` opens that chapter.
In the other direction, you can also go from the "Matching exercise" at the end of a chapter to the exercise.

### If you run it unfilled, the test tells you the next step

![Output of ./drill run 01 run on an unsolved exercise](https://nyanziba.github.io/ros2-cpp-drill/発表資料/images/02_run_fail.png)

The failed test contains what to do next, in Japanese.
The most frequent stumbling block is **taking the return value of `create_publisher` in a local variable**,
so it even says "Did you put it in `publisher_`?".

### It even catches the way you get it wrong

This is the case where you write `++count_` instead of `count_++`. It compiles, and it can publish,
but the first message is still not `Hello, world! 0`.

![Output when you write ++count_](https://nyanziba.github.io/ros2-cpp-drill/発表資料/images/04_run_wrong.png)

2 of the 4 tests pass. The 2 that fail reply with "**Are you using the post-increment (`count_++`) on `count_`?**",
with the actual values.
The detail that a human reviewer used to point out is built into the failure message.

### Fix it and it passes

![Output when all 4 tests pass](https://nyanziba.github.io/ros2-cpp-drill/発表資料/images/05_run_pass.png)

Passing alone does not move you to the next one. Deleting `// I AM NOT DONE` yourself is the sign that you are done.

### Everyday use

![Output of ./drill watch passing and moving to the next exercise](https://nyanziba.github.io/ros2-cpp-drill/発表資料/images/06_watch.png)

You write with `./drill watch` left open. Save → rebuild → retest runs automatically,
and when it passes it moves on to the next exercise.

### What you end up with is an ordinary ROS 2 node

![Output of ros2 run and ros2 topic echo / hz](https://nyanziba.github.io/ros2-cpp-drill/発表資料/images/07_live.png)

`ros2 topic hz` shows 2.000 Hz. The 500 ms timer works exactly as set.
It is not a closed world just for the drill. What stays in your hands is the same thing as the official tutorial.

## About the output shown

**The terminal images in the slides are all output from real runs.**
Where lines were dropped from a long output, `……（中略）……` ("... (omitted) ...") is inserted,
and only the absolute path of the repository is shortened to `~/ros2-cpp-drill`. The wording is not changed.

The environment is Ubuntu 24.04 / ROS 2 Jazzy / g++ 13.3.0.

## Related

- [Overview of the material](README.md) — how the readings and exercises match
- [Getting started](getting-started.md) — running it on your machine
- [ROS 2 lecture 11: Writing pub/sub in C++](ros2/11_writing_pub_sub_in_cpp.md) — the chapter that matches the exercise shown in the demo
