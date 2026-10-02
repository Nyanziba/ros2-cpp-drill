# 5. Pointers 2 (Arrays and Choosing Between Them)

> **Goal of this chapter**: The real power of pointers is walking through arrays. The array `int arr[10]` and the pointer `int * p = arr;` are closely related.
> In this chapter you learn the automatic conversion from array to pointer, what pointer arithmetic means, and how to choose between references and pointers.
> In modern C++ there are few places where you use raw pointers directly, but you must know them to read existing code.

## 5.1 Automatic conversion from array to pointer (decay)

When you use an array name as a value, it is automatically converted to a pointer to its first element.
This is called **array-to-pointer decay**.

```cpp
int arr[3]{10, 20, 30};
int * p = arr;          // arr is automatically converted to &arr[0]

std::cout << arr;       // an address (the result of decay)
std::cout << &arr[0];   // the same address
```

**However, `sizeof` is different:**

```cpp
sizeof(arr);    // 12 bytes (the whole array: 3 × 4)
sizeof(p);      // 8 bytes (the pointer itself)
```

## 5.2 Pointer arithmetic

When you add an integer to a pointer, it does not move by that many bytes. It moves by **that many times the size of the pointed-to type.**

```cpp
int arr[4]{100, 200, 300, 400};
int * p = &arr[0];

p + 0   // same address, *p = 100
p + 1   // address + 4 bytes, *(p+1) = 200
p + 2   // address + 8 bytes, *(p+2) = 300
p + 3   // address + 12 bytes, *(p+3) = 400
```

The result of the arithmetic is **a number of elements, not a number of bytes**.

### With char *, it moves 1 byte

```cpp
char carr[4]{'a', 'b', 'c', 'd'};
char * cp = &carr[0];

cp + 1  // address + 1 byte
cp + 2  // address + 2 bytes
```

When the pointed-to type changes, the step size of pointer arithmetic changes.

## 5.3 Walking an array

You can use pointer arithmetic to process array elements in order.

```cpp
int arr[5]{1, 2, 3, 4, 5};
int * p = arr;

for (int i = 0; i < 5; ++i) {
  std::cout << *(p + i) << " ";  // the same as arr[i]
}
```

`arr[i]` is shorthand for `*(arr + i)`.

## 5.4 Out-of-bounds access

If you access beyond the range of an array, it is **undefined behavior**.
You can detect it with AddressSanitizer (`-fsanitize=address`).

```cpp
int arr[2]{1, 2};
int * p = &arr[0];
std::cout << *(p + 2);  // Out of range!
```

Compile:
```bash
g++ -fsanitize=address basics05_oob.cpp -o basics05_oob && ./basics05_oob
```

Output (first part):
```
==1==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffffcb00028 at pc 0x555555555394
READ of size 4 at 0x7ffffcb00028 thread T0
    #0 0x555555555393 in main (..../basics05_oob+0x1393)
    ...
Address 0x7ffffcb00028 is located in stack of thread T0 at offset 40 in frame
    #0 0x555555555258 in main (...)
  This frame has 1 object(s):
    [32, 40) 'arr' (line 5) <== Memory access at offset 40 overflows this variable
```

The tool reports the buffer overflow in detail.

## 5.5 Passing an array to a function

When you pass an array to a function, the size information is lost.

```cpp
void print_array(int * arr, int size) {
  for (int i = 0; i < size; ++i) {
    std::cout << arr[i] << " ";
  }
}

int data[5]{10, 20, 30, 40, 50};
print_array(data, 5);  // You must pass the size separately
```

The array implicitly decays to `int *`, so inside the function **you cannot know the size.**
That is why the convention is to pass the size as a separate argument.

**In modern C++, `std::array<int, 5>` or `std::vector<int>` is recommended.**

## 5.6 Choosing between references and pointers

Use the table below to choose between a reference and a pointer.

| Situation | Reference | Pointer | Reason |
| --- | --- | --- | --- |
| It always exists | ○ Recommended | △ | Simple. No null check needed |
| It may be null | ✗ | ○ Required | Can express "optional" |
| You need to reseat (switch) it | ✗ | ○ Required | List traversal, etc. |
| You need to walk an array or do pointer arithmetic | △ | ○ Required | Only pointers support arithmetic |
| You want the caller's syntax to look like a "normal variable" | ○ Recommended | △ | A reference can be called as `f(a)` |

## 5.7 Moving to modern C++

In modern C++, you almost never use a raw pointer (`int *`) directly.
Use the following instead.

| Purpose | Raw pointer | Modern C++ |
| --- | --- | --- |
| Array | `int arr[10]` | `std::vector<int>` or `std::array<int, 10>` |
| Dynamic allocation | `int * p = new int(5)` | `std::unique_ptr<int>` or `std::shared_ptr<int>` |
| Optional value | `int * p = nullptr` | `std::optional<int>` |

For details, see C++ chapter 6, "Smart pointers".

## Try it yourself

**Check arrays, pointer arithmetic, and out-of-bounds access for real.**

```cpp
// basics05_practice.cpp
#include <iostream>

int main()
{
  std::cout << "=== Pointers 2 (arrays and choosing between them) ===\n\n";

  std::cout << "--- 1. Automatic conversion from array to pointer (decay) ---\n";
  int arr[3]{10, 20, 30};
  std::cout << "int arr[3]{10, 20, 30};\n";
  std::cout << "arr (array name used as a value) = " << arr << " (pointer to the first element)\n";
  std::cout << "&arr[0] = " << &arr[0] << " (same)\n";
  std::cout << "sizeof(arr) = " << sizeof(arr) << " (whole: 3 × 4 bytes)\n";
  int * ptr_to_arr = arr;
  std::cout << "sizeof(ptr_to_arr) = " << sizeof(ptr_to_arr) << " (8 bytes, depends on the architecture)\n\n";

  std::cout << "--- 2. Pointer arithmetic (the address moves by the type size) ---\n";
  int iarr[4]{100, 200, 300, 400};
  int * ip = &iarr[0];
  std::cout << "int iarr[4]{100, 200, 300, 400};\n";
  std::cout << "int * ip = &iarr[0];\n";
  std::cout << "ip         = " << ip << " (&iarr[0])\n";
  std::cout << "ip+1       = " << (ip + 1) << " (+ 4 bytes, sizeof(int))\n";
  std::cout << "ip+2       = " << (ip + 2) << " (+ 8 bytes)\n";
  std::cout << "ip+3       = " << (ip + 3) << " (+ 12 bytes)\n\n";

  std::cout << "Check the values too:\n";
  std::cout << "*ip        = " << *ip << "\n";
  std::cout << "*(ip+1)    = " << *(ip + 1) << "\n";
  std::cout << "*(ip+2)    = " << *(ip + 2) << "\n";
  std::cout << "*(ip+3)    = " << *(ip + 3) << "\n\n";

  std::cout << "--- 3. With char*, it moves 1 byte ---\n";
  char carr[4]{'a', 'b', 'c', 'd'};
  char * cp = &carr[0];
  std::cout << "char carr[4]{'a', 'b', 'c', 'd'};\n";
  std::cout << "char * cp = &carr[0];\n";
  std::cout << "cp         = " << (void*)cp << " (&carr[0])\n";
  std::cout << "cp+1       = " << (void*)(cp + 1) << " (+ 1 byte, sizeof(char))\n";
  std::cout << "cp+2       = " << (void*)(cp + 2) << " (+ 2 bytes)\n";
  std::cout << "cp+3       = " << (void*)(cp + 3) << " (+ 3 bytes)\n\n";

  std::cout << "--- 4. Out-of-bounds access (caught with -fsanitize=address) ---\n";
  std::cout << "(It is safer to run this in a separate program)\n";
  std::cout << "int small[2]{1, 2};\n";
  std::cout << "int * p = &small[0];\n";
  std::cout << "std::cout << *(p+2);  // Out of bounds!\n";
  std::cout << "Compile: g++ -fsanitize=address ...\n";
  std::cout << "Then AddressSanitizer reports it\n\n";

  std::cout << "--- 5. Passing an array to a function (the size is lost) ---\n";
  auto print_array = [](int * arr, int size) {
    std::cout << "Inside the function:\n";
    for (int i = 0; i < size; ++i) {
      std::cout << "  arr[" << i << "] = " << arr[i] << "\n";
    }
  };
  int arr5[3]{5, 10, 15};
  std::cout << "int arr5[3]{5, 10, 15};\n";
  std::cout << "print_array(arr5, 3);\n";
  print_array(arr5, 3);
  std::cout << "Note: the size information is lost, so you must pass it separately\n\n";

  std::cout << "--- 6. Use std::string or std::vector (modern C++) ---\n";
  std::cout << "Instead of raw pointers:\n";
  std::cout << "  - std::vector<int> (knows its own size)\n";
  std::cout << "  - std::string (no need to care about null termination)\n";
  std::cout << "are recommended\n\n";

  std::cout << "--- 7. Reference vs pointer: which to use ---\n";
  std::cout << "+------------------+----------+----------+----------+\n";
  std::cout << "| Situation        | Ref      | Pointer  | Reason   |\n";
  std::cout << "+------------------+----------+----------+----------+\n";
  std::cout << "| Always exists    | ○ best  | △        | Simple   |\n";
  std::cout << "| May be null      | ×       | ○ needed | Can be null |\n";
  std::cout << "| Needs reseating  | ×       | ○ needed | Switching |\n";
  std::cout << "| Array walking    | △       | ○ needed | Arithmetic |\n";
  std::cout << "+------------------+----------+----------+----------+\n\n";

  std::cout << "--- 8. Summary: toward smart pointers ---\n";
  std::cout << "In modern C++ you almost never use a raw pointer (int*) directly\n";
  std::cout << "Instead:\n";
  std::cout << "  - std::unique_ptr<T> (one owner)\n";
  std::cout << "  - std::shared_ptr<T> (multiple owners)\n";
  std::cout << "are used (see C++ chapter 6)\n";

  return 0;
}
```

**Predict: check how the addresses move in pointer arithmetic. `char *` moves 1 byte and `int *` moves 4 bytes.**

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic basics05_practice.cpp -o basics05_practice && ./basics05_practice
```

[▶ Run in your browser (gcc 13.3)](https://godbolt.org/z/18vjqe5fb)

Output:

<details markdown="1"><summary>Answer (actual output)</summary>

```
=== Pointers 2 (arrays and choosing between them) ===

--- 1. Automatic conversion from array to pointer (decay) ---
int arr[3]{10, 20, 30};
arr (array name used as a value) = 0x7ffffffc58a8 (pointer to the first element)
&arr[0] = 0x7ffffffc58a8 (same)
sizeof(arr) = 12 (whole: 3 × 4 bytes)
sizeof(ptr_to_arr) = 8 (8 bytes, depends on the architecture)

--- 2. Pointer arithmetic (the address moves by the type size) ---
int iarr[4]{100, 200, 300, 400};
int * ip = &iarr[0];
ip         = 0x7ffffffc58c0 (&iarr[0])
ip+1       = 0x7ffffffc58c4 (+ 4 bytes, sizeof(int))
ip+2       = 0x7ffffffc58c8 (+ 8 bytes)
ip+3       = 0x7ffffffc58cc (+ 12 bytes)

Check the values too:
*ip        = 100
*(ip+1)    = 200
*(ip+2)    = 300
*(ip+3)    = 400

--- 3. With char*, it moves 1 byte ---
char carr[4]{'a', 'b', 'c', 'd'};
char * cp = &carr[0];
cp         = 0x7ffffffc58d4 (&carr[0])
cp+1       = 0x7ffffffc58d5 (+ 1 byte, sizeof(char))
cp+2       = 0x7ffffffc58d6 (+ 2 bytes)
cp+3       = 0x7ffffffc58d7 (+ 3 bytes)

--- 4. Out-of-bounds access (caught with -fsanitize=address) ---
(It is safer to run this in a separate program)
int small[2]{1, 2};
int * p = &small[0];
std::cout << *(p+2);  // Out of bounds!
Compile: g++ -fsanitize=address ...
Then AddressSanitizer reports it

--- 5. Passing an array to a function (the size is lost) ---
int arr5[3]{5, 10, 15};
print_array(arr5, 3);
Inside the function:
  arr[0] = 5
  arr[1] = 10
  arr[2] = 15
Note: the size information is lost, so you must pass it separately

--- 6. Use std::string or std::vector (modern C++) ---
Instead of raw pointers:
  - std::vector<int> (knows its own size)
  - std::string (no need to care about null termination)
are recommended

--- 7. Reference vs pointer: which to use ---
+------------------+----------+----------+----------+
| Situation        | Ref      | Pointer  | Reason   |
+------------------+----------+----------+----------+
| Always exists    | ○ best  | △        | Simple   |
| May be null      | ×       | ○ needed | Can be null |
| Needs reseating  | ×       | ○ needed | Switching |
| Array walking    | △       | ○ needed | Arithmetic |
+------------------+----------+----------+----------+

--- 8. Summary: toward smart pointers ---
In modern C++ you almost never use a raw pointer (int*) directly
Instead:
  - std::unique_ptr<T> (one owner)
  - std::shared_ptr<T> (multiple owners)
are used (see C++ chapter 6)
```

</details>

Check these three points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. **sizeof difference**: Are the sizes of the array and the pointer different?
2. **Pointer arithmetic**: Does `int *` move 4 bytes and `char *` move 1 byte?
3. **Passing an array to a function**: Is the size information lost inside the function, so that you must pass the size separately?

</details>

To try detecting out-of-bounds access with AddressSanitizer, use a separate program:

```bash
# Create basics05_oob.cpp and compile it with -fsanitize=address
g++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address basics05_oob.cpp -o basics05_oob
timeout 5 ./basics05_oob 2>&1 | head -10
```

## Common pitfalls

**"Why does pointer arithmetic move by the type size instead of by 1?"**

This is how C++ is designed. `p + 1` means "the next element", so it moves by the size of the type.
If you need low-level byte operations, cast the pointer, as in `(char*)p + 1`.

**"What is the difference between an array and a pointer?"**

An array has a fixed size, while a pointer can point to anything later. Because of decay, an array name is converted to a pointer, but `sizeof` is different.

**"If I should avoid raw pointers, why learn them?"**

Raw pointers appear a lot in existing legacy code (C-style functions, old C++ libraries).
You must be able to read them, but in new code (especially ROS 2), use `std::vector` or `std::shared_ptr`.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `cppb05_pointer_array`: arrays and pointers

```bash
./drill run cppb05
```

If you get stuck, run `./drill hint cppb05`. From the exercise side, `./drill read cppb05` brings you back to this chapter.

## References

- [C++ 4. References and const](../cpp/04_references_and_const.md): comparing pointers and references
- [C++ 6. Smart pointers](../cpp/06_smart_pointers.md): the alternative to raw pointers
- cppreference: [std::vector](https://en.cppreference.com/w/cpp/container/vector), [std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)

---

Previous → [4. Pointers 1](04_pointers_1.md)
Next → [6. const](06_const.md)
