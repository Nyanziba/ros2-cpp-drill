# 8. Manual memory management

> **Goal of this chapter**: In C you allocate and free memory by hand. `malloc` may fail to allocate (it returns `NULL`), forgetting `free` causes a leak, and using memory after it is freed **breaks things without crashing** (so you do not notice). By always making clear "who owns this pointer", you can reduce bugs. Seeing real errors with Address Sanitizer makes them stick in your memory.

## 8.1 `malloc` — dynamic allocation

**`malloc(size)` allocates `size` bytes and returns its address. On failure it returns `NULL`.**

```c
int *p = (int *)malloc(sizeof(int) * 5);
if (p == NULL) {
    fprintf(stderr, "malloc failed\n");
    return 1;
}
```

The return value of `malloc` is `void *`, and you use it with a cast. The size is calculated with `sizeof(int) * 5`.

Once the allocation succeeds, you can use it like an array through the pointer (`p[0]`, `p[1]`).
The output from actually running it is collected in "Try it yourself" in this chapter.

**`calloc(count, size)` allocates with initialization.** It fills the memory with 0.

```c
int *q = (int *)calloc(3, sizeof(int));
if (q == NULL) { /* ... */ }
// q[0], q[1], q[2] are all 0
```

The standard says `calloc` fills with 0. We check the measured result in "Try it yourself".

## 8.2 Handling allocation failure

**With `malloc`, always consider the possibility of failure.** Running out of memory can happen in embedded systems.

```c
int *arr = (int *)malloc(sizeof(int) * 1000);
if (arr == NULL) {
    // handling for when the allocation fails
    fprintf(stderr, "Failed to allocate memory\n");
    return -1;
}
// use arr...
free(arr);
```

If you skip the failure handling, dereferencing a `NULL` pointer crashes the program.

## 8.3 `free` — releasing

**`free(p)` returns the memory allocated with `malloc`.**

```c
int *p = (int *)malloc(sizeof(int));
*p = 42;
printf("*p = %d\n", *p);
free(p);
// p is no longer valid
```

After `free`, `p` becomes "a variable that holds an address you must no longer use".
The value does not disappear, so you can use it by mistake. That is the story in 8.5.

**Multiple allocations and frees.**

```c
int *arr[3];
for (int i = 0; i < 3; i++) {
    arr[i] = (int *)malloc(sizeof(int) * 2);
    arr[i][0] = i * 10;
}
for (int i = 0; i < 3; i++) {
    free(arr[i]);
}
```

**You need a `free` for each element.** It is not true that one `free` of the array itself is enough.
The output from actually running it is in "Try it yourself".

## 8.4 Memory leaks and double free

**Leak: you do not free the memory you allocated.**

```c
void leaky_function(void)
{
    int *p = (int *)malloc(sizeof(int) * 100);
    // ... processing ...
    return;  // forgot free! memory leak
}
```

If it is called many times, the memory runs out.

**Double free: you `free` the same pointer twice.**

```c
int *p = (int *)malloc(sizeof(int));
free(p);
free(p);  // double free → crash or undefined behavior
```

## 8.5 Use-after-free

**If you use a pointer after `free`, it can crash, or read and write unexpected values.**

```c
int *p = (int *)malloc(sizeof(int));
*p = 42;
free(p);
printf("*p = %d\n", *p);  // use after free. This is the tricky part
```

**Contrary to what you expect, this usually does not crash.** These are measured values from running the same executable 5 times.

```
*p = 1599802780     exit=0
*p = -1009684065    exit=0
*p = -1092865557    exit=0
*p = 2119807940     exit=0
*p = 22557486       exit=0
```

A different value comes out every time, and **it exits normally every time.** This is the most dangerous property of use-after-free.
If it crashed, you would notice, but it does not crash, so you cannot notice.
Tests pass too, and it breaks only when it hits a different value in the field.

So the only way is to **make it visible with tools**.
First, `gcc` itself may notice (measured).

```
warning: pointer ‘p’ used after ‘free’ [-Wuse-after-free]
note: call to ‘free’ here
```

And Address Sanitizer stops it for sure. The actual detection message:

```
==214035==ERROR: AddressSanitizer: heap-use-after-free on address 0x502000000010
READ of size 4 at 0x502000000010 thread T0
    #0 0x642ec0775345 in main
```

From the message you can identify "which address", "which size", and "which line of the main function".

## 8.6 Make ownership explicit

**For function arguments and return values, state "who owns it" in a comment.**

```c
// the caller is responsible for freeing the returned memory
int *create_array(size_t size)
{
    // ownership: caller must free
    return (int *)malloc(sizeof(int) * size);
}

void process_data(int *data, size_t size)
{
    // ownership: caller owns data, we don't free
    for (size_t i = 0; i < size; i++) {
        printf("%d\n", data[i]);
    }
}
```

If you make clear "who has ownership", you can prevent forgotten `free`, double free, and use-after-free.

## Try it yourself

Check how `malloc`, `calloc`, and `free` behave, and detect errors with Address Sanitizer.

```c
// memory_all.c
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    printf("== malloc and free ==\n");
    int *p = (int *)malloc(sizeof(int) * 5);
    if (p == NULL) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    p[0] = 10;
    p[1] = 20;
    printf("p[0] = %d, p[1] = %d\n", p[0], p[1]);
    free(p);

    printf("\n== calloc (zeroed) ==\n");
    int *q = (int *)calloc(3, sizeof(int));
    if (q == NULL) {
        fprintf(stderr, "calloc failed\n");
        return 1;
    }
    printf("q[0] = %d, q[1] = %d, q[2] = %d\n", q[0], q[1], q[2]);
    free(q);

    printf("\n== Multiple allocations ==\n");
    int *arr[3];
    for (int i = 0; i < 3; i++) {
        arr[i] = (int *)malloc(sizeof(int) * 2);
        if (arr[i] == NULL) {
            fprintf(stderr, "malloc failed\n");
            return 1;
        }
        arr[i][0] = i * 10;
        arr[i][1] = i * 100;
    }
    for (int i = 0; i < 3; i++) {
        printf("arr[%d][0] = %d, arr[%d][1] = %d\n", i, arr[i][0], i, arr[i][1]);
        free(arr[i]);
    }

    printf("All cleaned up\n");
    return 0;
}
```

**Predict: Are the values allocated with `calloc` 0? Are different addresses assigned by several `malloc` calls? Can all of them be freed with `free`?**

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic memory_all.c -o memory_all && ./memory_all
```

<details markdown="1"><summary>Answer (actual output)</summary>

```
== malloc and free ==
p[0] = 10, p[1] = 20

== calloc (zeroed) ==
q[0] = 0, q[1] = 0, q[2] = 0

== Multiple allocations ==
arr[0][0] = 0, arr[0][1] = 0
arr[1][0] = 10, arr[1][1] = 100
arr[2][0] = 20, arr[2][1] = 200
All cleaned up
```

</details>

Next, detect the use-after-free with Address Sanitizer.

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic -fsanitize=address memory_uaf.c -o memory_uaf && ./memory_uaf
```

Code example (`memory_uaf.c`):

```c
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int *p = (int *)malloc(sizeof(int));
    *p = 42;
    free(p);
    printf("*p = %d\n", *p);  // use-after-free
    return 0;
}
```

Output of Address Sanitizer:

```
==ERROR: AddressSanitizer: heap-use-after-free on address 0x502000000010
READ of size 4 at 0x502000000010 thread T0
    #0 0x... in main

0x502000000010 is located 0 bytes inside of 4-byte region [0x502000000010,0x502000000014)
freed by thread T0 here:
    #0 0x... in free
    #1 0x... in main
```

It states clearly that the memory has already been freed.

Check these 3 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. The values allocated with `calloc` are all 0.
2. Different addresses are assigned by several `malloc` calls.
3. Address Sanitizer detects access to memory that was already freed with `free`.

</details>

Next, try the following.

- What happens if you call `free` again after `free` (double free)?
- Pass an abnormally large size (for example `1 << 30`) to `malloc` and observe the allocation failure

## Common pitfalls

**Forgetting that `malloc` may return `NULL`**
Running out of memory happens in embedded systems. Always check for `NULL`.

**Memory leak from a forgotten `free`**
Free the memory you allocated, and take responsibility for it. A large leak can hang the system.

**`free` the same pointer twice (double free)**
This is undefined behavior. Set the pointer to `NULL`, or manage it with a flag.

```c
free(p);
p = NULL;   /* this alone is enough */
```

**The standard says `free(NULL)` "does nothing".** So if you set `p` to `NULL`,
it is safe even if you accidentally call `free(p)` again. You do not need to write a guard
such as `if (p != NULL)`.

**The variable seems to still exist after the free**
A pointer is just an address. `free` only "returns the memory", and does not erase the pointer variable. Check the control flow so that nothing accesses it after the free.

**The output of Address Sanitizer is long**
Read it slowly. It lists "which address" and "malloc/free / where it was allocated".

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c08_malloc_free` — manual memory management

```bash
./drill run c08
```

If you get stuck, use `./drill hint c08`. From the exercise side, `./drill read c08` brings you back to this chapter.

## References

- [cppreference: Dynamic memory allocation](https://en.cppreference.com/w/c/memory)
- [GCC Address Sanitizer](https://gcc.gnu.org/wiki/AddressSanitizer)
- [CERT: MEM31-C](https://wiki.sei.cmu.edu/confluence/display/c/MEM31-C) — Free memory when no longer needed

---

Previous chapter → [7. Bit operations and register access](07_bit_operations_and_registers.md)
