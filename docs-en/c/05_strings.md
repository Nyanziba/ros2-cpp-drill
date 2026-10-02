# 5. Strings

> **Goal of this chapter**: A C string is a plain format: "an array + a NUL terminator". Because of the NUL terminator, `strlen` scans the string every time. `strcpy` is dangerous, and `strncpy` has a trap. By understanding how to deal with buffer sizes, and what is good about `snprintf`, you can prevent buffer overflows. This chapter is a basic of security.

## 5.1 The NUL terminator and `strlen`

**A C string is a convention: "an array of characters + ending with `'\0'`".**

```c
char s[] = "hello";  // memory layout: 'h', 'e', 'l', 'l', 'o', '\0'
```

`'\0'` is the number 0 treated as a character, and it is the mark that shows the end of the string (the NUL terminator).

**`strlen` scans every time until it finds the NUL.** You cannot know where the string ends until you find the NUL.

```c
char s[] = "hello";
size_t len = strlen(s);  // inside: counts 'h' 'e' 'l' 'l' 'o' and returns 5
```

Measured values:

```
s: hello, strlen(s): 5, sizeof(s): 6
```

`sizeof(s)` is 6. The reason is that "h e l l o NUL" is 6 bytes. `strlen` does not count the NUL and returns only 5.

## 5.2 How to count the buffer size

**`sizeof` of an array is "all bytes including the NUL". It is different from the number of characters.**

```c
char arr[] = "abc";
printf("sizeof(arr): %zu\n", sizeof(arr));  // 4 (a b c \0)
printf("strlen(arr): %zu\n", strlen(arr));  // 3 (a b c)
```

Measured values:

```
sizeof(arr): 4 (includes NUL)
strlen(arr): 3 (without NUL)
```

**How many characters fit in a buffer?**

If the buffer is `char buf[10]`, it can hold a string of at most 9 characters (the last 1 byte is for the NUL).

## 5.3 The danger of `strcpy`

**`strcpy` does not know the size of the destination buffer. It is dangerous.**

```c
char buffer[5];
strcpy(buffer, "hello");  // 'h' 'e' 'l' 'l' 'o' '\0' → needs 6 bytes
                           // buffer has only 5 bytes → buffer overflow!
```

Compiler warning:

```
warning: '__builtin_memcpy' writing 6 bytes into a region of size 5 overflows the destination
```

`strcpy` remains only in code from the 1990s. Do not use it in new code.

## 5.4 The trap of `strncpy`

**`strncpy` may not add a NUL terminator.** If the character limit in the argument leaves no room to place a NUL, it ends without a NUL.

**Do not try it with an uninitialized buffer.** What is in `buf[4]` is left to luck,
and if it happens to be 0, you cannot see the key point that "there is no NUL". **Fill it with known values first.**

```c
#include <stdio.h>
#include <string.h>

int main(void)
{
  /* Prepare 16 bytes, fill the first 10 bytes with 'X', and put a NUL at the 11th.
   * This way you can observe "how far it reads when there is no NUL"
   * without going outside the array. */
  char buf[16];
  memset(buf, 'X', 10);
  buf[10] = '\0';

  strncpy(buf, "test", 4);   /* Copy only 4 characters. No room is given for the NUL */

  printf("buf[0..4] = %c %c %c %c %c\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
  printf("numeric value of buf[4] = %d   ('X' = 88)\n", (unsigned char)buf[4]);
  printf("strlen(buf) = %zu   ← not 4\n", strlen(buf));
  printf("buf = \"%s\"\n", buf);
  return 0;
}
```

Measured values:

```
buf[0..4] = t e s t X
numeric value of buf[4] = 88   ('X' = 88)
strlen(buf) = 10   ← not 4
buf = "testXXXXXX"
```

**`strncpy` copied only 4 characters, but as a string it looks like 10 characters.**
It did not place a NUL, so both `strlen` and `printf("%s", ...)` keep running until the next NUL.

Here I placed a NUL at the 11th position myself, so it stopped at 10.
**Without it, it would read beyond the array** (undefined behavior).
This is how a buffer over-read happens.

The compiler also notices and warns (measured).

```
warning: ‘strncpy’ output truncated before terminating nul copying 4 bytes
from a string of the same length [-Wstringop-truncation]
```

**If you use `strncpy`, always place the NUL yourself.**

```c
char buf[10];
strncpy(buf, "test", sizeof(buf) - 1);
buf[sizeof(buf) - 1] = '\0';  // place the NUL manually
```

## 5.5 `snprintf` — safer

**`snprintf` takes the buffer size as an argument and terminates with a NUL automatically.**

```c
char buf[10];
int written = snprintf(buf, sizeof(buf), "hello world");
printf("snprintf wrote: %d bytes (limited to %zu)\n", written, sizeof(buf));
printf("buf: %s\n", buf);
```

Measured values:

```
snprintf wrote: 11 bytes (limited to 10)
buf: hello wor
```

The return value of `snprintf` is "the number of characters it wanted to write, not counting the NUL". If the return value is `sizeof(buf)` or more, it means the output was truncated. This tells you whether "it was written completely" or "it was cut off in the middle".

Compiler warning:

```
warning: 'hello world' directive output truncated writing 11 bytes into a destination of size 10
```

## Try it yourself

Check the NUL terminator, the scan of `strlen`, buffer size, `strcpy`, `strncpy`, and `snprintf` for yourself.

```c
// string_all.c
#include <stdio.h>
#include <string.h>

int main(void)
{
  // the NUL terminator, and the difference between sizeof and strlen
  printf("== NUL-terminated string ==\n");
  char s[] = "hello";
  printf("s: %s\n", s);
  printf("strlen(s): %zu, sizeof(s): %zu\n", strlen(s), sizeof(s));

  printf("\n== sizeof vs actual length ==\n");
  char arr[] = "abc";
  printf("sizeof(arr): %zu, strlen(arr): %zu\n", sizeof(arr), strlen(arr));

  // the trap of strncpy. An uninitialized buffer is left to luck, so fill it with 'X' first
  printf("\n== strncpy doesn't add NUL ==\n");
  char buf1[16];
  memset(buf1, 'X', 10);
  buf1[10] = '\0';
  strncpy(buf1, "test", 4);
  printf("After strncpy(buf1, \"test\", 4):\n");
  printf("  buf1[0]=%c, buf1[1]=%c, buf1[2]=%c, buf1[3]=%c, buf1[4]=%c\n",
         buf1[0], buf1[1], buf1[2], buf1[3], buf1[4]);
  printf("  strlen(buf1)=%zu  buf1=\"%s\"\n", strlen(buf1), buf1);

  // the safe way
  printf("\n== Safe: strncpy + manual NUL ==\n");
  char buf2[10];
  strncpy(buf2, "test", sizeof(buf2) - 1);
  buf2[sizeof(buf2) - 1] = '\0';
  printf("buf2: %s\n", buf2);

  // snprintf
  printf("\n== snprintf with size limit ==\n");
  char buf3[10];
  int written = snprintf(buf3, sizeof(buf3), "hello world");
  printf("Written %d bytes (size limit %zu), buf3: %s\n", written, sizeof(buf3), buf3);

  // strlen scans every time
  printf("\n== strlen is always linear ==\n");
  char long_str[] = "abcdefghij";
  printf("1st strlen: %zu, 2nd strlen: %zu\n", strlen(long_str), strlen(long_str));

  return 0;
}
```

**Predict: What is the difference between `strlen(s)` and `sizeof(s)`? Is `buf1` NUL-terminated? How many bytes could `snprintf` write?**

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic string_all.c -o string_all && ./string_all
```

<details markdown="1"><summary>Answer (actual output)</summary>

```
== NUL-terminated string ==
s: hello
strlen(s): 5, sizeof(s): 6

== sizeof vs actual length ==
sizeof(arr): 4, strlen(arr): 3

== strncpy doesn't add NUL ==
After strncpy(buf1, "test", 4):
  buf1[0]=t, buf1[1]=e, buf1[2]=s, buf1[3]=t, buf1[4]=X
  strlen(buf1)=10  buf1="testXXXXXX"

== Safe: strncpy + manual NUL ==
buf2: test

== snprintf with size limit ==
Written 11 bytes (size limit 10), buf3: hello wor

== strlen is always linear ==
1st strlen: 10, 2nd strlen: 10
```

</details>

Check these 3 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. `sizeof(s)` is 6 bytes including the NUL. `strlen(s)` is 5 bytes without the NUL.
2. `strncpy(buf1, "test", 4)` does not place a NUL, so in `buf1[4]` the `'X'` that was filled in stays as it is.
   That is why `strlen` returns 10 even though 4 characters were copied.
3. The return value 11 of `snprintf` is "the number of bytes needed to write it completely". The buffer is 10, so the output was truncated to "hello wor".

</details>

Next, try the following.

- Use `strcpy` instead of `strncpy`, and look at the buffer overflow warning
- Make it even shorter with `snprintf(buf3, 5, "hello world")`

## Common pitfalls

**`error: '__builtin_memcpy' writing N bytes into a region of size M`**
The buffer is too small for `strcpy` or `strcat`. Use `snprintf`.

**`strlen(s)` is different from what you expected**
The string may not end with a NUL. If it is right after using `strncpy`, place the NUL manually.

**You use `strncpy`, but `printf("%s", buf)` behaves strangely**
`strncpy` may not place a NUL. Always put `buf[size-1] = '\0'` at the end.

**The return value of `snprintf` is negative**
It is a format string error. Check the format (normally the return value is 0 or more).

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c05_string` — strings and the NUL terminator

```bash
./drill run c05
```

If you get stuck, use `./drill hint c05`. From the exercise side, `./drill read c05` brings you back to this chapter.

## References

- C99 standard, [string.h](https://en.cppreference.com/w/c/string/byte)
- [CERT: STR31-C. Guarantee that string-handling functions do not form invalid pointers](https://wiki.sei.cmu.edu/confluence/display/c/STR31-C)

---

Previous chapter → [4. Pointers 2 — arrays and pointer arithmetic](04_pointers_2.md)
Next chapter → [6. Structs and alignment](06_structs_and_alignment.md)
