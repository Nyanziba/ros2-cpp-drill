# How to Contribute

**日本語: [CONTRIBUTING.md](CONTRIBUTING.md)**

Contributions are welcome. Fixing one typo or adding a new chapter and exercise both help.
If fixing is hard, **just reporting an issue is also a great contribution.**
Requests are welcome too, such as "I want a chapter or an exercise about this". Please use the "Request" issue template.

This is a personal project. I try to follow the latest specifications of ROS 2 and C++ as closely as I can, but I cannot keep up with everything. If you find something out of date, please tell me in an issue.

Your contribution will be released under the same [MIT License](LICENSE) as this repository.
Everyone who takes part in this project must follow the [Code of Conduct](CODE_OF_CONDUCT.en.md).

## Contents

- [Reporting (Issues)](#reporting-issues)
- [Sending changes (Pull Requests)](#sending-changes-pull-requests)
- [The most important rule: output is measured](#the-most-important-rule-output-is-measured)
- [Writing readings (docs/)](#writing-readings-docs)
- [Creating exercises (exercises/)](#creating-exercises-exercises)
- [English version](#english-version)
- [Checklist before you send](#checklist-before-you-send)

## Reporting (Issues)

Choose a template from [Issues](https://github.com/Nyanziba/ros2-cpp-drill/issues/new/choose).

| When | Template |
| --- | --- |
| The output in the text differs from what you got when you ran it | Output differs from the actual run |
| A typo, an error in the explanation, a broken link, a translation error | Content error |
| An exercise does not build or its tests do not pass, a `drill` bug | Exercise or drill bug |
| You want a chapter, exercise, or topic, or something is out of date | Request |

**Output mismatches are especially welcome.** This material promises that "all output shown is measured",
so a mismatch is an error. Please add the environment you ran it in (OS, `g++ --version`, whether you used Docker).

## Sending changes (Pull Requests)

1. Fork this repository and create a branch from `main` (example: `fix/c03-output`).
2. Make your changes. **Put only one topic in one PR** (do not mix a typo fix with a new chapter).
3. Check the [checklist](#checklist-before-you-send) below.
4. Open a PR to `main`. The PR template appears; please fill in its items.

Write commit messages as `<type>: <description>`. The type is one of `feat` `fix` `docs` `refactor` `test` `ci` `chore`.
The description can be in Japanese or English.

```
fix: match the output of chapter C 03 to the actual run
```

### Prepare your environment

Building the exercises needs ROS 2 Jazzy (Ubuntu 24.04). **Using Docker is the most reliable way.**
The steps are in [README](README.en.md) and [Getting started](docs-en/getting-started.md).

```bash
docker compose build
docker compose run --rm drill ./drill verify     # test all exercises
```

To check the readings site on your machine, do this:

```bash
python3 -m venv .venv-docs
.venv-docs/bin/pip install -r docs-requirements.txt
.venv-docs/bin/mkdocs build --strict                    # Japanese version → site/
.venv-docs/bin/mkdocs build --strict -f mkdocs.en.yml   # English version → site/en/ (always after the Japanese version)
```

## The most important rule: output is measured

**Every compile error, warning, and run result in the readings must be output you actually ran.**
Do not include output written from a guess or memory, or output edited by hand.

- **The environment is Ubuntu 24.04 / g++ 13.3.0 / ROS 2 Jazzy.** The Docker image in this repository is this environment.
  - The existing output was taken on x86_64. On an Apple Silicon Mac,
    build an x86_64 image with `docker build --platform linux/amd64 -t ros2-drill:jazzy-amd64 .` and measure with it.
    Run results are almost the same on arm64, but some things can change with the architecture, such as the sign of `char`.
  - If you measured in another environment (example: Apple clang on macOS), **state that environment in the text**
    (the Design Patterns track has places that say "measured with Apple clang / arm64").
- **If you change the code, measure the output again.** Even changing one comment line
  can change the output or line numbers of a compile error that quotes that source line.
- **Put all the code that produced the output in the text.** If you add a `main` or other code that is not in the text and measure that,
  readers cannot reproduce the same output.
- Values that change on every run, such as addresses and times, can stay as you measured them.
- When you show only part of the output, mark the omission with `...` or similar.
- For things whose result depends on the environment (data races, undefined behavior, exit codes, and so on), write the environment you measured in and that the result "depends on the environment".

The code in the C++ chapters has a [Compiler Explorer](https://godbolt.org/) link (`▶ Run in your browser`).
**If you change the code, make the link again.** Paste the new code into Compiler Explorer,
use the compiler `x86-64 gcc 13.3` and the same options as the command in the text, and replace the link with the Share short link.

## Writing readings (docs/)

- Put files in the directory of each track (`docs/c/` `docs/cpp-basics/` `docs/cpp/` `docs/ros2/` `docs/patterns/`).
  File names are `<number>_<title>.md`.
- **Add new pages to `nav` in `mkdocs.yml`.** If you do not, they do not appear on the site.
- Link pages to each other with relative paths to `.md` (example: `[6. const](06_const.md)`).
- `mkdocs build --strict` fails on broken links, but **it does not fail on a mismatched heading anchor (`#...`)** (it only prints INFO).
  Check by eye that the build output has no `does not contain an anchor`.

### Chapter template

Existing chapters are mostly written in the following form. Please make new chapters match it.

```markdown
# 6. const

> **Goal of this chapter**: (what the reader can do after it, in 2 to 3 lines)

## 6.1 (section)
(Body text. Section numbers are "chapter.section")

## Try it yourself

(code to try)

**Predict: (a question that makes the reader guess what will happen before running)**

    g++ -std=c++17 -Wall -Wextra -Wpedantic xxx.cpp -o xxx && ./xxx

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/...)

<details markdown="1"><summary>Answer (actual output)</summary>

(measured output)

</details>

## Common pitfalls
## Matching exercise
## References

---

Previous chapter → [...](...)
Next chapter → [...](...)
```

- **Always fold the answer to a "Predict" question in `<details>`.** This is because the reader is expected to guess first and then open it.
- Write in a short, plain style.

## Creating exercises (exercises/)

A reading chapter and an exercise match **one to one** (example: `docs/cpp-basics/06_const.md` ↔ `exercises/cppb06_const`).

### File layout

`<id>` is the exercise ID (example: `cppb06_const`).

```
exercises/<id>/
├── CMakeLists.txt          # project(drill_<id>). Add -Wall -Wextra -Wpedantic
├── package.xml             # <name>drill_<id></name>
├── README.md               # exercise text (what to do, how to run, common pitfalls, tests, references)
├── include/drill/...       # headers
├── src/...                 # files the learner edits (// I AM NOT DONE at the top)
└── test/test_exercise.cpp  # tests for grading (the learner does not edit)
templates/<id>/...          # initial state of the files the learner edits (where ./drill reset restores)
solutions/<id>/...          # sample solution (shown by ./drill solution)
```

- Put `// I AM NOT DONE` at the top of the files the learner edits. `drill` treats the exercise as "done"
  when all of these markers are gone.
- In `templates/<id>/`, put files with the **same content** as the files to edit in `exercises/<id>/`.
- Tests that use ROS 2 use the shared helper [`tools/drill_harness.hpp`](tools/drill_harness.hpp).
- Write test failure messages so the learner knows **what to check next**
  (example: "Did you put `create_publisher<std_msgs::msg::String>("topic", 10)` into `publisher_`?").

### Register it in exercises.json

Add one entry to the `exercises` array in `exercises.json`. The order is the order of `./drill list`.

| Field | Content |
| --- | --- |
| `id` / `package` | The exercise ID and the package name (`drill_<id>`) |
| `level` / `level_note` | The track name and its description |
| `title` / `chapter` | The exercise title and the matching chapter (shown by `./drill list`) |
| `kind` | `gtest` (normal) / `pytest` (launch or YAML) / `gtest_mutation` (the learner writes the tests) |
| `sources` | Files the learner edits (relative to the exercise directory) |
| `hints` | Hints shown by `./drill hint`, from the weakest to the strongest |
| `lecture` | The matching chapter (`title` and `path`) |
| `docs` | Links to official documentation (`./drill doc`) |

For the English fields (`title_en` `hints_en` `lecture_en` and so on), see [English version](#english-version).

### Check it

Check both: **it fails when unsolved, and it passes with the sample solution applied.**

```bash
docker compose run --rm drill ./drill run <id>     # unsolved: must fail
cp -r solutions/<id>/. exercises/<id>/              # apply the sample solution
docker compose run --rm drill ./drill run <id>     # must pass
docker compose run --rm drill ./drill reset <id>   # restore
```

Commit `exercises/` in the unsolved state (with `// I AM NOT DONE` still in place).

## English version

The English readings are in `docs-en/`, the exercise text is in `exercises/<id>/README.en.md`,
and in `exercises.json` they are the `*_en` fields. **The Japanese version is the original, and the English version is its translation.**

- If you fix the Japanese version, **please fix the English version in the same PR if you can.** The rules, the file name table, and the glossary
  are in [TRANSLATING.md](TRANSLATING.md).
- If English is hard for you, fix only the Japanese version and write "English version not done" in the PR body. A maintainer will follow up.
- When you translate code in the English version, [measure](#the-most-important-rule-output-is-measured) the output again too.

## Checklist before you send

The PR template has the same items.

- [ ] All output shown is the result of an actual run (if the environment differs from the default, it is stated in the text)
- [ ] If I changed code, I also made the output and the Compiler Explorer link again
- [ ] `mkdocs build --strict` passes (also the English `-f mkdocs.en.yml` if I changed the readings), and no anchor INFO appears
- [ ] If I changed an exercise, I checked that it fails when unsolved and passes with the sample solution
- [ ] I fixed the English version too (or wrote "English version not done" in the PR)
- [ ] Only one topic in one PR
