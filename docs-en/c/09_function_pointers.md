# 9. Function pointers

> **Goal of this chapter**: Function pointers are useful for callbacks, table-driven dispatch, and driver abstraction. You learn how to read the declaration, function decay, and the importance of NULL checks through implementation.

## 9.1 Declaring and reading a function pointer

**The declaration of a function pointer is very different depending on the position of the parentheses.**

```c
int (*fp)(int);      // a pointer to a function that takes an int and returns an int
int *fp(int);        // a function that takes an int and returns an int* (a function, not a pointer)
```

When you "read a declaration", look at where the `*` is.

| Declaration | How to read | Meaning |
| --- | --- | --- |
| `int (*fp)(int)` | `(*fp)` is a function pointer | a pointer to a function that takes an `int` and returns an `int` |
| `int *fp(int)` | `fp(int)` is a function | a function that takes an `int` and returns an `int*` |

Depending on the position of the parentheses, "a pointer" and "a function that returns a pointer" are swapped.

## 9.2 Function name decay

**In C, a function name automatically decays to a function pointer.** Both `&func` and `func` are valid.

```c
int add(int a, int b) {
    return a + b;
}

typedef int (*Operation)(int, int);

Operation op1 = add;      // decay
Operation op2 = &add;     // explicit address-of
```

Both are the same. The latter makes the intent clearer, but the former is more common by convention.

```c
printf("op1(3, 4) = %d\n", op1(3, 4));    // 7
printf("(*op2)(3, 4) = %d\n", (*op2)(3, 4));  // 7
```

You can call it either way.

## 9.3 Make it readable with typedef

**Function pointer types are complex, so giving them an alias with `typedef` makes them easier to read.**

```c
typedef int (*Handler)(int param);

Handler h = some_function;
h(42);
```

Without `typedef`, it becomes wordy like this.

```c
int (*h)(int param) = some_function;  // hard to read
h(42);
```

In real work, always use `typedef`.

## 9.4 Callback: the qsort example

**The C standard library `qsort` takes a comparison function as a callback.**

```c
#include <stdlib.h>

int cmp_asc(const void *a, const void *b) {
    return *(int *)a - *(int *)b;
}

int cmp_desc(const void *a, const void *b) {
    return *(int *)b - *(int *)a;
}

int main(void)
{
    int arr[] = {5, 2, 8, 1, 9, 3};
    int n = sizeof(arr) / sizeof(arr[0]);

    qsort(arr, n, sizeof(int), cmp_asc);
    // arr[] = {1, 2, 3, 5, 8, 9}

    qsort(arr, n, sizeof(int), cmp_desc);
    // arr[] = {9, 8, 5, 3, 2, 1}
    return 0;
}
```

Just by passing a comparison function, the same `qsort` handles both ascending and descending order. This is the basic form of **polymorphism**.

## 9.5 Table-driven dispatch

**Command handling is hard to extend with a switch statement. With an array of function pointers (table-driven), you only add a handler.**

```c
typedef int (*Handler)(int param);

typedef struct {
    int id;
    Handler handler;
} CommandEntry;

int handle_start(int param) { ... }
int handle_stop(int param) { ... }
int handle_reset(int param) { ... }

CommandEntry cmd_table[] = {
    {1, handle_start},
    {2, handle_stop},
    {3, handle_reset},
    {0, NULL}  // end marker
};

int dispatch_command(int cmd_id, int param) {
    for (int i = 0; cmd_table[i].handler != NULL; i++) {
        if (cmd_table[i].id == cmd_id) {
            return cmd_table[i].handler(param);
        }
    }
    return -1;  // unknown command
}
```

Flow at run time: `dispatch_command(1, 100)` → scan `cmd_table` → call `handle_start`.

Microcontroller command dispatch is exactly this shape.

## 9.6 The danger of a NULL function pointer

**If you call an unregistered (NULL) function pointer, the program crashes immediately with a Segmentation Fault.** This is the point that bites hardest in real work.

```c
typedef int (*Handler)(int);

Handler h = NULL;

// never call the following!
// int result = h(42);  // ← Segmentation Fault
```

**Always check for NULL before calling:**

```c
if (h != NULL) {
    int result = h(42);
} else {
    printf("Handler is not registered\n");
}
```

It is the same when you look up a handler in a table.

```c
int dispatch_command(int cmd_id, int param) {
    for (int i = 0; cmd_table[i].handler != NULL; i++) {
        if (cmd_table[i].id == cmd_id) {
            // here handler is always non-NULL
            return cmd_table[i].handler(param);
        }
    }
    // handling for when it is not found
    return -1;
}
```

By placing `{0, NULL}` at the end, the loop ends safely.

## 9.7 Put handlers in a struct to abstract a driver

**When you provide the same interface to several devices, pack the handlers into a struct.**

```c
typedef struct {
    int (*open)(void);
    int (*read)(void);
    int (*write)(int);
    int (*close)(void);
} DeviceOps;

// Device A and Device B have different implementations,
// but can be operated through the same DeviceOps interface
DeviceOps dev_a = {
    .open = device_a_open,
    .read = device_a_read,
    .write = device_a_write,
    .close = device_a_close
};

// operate both devices with the same code
dev_a.open();
int val = dev_a.read();
dev_a.write(val + 10);
dev_a.close();
```

It is a standard pattern for hiding device-specific implementations such as motor drivers, CAN transceivers, and sensors.

## Try it yourself

This code demonstrates all the features of function pointers.

```c
// func_pointer_all.c
#include <stdio.h>
#include <stdlib.h>

typedef int (*Operation)(int, int);
typedef int (*Handler)(int);

// arithmetic functions
int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }
int subtract(int a, int b) { return a - b; }

// command handlers
int handle_start(int param) { printf("  [START] param=%d\n", param); return 0; }
int handle_stop(int param) { printf("  [STOP] param=%d\n", param); return 0; }
int handle_reset(int param) { printf("  [RESET] param=%d\n", param); return 0; }

// table-driven command entry
typedef struct {
    int id;
    Handler handler;
} CommandEntry;

CommandEntry cmd_table[] = {
    {1, handle_start},
    {2, handle_stop},
    {3, handle_reset},
    {0, NULL}
};

int dispatch_command(int cmd_id, int param) {
    for (int i = 0; cmd_table[i].handler != NULL; i++) {
        if (cmd_table[i].id == cmd_id) {
            return cmd_table[i].handler(param);
        }
    }
    printf("  Unknown command: %d\n", cmd_id);
    return -1;
}

// comparison functions for qsort
int cmp_asc(const void *a, const void *b) {
    return *(int *)a - *(int *)b;
}

int cmp_desc(const void *a, const void *b) {
    return *(int *)b - *(int *)a;
}

int main(void)
{
    printf("== Function pointers: decay and typedef ==\n");
    Operation op1 = add;      // decay
    Operation op2 = &multiply;
    printf("op1(3, 4) = %d\n", op1(3, 4));
    printf("op2(3, 4) = %d\n", op2(3, 4));
    printf("\n");

    printf("== Callback: qsort ==\n");
    int arr[] = {5, 2, 8, 1, 9, 3};
    int n = sizeof(arr) / sizeof(arr[0]);
    printf("Original array: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    qsort(arr, n, sizeof(int), cmp_asc);
    printf("Ascending: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    qsort(arr, n, sizeof(int), cmp_desc);
    printf("Descending: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n\n");

    printf("== Table-driven dispatch ==\n");
    dispatch_command(1, 100);
    dispatch_command(2, 200);
    dispatch_command(3, 300);
    dispatch_command(99, 400);
    printf("\n");

    printf("== NULL check ==\n");
    Handler h = NULL;
    if (h != NULL) {
        h(42);
    } else {
        printf("Handler is NULL. Calling it would crash.\n");
    }
    printf("\n");

    printf("Done\n");
    return 0;
}
```

**Predict: Does qsort sort in both ascending and descending order? Does the table dispatch call the right handler? Does the NULL check work?**

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic func_pointer_all.c -o func_pointer_all && ./func_pointer_all
```

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: -->
```
== Function pointers: decay and typedef ==
op1(3, 4) = 7
op2(3, 4) = 12

== Callback: qsort ==
Original array: 5 2 8 1 9 3 
Ascending: 1 2 3 5 8 9 
Descending: 9 8 5 3 2 1 

== Table-driven dispatch ==
  [START] param=100
  [STOP] param=200
  [RESET] param=300
  Unknown command: 99

== NULL check ==
Handler is NULL. Calling it would crash.

Done
```

</details>

Check these 3 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. qsort sorts correctly in ascending and descending order.
2. The table dispatch calls the handler that matches the command ID.
3. The NULL check works and prevents a crash.

</details>

Next, check what happens when you call a NULL function pointer.

```c
// null_func_crash.c
#include <stdio.h>

typedef int (*Handler)(int);

int main(void)
{
    printf("Calling a NULL function pointer...\n");
    Handler h = NULL;
    int result = h(42);  // ← Segmentation Fault
    printf("This will not be printed\n");
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic null_func_crash.c -o null_func_crash && ./null_func_crash ; echo "Exit code: $?"
```

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: tty=yes filter="tail -n 3" -->
```
Calling a NULL function pointer...
Segmentation fault
Exit code: 139
```

</details>

**A Segmentation fault appears and the process ends immediately. Exit code 139 = 128 + 11 (signal SIGSEGV).** This is the point you most need to be careful about in real work.

## Common pitfalls

**Confusing the position of the parentheses**
`int (*fp)(int)` and `int *func(int)` are completely different. Check the position of the parentheses carefully.

**A crash from a forgotten NULL check**
Calling a handler that is not found gives a Segmentation Fault. At design time, make clear where the NULL checks go.

**The types do not match**
If the type of the callback function is different, the compiler gives an error. Make the type clear with `typedef`.

**Overflow in a qsort comparison function**
`return a - b;` overflows for large values. It is safer to split the comparison with `if`.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c09_function_pointer` — function pointers and table-driven dispatch

```bash
./drill run c09
```

If you get stuck, use `./drill hint c09`. From the exercise side, `./drill read c09` brings you back to this chapter.

## References

- [cppreference: Function pointers](https://en.cppreference.com/w/c/language/function_declaration)
- [cppreference: qsort](https://en.cppreference.com/w/c/algorithm/qsort)

---

Previous chapter → [8. Manual memory management](08_manual_memory_management.md)
Next chapter → [10. volatile and interrupt safety](10_volatile_and_interrupt_safety.md)
