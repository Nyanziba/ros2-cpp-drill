# 12. Preprocessor and debugging

> **Goal of this chapter**: The C preprocessor (macros) does **plain text substitution** before compilation. There is no type checking and no calculation. You check by measurement the traps of missing parentheses, evaluating an argument more than once, and multi-statement macros. You also learn how to raise robustness in microcontroller control by using `assert` and the compile-time check (C11 `_Static_assert`).

## 12.1 `#define` is string substitution

**A macro is just text substitution. There is no type and no checking.**

```c
#define DOUBLE(x) x * 2
```

In this case, `DOUBLE(1 + 2)` is expanded as follows.

```c
1 + 2 * 2   // by operator precedence, 1 + (2*2) = 5
```

Without parentheses, the order of calculation is not what you expect.

## 12.2 The trap of forgetting parentheses

**Parentheses are required both in the macro definition and around the arguments.**

Checking by measurement:

```c
#define DOUBLE_BROKEN(x) x * 2
#define DOUBLE_FIXED(x) ((x) * 2)

DOUBLE_BROKEN(1 + 2)  // expansion: 1 + 2 * 2 = 5
DOUBLE_FIXED(1 + 2)   // expansion: ((1 + 2) * 2) = 6
```

Measured values:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// macro_parentheses.c
#include <stdio.h>

#define DOUBLE_BROKEN(x) x * 2
#define DOUBLE_FIXED(x) ((x) * 2)

int main(void)
{
    printf("DOUBLE_BROKEN(1 + 2) = %d\n", DOUBLE_BROKEN(1 + 2));  // expansion: 1 + 2 * 2 = 5
    printf("DOUBLE_FIXED(1 + 2) = %d\n", DOUBLE_FIXED(1 + 2));    // expansion: ((1 + 2) * 2) = 6
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic macro_parentheses.c -o macro_parentheses && ./macro_parentheses
```

</details>

<!-- measure: -->
```
DOUBLE_BROKEN(1 + 2) = 5
DOUBLE_FIXED(1 + 2) = 6
```

**Rules for parentheses:**
- Wrap the whole expression of the macro body in parentheses: `((x) * 2)`
- Wrap the macro arguments in parentheses too: `((x) * (x))`

## 12.3 The trap of evaluating an argument twice

**A macro substitutes its argument as it is, several times, so if you pass an expression with side effects, it behaves unexpectedly.**

```c
#define SQ(x) ((x) * (x))

int i = 0;
int result = SQ(i++);  // i++ is evaluated twice
```

Measured values:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// macro_double_evaluation.c
#include <stdio.h>

#define SQ(x) ((x) * (x))

int main(void)
{
    int i = 0;
    int result = SQ(i++);  // i++ is evaluated twice

    printf("i after SQ(i++) = %d, result = %d\n", i, result);
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic macro_double_evaluation.c -o macro_double_evaluation && ./macro_double_evaluation
```

</details>

<!-- measure: filter="tail -n 1" -->
```
i after SQ(i++) = 2, result = 0
```

**Note: the above is undefined behavior.** `SQ(i++)` changes `i` twice in the same expression, so the C standard says "the result is not guaranteed". In this measurement it gave these values, but it may differ between runs and between implementations.

**Do not write `SQ(i++)`.** Instead:

```c
int temp = i;
int result = SQ(temp);
i++;
```

## 12.4 Wrap a multi-statement macro in `do { } while (0)`

**A macro with several statements breaks inside `if` and `else` unless you wrap it in `do { ... } while (0)`.**

A multi-statement macro without the wrapper:

```c
#define LOG_BROKEN(fmt) \
    printf("DEBUG: " fmt "\n"); \
    printf("More info\n")

if (condition)
    LOG_BROKEN("message");  // the else is ignored ✗
else
    handle_error();
```

Compiler warning at compile time:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// multistatement_macro.c
#include <stdio.h>

static void handle_error(void)
{
    printf("error\n");
}

#define LOG_BROKEN(fmt) \
    printf("DEBUG: " fmt "\n"); \
    printf("More info\n")

int main(void)
{
    int condition = 1;

    if (condition)
        LOG_BROKEN("message");  // the else is ignored ✗
    else
        handle_error();

    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic multistatement_macro.c -o multistatement_macro
```

</details>

<!-- measure: -->
```
multistatement_macro.c: In function ‘main’:
multistatement_macro.c:10:5: warning: macro expands to multiple statements [-Wmultistatement-macros]
   10 |     printf("DEBUG: " fmt "\n"); \
      |     ^~~~~~
multistatement_macro.c:18:9: note: in expansion of macro ‘LOG_BROKEN’
   18 |         LOG_BROKEN("message");  // the else is ignored ✗
      |         ^~~~~~~~~~
multistatement_macro.c:17:5: note: some parts of macro expansion are not guarded by this ‘if’ clause
   17 |     if (condition)
      |     ^~
multistatement_macro.c:19:5: error: ‘else’ without a previous ‘if’
   19 |     else
      |     ^~~~
```

Besides the warning, `error: ‘else’ without a previous ‘if’` is also printed. The comment in the code above says "the else is ignored", but in fact the `else` is not ignored: the macro expands to 2 statements, so the body of the `if` is only the first one, and the `else` is left after a statement that has no `if` — a compile error.

**Fix with `do { } while (0)`:**

```c
#define LOG_FIXED(fmt) do { \
    printf("DEBUG: " fmt "\n"); \
    printf("More info\n"); \
} while (0)

if (condition)
    LOG_FIXED("message");  // the control flow connects correctly ✓
else
    handle_error();
```

The caller adds the semicolon (do not put it at the end of the macro).

## 12.5 Stringizing `#` and token pasting `##`

**`#` turns an argument into a string. `##` joins two tokens.**

```c
#define STRINGIFY(x) #x
#define CONCAT(a, b) a ## _ ## b

STRINGIFY(hello)     // "hello"
CONCAT(foo, bar)     // foo_bar (joins the tokens foo and bar)
```

To check the macro expansion, look at the output of the preprocessor:

```bash
gcc -E -P -std=c99 macro_expansion.c
```

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// macro_expansion.c
#define STRINGIFY(x) #x
#define CONCAT(a, b) a ## _ ## b

const char *greeting = STRINGIFY(hello);  // "hello"
int CONCAT(foo, bar);                     // foo_bar(joins the tokens foo and bar)
```

```bash
gcc -E -P -std=c99 macro_expansion.c
```

</details>

Measured values:

<!-- measure: -->
```c
const char *greeting = "hello";
int foo_bar;
```

## 12.6 `assert` — a run-time check and a compile-time check

**`assert(condition)` reports to standard error and ends the program if the condition is false.**

```c
#include <assert.h>

assert(x > 0);  // if x > 0 is not true, print a message and abort
```

You can disable assertions with `#define NDEBUG`:

```bash
gcc -DNDEBUG myfile.c   // all assert calls disappear (the condition is not evaluated)
```

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// assert_expansion.c
#include <assert.h>

int main(void)
{
    assert(1 == 1);
    return 0;
}
```

```bash
gcc -E -P -std=c11 assert_expansion.c | sed -n '/^int main/,$p'
gcc -E -P -std=c11 -DNDEBUG assert_expansion.c | sed -n '/^int main/,$p'
```

</details>

Measured values (without `NDEBUG`):

<!-- measure: filter="head -n 5" -->
```c
int main(void)
{
    ((1 == 1) ? (void) (0) : __assert_fail ("1 == 1", "assert_expansion.c", 6, __extension__ __PRETTY_FUNCTION__));
    return 0;
}
```

Measured values (with `-DNDEBUG`; it becomes an expression that does nothing):

<!-- measure: filter="tail -n 5" -->
```c
int main(void)
{
    ((void) (0));
    return 0;
}
```

**Do not write side effects inside an assertion:**

```c
assert(++i == 10);  // ++i disappears when NDEBUG is defined ✗

// correct:
i++;
assert(i == 10);    // ✓
```

## 12.7 C11 `_Static_assert` — compile-time type and size checks

**Where the size of a struct must not change (such as the payload of a CAN message), check it at compile time.**

```c
#include <stddef.h>

typedef struct {
    uint32_t timestamp;
    uint8_t data[4];
} CANPayload;

_Static_assert(sizeof(CANPayload) == 8, "CAN payload must be exactly 8 bytes");
```

**`_Static_assert` is a feature since C11.** It is evaluated at compile time, and if the condition is false, it stops with a **compile error**.

Example run (on failure):

```bash
gcc -std=c11 static_assert_fail.c
```

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// static_assert_fail.c
typedef struct {
    int x;
    char y;
    char padding[3];
} MyStruct;

_Static_assert(sizeof(MyStruct) == 16, "MyStruct must be 16 bytes");

int main(void)
{
    return 0;
}
```

```bash
gcc -std=c11 static_assert_fail.c
```

</details>

<!-- measure: -->
```
static_assert_fail.c:8:1: error: static assertion failed: "MyStruct must be 16 bytes"
    8 | _Static_assert(sizeof(MyStruct) == 16, "MyStruct must be 16 bytes");
      | ^~~~~~~~~~~~~~
```

On success, nothing is printed and compilation passes:

```c
typedef struct {
    int x;
    char y;
    char padding[3];
} MyStruct;

_Static_assert(sizeof(MyStruct) == 8, "MyStruct must be 8 bytes");
// compile succeeds, run result: sizeof(MyStruct) = 8
```

If you add a `main` and run it, it prints:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// static_assert_ok.c
#include <stdio.h>

typedef struct {
    int x;
    char y;
    char padding[3];
} MyStruct;

_Static_assert(sizeof(MyStruct) == 8, "MyStruct must be 8 bytes");

int main(void)
{
    printf("sizeof(MyStruct) = %zu\n", sizeof(MyStruct));
    return 0;
}
```

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic static_assert_ok.c -o static_assert_ok && ./static_assert_ok
```

</details>

<!-- measure: -->
```
sizeof(MyStruct) = 8
```

## 12.8 Define a macro at compile time with `-D`

**When you compile a program, you can pass a `#define` as a command-line argument.**

```c
// myfile.c
#include <stdio.h>

#ifdef DEBUG_MODE
    #define LOG(msg) printf("[DEBUG] %s\n", msg)
#else
    #define LOG(msg)  // empty macro (does nothing)
#endif

int main(void) {
    LOG("This is a debug message");
    printf("Program running\n");
    return 0;
}
```

**Normal build:**

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic myfile.c -o myfile
./myfile
```

<!-- measure: -->
```
Program running
```

**Build in debug mode:**

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic -DDEBUG_MODE myfile.c -o myfile_debug
./myfile_debug
```

<!-- measure: -->
```
[DEBUG] This is a debug message
Program running
```

## 12.9 Check the expansion with `gcc -E` (a debugging tool)

**You can check whether a macro is expanded correctly by looking at the output of the preprocessor.**

```bash
gcc -E -P -std=c99 macro_expansion.c
```

The `-E` flag stops at the preprocessor stage and prints the expanded code to standard output. With `-P`, the line-marker lines (`# 1 "..."`) are not printed.

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// macro_expansion.c
#define STRINGIFY(x) #x
#define CONCAT(a, b) a ## _ ## b

const char *greeting = STRINGIFY(hello);  // "hello"
int CONCAT(foo, bar);                     // foo_bar(joins the tokens foo and bar)
```

```bash
gcc -E -P -std=c99 macro_expansion.c
```

</details>

Measured values:

<!-- measure: -->
```c
const char *greeting = "hello";
int foo_bar;
```

It is handy for checking whether a macro is expanded into the form you intended.

## Try it yourself

Check how the preprocessor, assertions, and compile-time checks behave for yourself.

```c
// preprocessor_demo.c
#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/* the trap of forgetting parentheses, and evaluating an argument more than once */
#define DOUBLE_BROKEN(x) x * 2
#define DOUBLE_FIXED(x) ((x) * 2)

#define SQ_MACRO(x) ((x) * (x))

/* multi-statement macro */
#define LOG(msg) do { \
    printf("[LOG] %s\n", msg); \
    printf("      (logged)\n"); \
} while (0)

/* CAN message payload */
typedef struct {
    uint32_t timestamp;
    uint8_t data[4];
} CANPayload;

_Static_assert(sizeof(CANPayload) == 8, "CANPayload must be exactly 8 bytes");

int main(void)
{
    printf("== Macro expansion pitfalls ==\n");
    printf("DOUBLE_BROKEN(1 + 2) = %d (should be 5, not 6)\n", DOUBLE_BROKEN(1 + 2));
    printf("DOUBLE_FIXED(1 + 2) = %d (should be 6)\n", DOUBLE_FIXED(1 + 2));

    printf("\n== Multi-statement macro ==\n");
    if (1)
        LOG("Hello");
    else
        printf("(never reached)\n");

    printf("\n== assert ==\n");
    assert(1 == 1);
    printf("assert(1 == 1) passed\n");

    printf("\n== sizeof check ==\n");
    printf("sizeof(CANPayload) = %zu (should be 8)\n", sizeof(CANPayload));

    printf("\n== SQ(i++) pitfall (undefined behavior) ==\n");
    int i = 0;
    printf("Before: i = %d\n", i);
    int result = SQ_MACRO(i++);
    printf("After SQ_MACRO(i++): i = %d, result = %d\n", i, result);
    printf("(Note: SQ_MACRO(i++) is undefined behavior; do not use in real code)\n");

    return 0;
}
```

**Predict: Is `DOUBLE_BROKEN(1 + 2)` 5 (not 6)? Does it become 6 when you add parentheses? Does the size check with `_Static_assert` pass?**

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic preprocessor_demo.c -o preprocessor_demo && ./preprocessor_demo
```

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: -->
```
preprocessor_demo.c: In function ‘main’:
preprocessor_demo.c:49:28: warning: operation on ‘i’ may be undefined [-Wsequence-point]
   49 |     int result = SQ_MACRO(i++);
      |                            ^
preprocessor_demo.c:11:29: note: in definition of macro ‘SQ_MACRO’
   11 | #define SQ_MACRO(x) ((x) * (x))
      |                             ^
== Macro expansion pitfalls ==
DOUBLE_BROKEN(1 + 2) = 5 (should be 5, not 6)
DOUBLE_FIXED(1 + 2) = 6 (should be 6)

== Multi-statement macro ==
[LOG] Hello
      (logged)

== assert ==
assert(1 == 1) passed

== sizeof check ==
sizeof(CANPayload) = 8 (should be 8)

== SQ(i++) pitfall (undefined behavior) ==
Before: i = 0
After SQ_MACRO(i++): i = 2, result = 0
(Note: SQ_MACRO(i++) is undefined behavior; do not use in real code)
```

</details>

Check the following.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. `DOUBLE_BROKEN(1 + 2)` is 5 (not 6).
2. `_Static_assert` lets the compilation pass (`CANPayload` is indeed 8 bytes).
3. The multi-statement macro works correctly even inside `if`.
4. With `SQ_MACRO(i++)`, `i` becomes 2 (it is undefined behavior, but the compiler gives a warning).

</details>

Next, also try the following.

- Assertions disappear with `-DNDEBUG`: `gcc -std=c11 -DNDEBUG preprocessor_demo.c -o preprocessor_demo_ndebug && ./preprocessor_demo_ndebug`
- Check the expansion with `-E`: `gcc -E -std=c11 preprocessor_demo.c | head -50`
- A failing `_Static_assert`: look at the compiler error message when the struct size is not 8 bytes.

## Common pitfalls

**A macro is expanded into an unexpected form**

Check whether parentheses are missing, and check the precedence. Look at the real expanded form with `gcc -E`, and then fix the macro.

**The control flow of a multi-statement macro breaks**

Wrap it in `do { ... } while (0)`, and add the semicolon at the call site.

**Writing a side effect inside `assert` causes a bug under NDEBUG**

Do side effects outside the assertion, and keep only the condition check inside the assertion.

**Compilation stops when a size already fixed by `_Static_assert` is exceeded**

When you extend a struct, update the `_Static_assert` as well.

**`_Static_assert` cannot be used in C99**

Compile with `-std=c11` or `-std=c17`. Or you can substitute it with the GNU extension attribute `__attribute__((error()))` and similar.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c12_preprocessor` — preprocessor and debugging

```bash
./drill run c12
```

If you get stuck, use `./drill hint c12`. From the exercise side, `./drill read c12` brings you back to this chapter.

## References

- [cppreference: Preprocessor](https://en.cppreference.com/w/c/preprocessor)
- [cppreference: assert.h](https://en.cppreference.com/w/c/error/assert)
- [cppreference: _Static_assert](https://en.cppreference.com/w/c/keyword/_Static_assert)

---

Previous chapter → [11. Endianness and serialization](11_endianness_and_serialization.md)

The C track ends here. [Back to the chapters of the C course](README.md)
