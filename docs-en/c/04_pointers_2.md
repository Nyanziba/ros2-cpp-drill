# 4. Pointers 2 — arrays and pointer arithmetic

> **Goal of this chapter**: When you pass a C array as a function argument, it is converted ("decays") into a pointer. As a result, `sizeof(array)` can no longer give the size of the whole array, and the function loses the means to know "how many elements there are". Pointer arithmetic `p + 1` moves not "1 byte" but "the size of the pointed-to type", so `int *` and `char *` behave differently. Out-of-bounds access is not stopped by the C compiler or runtime, and it silently corrupts memory. With Address Sanitizer, you can catch this kind of violation.

## 4.1 An array name decays to a pointer to its first element

**When an array is used in an expression, it is automatically converted to "a pointer to its first element".**

```c
int arr[5] = {10, 20, 30, 40, 50};
int *p = arr;  // arr decays to &arr[0]
```

`arr` and `&arr[0]` point to the same address.

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// array_decay.c
#include <stdio.h>

int main(void)
{
    int arr[5] = {10, 20, 30, 40, 50};
    int *p = arr;  // arr decays to &arr[0]

    printf("arr = %p\n", (void *)arr);
    printf("&arr[0] = %p\n", (void *)&arr[0]);
    printf("p = %p\n", (void *)p);
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic array_decay.c -o array_decay && ./array_decay
```

</details>

```
arr = 0x7ffffffc5950
&arr[0] = 0x7ffffffc5950
p = 0x7ffffffc5950
```

**But there are exceptions.** With `sizeof(array)` and `&array`, it does not decay.

```c
sizeof(arr)       // the size of the whole array (does not decay)
&arr              // a pointer to the array (does not decay)
```

On the other hand, when you pass an array to a function, it always decays.

## 4.2 Pointer arithmetic — the step depends on the type

**`p + 1` does not "advance 1 byte". It "advances the size of the pointed-to type".**

For example, compare `int *` and `char *`.

```c
int int_arr[3] = {100, 200, 300};
int *int_p = int_arr;

char char_arr[3] = {'a', 'b', 'c'};
char *char_p = char_arr;
```

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// pointer_step.c
#include <stdio.h>

int main(void)
{
    int int_arr[3] = {100, 200, 300};
    int *int_p = int_arr;

    char char_arr[3] = {'a', 'b', 'c'};
    char *char_p = char_arr;

    printf("int_p = %p\n", (void *)int_p);
    printf("int_p + 1 = %p\n", (void *)(int_p + 1));
    printf("difference: %td bytes (sizeof(int) = %zu)\n",
           (char *)(int_p + 1) - (char *)int_p, sizeof(int));

    printf("char_p = %p\n", (void *)char_p);
    printf("char_p + 1 = %p\n", (void *)(char_p + 1));
    printf("difference: %td bytes (sizeof(char) = %zu)\n",
           (char *)(char_p + 1) - (char *)char_p, sizeof(char));
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic pointer_step.c -o pointer_step && ./pointer_step
```

</details>

When you advance `int_p`:

```
int_p = 0x7ffffffc5938
int_p + 1 = 0x7ffffffc593c
difference: 4 bytes (sizeof(int) = 4)
```

When you advance `char_p`:

```
char_p = 0x7ffffffc5945
char_p + 1 = 0x7ffffffc5946
difference: 1 bytes (sizeof(char) = 1)
```

**Pointer arithmetic counts in units of "elements".** Thanks to this mechanism, you can access the elements of an array in order.

## 4.3 `sizeof(array)` and `sizeof(pointer)`

**An array and a pointer to its first element are different things.**

```c
int my_arr[5];
int *my_ptr = my_arr;

printf("sizeof(my_arr) = %zu\n", sizeof(my_arr));  // 20 (5 * 4)
printf("sizeof(my_ptr) = %zu\n", sizeof(my_ptr));  // 8 (pointer size)
```

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// array_size.c
#include <stdio.h>

int main(void)
{
    int my_arr[5];
    int *my_ptr = my_arr;

    printf("sizeof(my_arr) = %zu\n", sizeof(my_arr));  // 20 (5 * 4)
    printf("sizeof(my_ptr) = %zu\n", sizeof(my_ptr));  // 8 (pointer size)

    size_t num_elements = sizeof(my_arr) / sizeof(my_arr[0]);  // 5
    printf("num_elements = %zu\n", num_elements);
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic array_size.c -o array_size && ./array_size
```

</details>

Measured values:

```
sizeof(my_arr) = 20
sizeof(my_ptr) = 8
```

**To calculate the number of elements of an array:**

```c
size_t num_elements = sizeof(my_arr) / sizeof(my_arr[0]);  // 5
```

Measured values:

```
num_elements = 5
```

This calculation is valid "only when it acts directly on an array". You cannot use it on a pointer.

## 4.4 When you pass an array to a function, the number of elements is lost

**When you pass an array to a function, it automatically decays to a pointer. As a result, you can no longer tell the number of elements.**

```c
void print_array_broken(int *arr)
{
    printf("sizeof(arr) = %zu\n", sizeof(arr));  // 8 (pointer size)
}

int main(void)
{
    int data[5] = {10, 20, 30, 40, 50};
    printf("sizeof(data) = %zu\n", sizeof(data));  // 20 (the whole array)
    print_array_broken(data);  // it decays here!
}
```

Measured values:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// array_decay_broken.c
#include <stdio.h>

void print_array_broken(int *arr)
{
    printf("sizeof(arr) = %zu\n", sizeof(arr));  // 8 (pointer size)
}

int main(void)
{
    int data[5] = {10, 20, 30, 40, 50};
    printf("sizeof(data) = %zu\n", sizeof(data));  // 20 (the whole array)
    print_array_broken(data);  // it decays here!
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic array_decay_broken.c -o array_decay_broken && ./array_decay_broken
```

</details>

```
sizeof(data) = 20
sizeof(arr) = 8
```

**The function cannot know the number of elements, so you need to receive the "number of elements" as a separate argument.**

```c
#include <stdio.h>
#include <stddef.h>   /* size_t */

void print_array_safe(const int *arr, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        printf("arr[%zu] = %d\n", i, arr[i]);
    }
}

int main(void)
{
    int data[5] = {10, 20, 30, 40, 50};
    print_array_safe(data, sizeof(data) / sizeof(data[0]));
}
```

Measured values:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// print_array_safe.c
#include <stdio.h>
#include <stddef.h>   /* size_t */

void print_array_safe(const int *arr, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        printf("arr[%zu] = %d\n", i, arr[i]);
    }
}

int main(void)
{
    int data[5] = {10, 20, 30, 40, 50};
    print_array_safe(data, sizeof(data) / sizeof(data[0]));
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic print_array_safe.c -o print_array_safe && ./print_array_safe
```

</details>

```
arr[0] = 10
arr[1] = 20
arr[2] = 30
arr[3] = 40
arr[4] = 50
```

**In microcontroller control, you must never lose the buffer size.** Always pass the number of elements as an argument.

## 4.5 Array subscript is sugar for `*(a + i)`

**`a[i]` and `*(a + i)` are the same at the compiler level.**

```c
int test_arr[5] = {10, 20, 30, 40, 50};
printf("test_arr[2] = %d\n", test_arr[2]);
printf("*(test_arr + 2) = %d\n", *(test_arr + 2));
```

Measured values:

```
test_arr[2] = 30
*(test_arr + 2) = 30
```

Whether it is an array or a pointer, a subscript operation is "pointer arithmetic + dereference" inside.

## 4.6 Out-of-bounds access — it breaks silently, and ASAN catches it

**C has no "array bounds check". An out-of-bounds read just returns a garbage value.**

```c
int arr[5] = {10, 20, 30, 40, 50};
printf("arr[5] = %d\n", arr[5]);  // out of bounds (undefined behavior)
```

Running with `gcc -std=c99 -Wall -Wextra -Wpedantic` gives:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// out_of_bounds.c
#include <stdio.h>

int main(void)
{
    int arr[5] = {10, 20, 30, 40, 50};
    printf("arr[5] = %d\n", arr[5]);  // out of bounds (undefined behavior)
    printf("arr[6] = %d\n", arr[6]);
    printf("arr[7] = %d\n", arr[7]);
    return 0;
}
```

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic out_of_bounds.c -o out_of_bounds && ./out_of_bounds
for run in 1 2 3; do echo "=== Run $run ==="; ./out_of_bounds; done
gcc -std=c99 -Wall -Wextra -Wpedantic -g -fsanitize=address out_of_bounds.c -o out_of_bounds_asan && ./out_of_bounds_asan 2>&1 | head -n 16
```

</details>

```
arr[5] = 32767
arr[6] = 525268992
arr[7] = 1438676869
```

**Important: the values change if you run it several times.** A different garbage value comes out every time.

The results of the next 3 runs:

```
=== Run 1 ===
arr[5] = 32767
arr[6] = -1817045504
arr[7] = -1489959779

=== Run 2 ===
arr[5] = 32767
arr[6] = 169699840
arr[7] = -883402572

=== Run 3 ===
arr[5] = 32767
arr[6] = 1834489600
arr[7] = 565344630
```

The values are different every time, so you must not report these values as "measured values".

**Detect it with ASAN (Address Sanitizer).**

Run again with `gcc -std=c99 -Wall -Wextra -Wpedantic -g -fsanitize=address`:

```
=================================================================
==28==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffffcf00034 at pc 0x55555555544c bp 0x7ffffffc5910 sp 0x7ffffffc5900
READ of size 4 at 0x7ffffcf00034 thread T0
    #0 0x55555555544b in main /w/out_of_bounds.c:7
    #1 0x7ffffef031c9  (/lib/x86_64-linux-gnu/libc.so.6+0x2a1c9) (BuildId: a4a7992a8e66555c8141ab2a08a8465ff6e0ea65)
    #2 0x7ffffef0328a in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x2a28a) (BuildId: a4a7992a8e66555c8141ab2a08a8465ff6e0ea65)
    #3 0x555555555184 in _start (/w/out_of_bounds_asan+0x1184) (BuildId: b1df6c1edf52925bfed4cb5e81a8731ef34b6c52)

Address 0x7ffffcf00034 is located in stack of thread T0 at offset 52 in frame
    #0 0x555555555258 in main /w/out_of_bounds.c:5

  This frame has 1 object(s):
    [32, 52) 'arr' (line 6) <== Memory access at offset 52 overflows this variable
HINT: this may be a false positive if your program uses some custom stack unwind mechanism, swapcontext or vfork
      (longjmp and C++ exceptions *are* supported)
SUMMARY: AddressSanitizer: stack-buffer-overflow /w/out_of_bounds.c:7 in main
```

**Important information:**

- `stack-buffer-overflow` — out-of-bounds access to a stack buffer
- `READ of size 4` — a 4-byte read (the size of `int`)
- `#0 0x55555555544b in main out_of_bounds.c:7` — the line where the violation happened

Without ASAN, this bug would be missed and would lead to data corruption with an unknown cause.

## Try it yourself

Check array decay, pointer arithmetic, the difference in `sizeof`, the loss in function arguments, and out-of-bounds access for yourself.

```c
// pointer_arithmetic.c
#include <stdio.h>
#include <string.h>

// safe sum function
int sum_array(const int *arr, size_t n)
{
    int total = 0;
    for (size_t i = 0; i < n; i++) {
        total += arr[i];
    }
    return total;
}

// out-of-bounds access demo
void out_of_bounds_demo(void)
{
    int arr[5] = {10, 20, 30, 40, 50};

    printf("  Valid: arr[0]=%d, arr[4]=%d\n", arr[0], arr[4]);
    printf("  Out of bounds:\n");
    printf("    arr[5]=%d\n", arr[5]);
    printf("    arr[6]=%d\n", arr[6]);
}

int main(void)
{
    printf("=== Array-to-pointer decay ===\n");
    int arr[5] = {10, 20, 30, 40, 50};
    int *p = arr;
    printf("arr == &arr[0]: %s\n", (void *)arr == (void *)&arr[0] ? "yes" : "no");
    printf("p points to arr[0]: %d\n", *p);

    printf("\n=== Pointer arithmetic ===\n");
    int int_arr[3] = {100, 200, 300};
    int *int_p = int_arr;
    printf("int: int_p = %p, int_p+1 = %p (diff: 4 bytes)\n",
           (void *)int_p, (void *)(int_p + 1));
    printf("sizeof(int) = %zu\n", sizeof(int));

    char char_arr[3] = {'a', 'b', 'c'};
    char *char_p = char_arr;
    printf("char: char_p = %p, char_p+1 = %p (diff: 1 byte)\n",
           (void *)char_p, (void *)(char_p + 1));
    printf("sizeof(char) = %zu\n", sizeof(char));

    printf("\n=== sizeof(array) vs sizeof(pointer) ===\n");
    int my_arr[5];
    int *my_ptr = my_arr;
    printf("sizeof(my_arr) = %zu\n", sizeof(my_arr));
    printf("sizeof(my_ptr) = %zu\n", sizeof(my_ptr));
    printf("Elements in my_arr: %zu\n", sizeof(my_arr) / sizeof(my_arr[0]));

    printf("\n=== Array subscript == pointer arithmetic ===\n");
    int test_arr[5] = {10, 20, 30, 40, 50};
    printf("test_arr[2] = %d\n", test_arr[2]);
    printf("*(test_arr + 2) = %d\n", *(test_arr + 2));

    printf("\n=== Safe array processing ===\n");
    int data[5] = {10, 20, 30, 40, 50};
    size_t n = sizeof(data) / sizeof(data[0]);
    int total = sum_array(data, n);
    printf("Sum of %zu elements: %d\n", n, total);

    printf("\n=== Out-of-bounds (without ASAN) ===\n");
    out_of_bounds_demo();
    printf("Program continues even after reading garbage...\n");

    return 0;
}
```

**Predict: Does the array name match the pointer as an address? Is the step different in pointer arithmetic on `int *` and `char *`? Does `sizeof(array)` not return the pointer size? Do `a[i]` and `*(a + i)` return the same value? Does an out-of-bounds read not stop the program?**

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic pointer_arithmetic.c -o pointer_arithmetic && ./pointer_arithmetic
```

<details markdown="1"><summary>Answer (actual output)</summary>

```
=== Array-to-pointer decay ===
arr == &arr[0]: yes
p points to arr[0]: 10

=== Pointer arithmetic ===
int: int_p = 0x7ffe66c32560, int_p+1 = 0x7ffe66c32564 (diff: 4 bytes)
sizeof(int) = 4
char: char_p = 0x7ffe66c325a4, char_p+1 = 0x7ffe66c325a5 (diff: 1 byte)
sizeof(char) = 1

=== sizeof(array) vs sizeof(pointer) ===
sizeof(my_arr) = 20
sizeof(my_ptr) = 8
Elements in my_arr: 5

=== Array subscript == pointer arithmetic ===
test_arr[2] = 30
*(test_arr + 2) = 30

=== Safe array processing ===
Sum of 5 elements: 150

=== Out-of-bounds (without ASAN) ===
  Valid: arr[0]=10, arr[4]=50
  Out of bounds:
    arr[5]=32767
    arr[6]=-705828608
  Program continues even after reading garbage...
```

</details>

Check these 3 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. The array name and `&arr[0]` are the same address.
2. Arithmetic on an `int` pointer moves 4 bytes, and on a `char` pointer it moves 1 byte.
3. An out-of-bounds read does not stop the program and returns a garbage value.

</details>

Next, detect it with ASAN.

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic -g -fsanitize=address pointer_arithmetic.c -o pointer_arithmetic_asan && ./pointer_arithmetic_asan 2>&1 | head -40
```

<details markdown="1"><summary>Answer (actual output)</summary>

```
=================================================================
==19==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffffcf00034 at pc 0x555555556560 bp 0x7ffffffc5620 sp 0x7ffffffc5610
READ of size 4 at 0x7ffffcf00034 thread T0
    #0 0x55555555655f in out_of_bounds_demo /w/pointer_arithmetic.c:22
    #1 0x555555556eb7 in main /w/pointer_arithmetic.c:66
    #2 0x7ffffef031c9  (/lib/x86_64-linux-gnu/libc.so.6+0x2a1c9) (BuildId: a4a7992a8e66555c8141ab2a08a8465ff6e0ea65)
    #3 0x7ffffef0328a in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x2a28a) (BuildId: a4a7992a8e66555c8141ab2a08a8465ff6e0ea65)
    #4 0x5555555561e4 in _start (/w/pointer_arithmetic_asan+0x21e4) (BuildId: 3246c11b1107d70233fca6dd37dcfd7bc6ef23c5)

Address 0x7ffffcf00034 is located in stack of thread T0 at offset 52 in frame
    #0 0x555555556341 in out_of_bounds_demo /w/pointer_arithmetic.c:17

  This frame has 1 object(s):
    [32, 52) 'arr' (line 18) <== Memory access at offset 52 overflows this variable
HINT: this may be a false positive if your program uses some custom stack unwind mechanism, swapcontext or vfork
      (longjmp and C++ exceptions *are* supported)
SUMMARY: AddressSanitizer: stack-buffer-overflow /w/pointer_arithmetic.c:22 in out_of_bounds_demo
Shadow bytes around the buggy address:
  0x7ffffceffd80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7ffffceffe00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7ffffceffe80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7ffffcefff00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7ffffcefff80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x7ffffcf00000: f1 f1 f1 f1 00 00[04]f3 f3 f3 f3 f3 00 00 00 00
  0x7ffffcf00080: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7ffffcf00100: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7ffffcf00180: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7ffffcf00200: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7ffffcf00280: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07 
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
```

</details>

ASAN detected it. You can see `stack-buffer-overflow` and `READ of size 4`.

Next, try the following.

- Write a function that takes an array, and confirm that `sizeof(arr)` becomes the "pointer size"
- Measure the step of pointer arithmetic for an `int` array and a `char` array
- Run it several times without ASAN, and confirm that the garbage value is different each time

## Common pitfalls

**Taking `sizeof(arr)` inside a function to calculate the array size**
When an array is passed to a function, it decays to a pointer, so `sizeof` returns the pointer size (usually 8 bytes). Receive the number of elements as a separate argument.

**The result of pointer arithmetic is different from what you expected**
Check that `p + 1` advances not "1 byte" but "the size of the pointed-to type". It differs between `int *` and `char *`.

**The value changes every time with out-of-bounds access**
You are reading uninitialized memory, so the value is different on each run. It is dangerous to assume "this value will appear". Use ASAN.

**The difference between `p[i]` and `*(p + i)`**
In C they are the same. Whichever you use, the compiler generates the same code.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c04_pointer_array` — arrays and pointer arithmetic

```bash
./drill run c04
```

If you get stuck, use `./drill hint c04`. From the exercise side, `./drill read c04` brings you back to this chapter.

## References

- [cppreference: Arrays](https://en.cppreference.com/w/c/language/array)
- [cppreference: Pointer arithmetic](https://en.cppreference.com/w/c/language/operator_arithmetic)
- [Address Sanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)

---

Previous chapter → [3. Pointers 1 — addresses and dereferencing](03_pointers_1.md)

Next chapter → [5. Strings](05_strings.md)
