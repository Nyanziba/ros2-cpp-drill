# ROS 2 coding conventions and language versions (summary)

This is a summary, in English, of the key points of the official document
[Code style and language versions](https://docs.ros.org/en/jazzy/The-ROS2-Project/Contributing/Code-Style-Language-Versions.html)
(original: `ros2_documentation/source/The-ROS2-Project/Contributing/Code-Style-Language-Versions.rst`, jazzy branch).
**This is only a summary, so when you are unsure, always check the original.**

If the original gives a 403 error, you can read the raw file directly.

```
https://raw.githubusercontent.com/ros2/ros2_documentation/jazzy/source/The-ROS2-Project/Contributing/Code-Style-Language-Versions.rst
```

## Target versions

| Language | Target |
| --- | --- |
| C | C99 |
| C++ | **C++17** |
| Python | Python 3 |
| CMake | 3.14.4 or later (REP 2000) |

## C++ — Google C++ Style Guide + ROS 2 changes

The base is the Google C++ Style Guide. ROS 2 adds its own changes on top.
**If you do not know the changes, you will write in plain Google style and get comments in review**, so this part is the real core.

### Formatting

| Item | ROS 2 rule | Note |
| --- | --- | --- |
| Line length | Up to **100 characters** | Relaxed from Google's 80 |
| Header extension | **`.hpp`** | So that tools can tell C and C++ apart |
| Implementation extension | **`.cpp`** | Same as above |
| Pointers | `char * c;` | Not `char* c;`. It breaks down with multiple declarations |
| Nested templates | `set<list<string>>` | No space between them |
| Before `public:` etc. | 0 spaces (recommended) or 2 | Keep the indentation a multiple of 2 |
| Indentation | 2 spaces, no tabs | |
| Braces | **Always use them** | Do not omit them even if `if` / `else` / `do` / `while` / `for` is one line |
| Brace position | Open on a new line for definitions of functions, classes, enums, and structs. For control statements, open on the same line (cuddle) | If the condition wraps, open on a new line even for control statements |
| Wrapping function calls | Wrap at the opening parenthesis, and indent continuation lines by 2 spaces | |

### Naming

| Target | Rule |
| --- | --- |
| Global variables | Lowercase with underscores, with a `g_` prefix |
| Constants | A departure from Google. For historical reasons `snake_case` / `PascalCase` / `UPPER_CASE` are mixed |
| Functions and methods | **Either** `CamelCase` (Google style) **or** `snake_case` (standard library style) is allowed. The ROS 2 core uses `snake_case`. New projects follow the related existing projects |

### What language features are allowed

| Feature | ROS 2's position |
| --- | --- |
| **Lambdas / `std::function` / `std::bind`** | **No restrictions** ("No restrictions") |
| Exceptions | Allowed. Idiomatic in user-facing APIs. But **avoid them in destructors**. Consider avoiding them in APIs that are wrapped by C |
| Visibility of class members | Drops Google's requirement that "all members be private". The default is private, and only what is needed is public |
| Boost | **Avoid it unless you really need it** |

It is important that lambdas and `std::bind` have no official ranking. "bind is old, so we should fix it" is not an official rule.
The points for deciding which to choose are collected in
[7. Lambdas and `std::bind`](cpp/07_lambdas_and_std_bind.md).

### Comments

- For documentation (classes and functions): `///` and `/** */`
- For implementation notes: `//`
- Reason: Doxygen and Sphinx can pick them up

### Linters and static analysis

| Tool | ament wrapper |
| --- | --- |
| Google `cpplint.py` | `ament_cpplint` |
| `uncrustify` | `ament_uncrustify` |
| clang-format | `ament_clang_format` |
| `cppcheck` | `ament_cppcheck` |

The compiler flags are expected to include **`-Wall -Wextra -Wpedantic`**
(the `CMakeLists.txt` of each exercise in this drill also enables these three).

## C — PEP 7 + changes

- Targets C99 (the reasons are that both `//` and `/* */` can be used, and that C99 is widespread enough)
- Allows C++-style `//` comments
- Putting the literal on the left in a comparison is optional (`0 == ret`. It prevents accidental assignment)
- Outside Python modules, the rule of putting `Py_` on everything does not apply. Use CamelCase package names
- Use `pep7` for style checking

## Python — PEP 8 + clarifications

- Lines up to **100 characters** are allowed
- **Prefer single quotes** (except where escaping is needed)
- Prefer hanging indent for continuation lines
- Prefer one import per line
- The linters are `pycodestyle` / `ament_pycodestyle`

## CMake

- Command names are lowercase (`find_package`, not `FIND_PACKAGE`)
- Identifiers are `snake_case`
- Leave the arguments of `else()` and `end...()` empty
- Do not put a space before `(`
- Indent with 2 spaces, no tabs
- Do not align indentation in multi-line macros (only 2 spaces)
- Prefer "a function + `set(... PARENT_SCOPE)`" to a macro
- Give local variables in macros a prefix such as `_`

## Documentation (Markdown / reStructuredText)

- **One sentence per line** (sentence per line). Do not leave extra spaces at the end of a line
- One blank line before and after a heading
- Blank lines before and after a code block, and write the language
- RST heading marks follow the Sphinx hierarchy (`#`, `*`, `=`, `-`, `^`, `"`)
- Markdown headings are in ATX form (1 to 6 `#`, followed by a space)

## How this drill matches

The code in this drill follows the rules above as follows.

- The extensions are `.hpp` / `.cpp`, the indentation is 2 spaces, and lines are within 100 characters
- `-Wall -Wextra -Wpedantic` is enabled in the `CMakeLists.txt` of every exercise
- Function and method names are `snake_case` (to match the ROS 2 core)
- We give priority to matching the official tutorial code on the page,
  so many callbacks are in the form "member function + `std::bind`" (the official docs also have lambda versions, and both are allowed by the conventions)

## Related

- [The design philosophy of rclcpp](rclcpp_design_philosophy.md) — why the API is designed that way
- [7. Lambdas and `std::bind`](cpp/07_lambdas_and_std_bind.md) — criteria for deciding when you fix existing code
- Neighboring documents in the original that you should check: `Developer-Guide.rst` (programming conventions in general),
  `Quality-Guide.rst` (quality levels), and REP 2000 (the dependency versions of each distro)
