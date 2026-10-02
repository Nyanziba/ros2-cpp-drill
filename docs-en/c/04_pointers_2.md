# 4. Pointers 2 — arrays and pointer arithmetic

> **Goal of this chapter**: When you pass a C array as a function argument, it is converted ("decays") into a pointer. As a result, `sizeof(array)` can no longer give the size of the whole array, and the function loses the means to know "how many elements there are". Pointer arithmetic `p + 1` moves not "1 byte" but "the size of the pointed-to type", so `int *` and `char *` behave differently. Out-of-bounds access is not stopped by the C compiler or runtime, and it silently corrupts memory. With Address Sanitizer, you can catch this kind of violation.

## 4.1 An array name decays to a pointer to its first element

**When an array is used in an expression, it is automatically converted to "a pointer to its first element".**

```c
int arr[5] = {10, 20, 30, 40, 50};
int *p = arr;  // arr decays to &arr[0]
```

`arr` and `&arr[0]` point to the same address.

```
arr = 0x7ffd6afc8570
&arr[0] = 0x7ffd6afc8570
p = 0x7ffd6afc8570
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

When you advance `int_p`:

```
int_p = 0x7ffd6afc8564
int_p + 1 = 0x7ffd6afc8568
差分: 4 バイト（sizeof(int) = 4）
```

When you advance `char_p`:

```
char_p = 0x7ffd6afc85c5
char_p + 1 = 0x7ffd6afc85c6
差分: 1 バイト（sizeof(char) = 1）
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

Measured values:

```
sizeof(my_arr) = 20 (entire array)
sizeof(my_ptr) = 8 (just pointer)
```

**To calculate the number of elements of an array:**

```c
size_t num_elements = sizeof(my_arr) / sizeof(my_arr[0]);  // 5
```

Measured values:

```
my_arr / sizeof(my_arr[0]) = 5 (number of elements)
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

```
In main:
  sizeof(data) = 20
  Elements: 5
Inside function:
  sizeof(arr) = 8 (not the array size!)
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

```
Inside function:
  n = 5 (elements)
  Printing elements:
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

```
Valid access:
  arr[0] = 10
  arr[4] = 50

Out-of-bounds read (reads garbage from stack):
  arr[5] = 32767 (first read)
  arr[6] = 1416566784 (second read)
  arr[7] = -1432954244 (third read)
```

**Important: the values change if you run it several times.** A different garbage value comes out every time.

The results of the next 3 runs:

```
=== Run 1 ===
  arr[5] = 32767 (first read)
  arr[6] = 1416566784 (second read)
  arr[7] = -1432954244 (third read)

=== Run 2 ===
  arr[5] = 32765 (first read)
  arr[6] = -1105437440 (second read)
  arr[7] = 859231859 (third read)

=== Run 3 ===
  arr[5] = 32767 (first read)
  arr[6] = 958064896 (second read)
  arr[7] = -827176993 (third read)
```

The values are different every time, so you must not report these values as "measured values".

**Detect it with ASAN (Address Sanitizer).**

Run again with `gcc -std=c99 -Wall -Wextra -Wpedantic -g -fsanitize=address`:

```
==223446==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x77da24600034 at pc 0x6444b99e552d bp 0x7ffd2989f5e0 sp 0x7ffd2989f5d0
READ of size 4 at 0x77da24600034 thread T0
    #0 0x6444b99e552c in main /tmp/claude-1000/-home-nyanziba-hobby-program/bfbf933d-8b99-4e93-9360-43360e47ad4f/scratchpad/test_oob.c:13
    #1 0x77da2662a1c9 in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58
    #2 0x77da2662a28a in __libc_start_main_impl ../csu/libc-start.c:360
    #3 0x6444b99e51a4 in _start (/tmp/claude-1000/-home-nyanziba-hobby-program/bfbf933d-8b99-4e93-9360-43360e47ad4f/scratchpad/test_oob_asan+0x11a4) (BuildId: 61803c7afc749dea2da335ab22612443c55880fc)

Address 0x77da24600034 is located in stack of thread T0 at offset 52 in frame
    #0 0x6444b99e5278 in main /tmp/claude-1000/-home-nyanziba-hobby-program/bfbf933d-8b99-4e93-9360-43360e47ad4f/scratchpad/test_oob.c:4

  This frame has 1 object(s):
    [32, 52) 'arr' (line 6) <== Memory access at offset 52 overflows this variable
HINT: this may be a false positive if your program uses some custom stack unwind mechanism, swapcontext or vfork
      (longjmp and C++ exceptions *are* supported)
SUMMARY: AddressSanitizer: stack-buffer-overflow /tmp/claude-1000/-home-nyanziba-hobby-program/bfbf933d-8b99-4e93-9360-43360e47ad4f/scratchpad/test_oob.c:13 in main
```

**Important information:**

- `stack-buffer-overflow` — out-of-bounds access to a stack buffer
- `READ of size 4` — a 4-byte read (the size of `int`)
- `#0 0x6444b99e552c in main test_oob.c:13` — the line where the violation happened

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
==223529==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffd9eae6564 at pc 0x7f78fa00552d bp 0x7ffd9eae6490 sp 0x7ffd9eae6480
READ of size 4 at 0x7ffd9eae6564 thread T0
    #0 0x7f78fa00552c in main /tmp/claude-1000/-home-nyanziba-hobby-program/bfbf933d-8b99-4e93-9360-43360e47ad4f/scratchpad/test_oob.c:13 (BuildId: 61803c7afc749dea2da335ab22612443c55880fc)
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
