# c04: Arrays and pointer arithmetic

Learn the relationship between arrays and pointers, and memory safety. This course has ASAN (AddressSanitizer) enabled, so buffer overflows are detected immediately.

## Learning goals

- The relationship between arrays and pointers
- Accessing elements with pointer arithmetic
- How to handle the number of elements safely
- Run-time detection with ASAN (AddressSanitizer)

## Functions to implement

1. **`int sum_array(const int *arr, size_t len)`**
   - Calculate the sum of len elements from the start of the array
   - Return 0 if len is 0
   - The const qualifier makes it read-only

2. **`int max_element(const int *arr, size_t len)`**
   - Return the maximum of len elements from the start of the array
   - Return INT_MIN if len is 0

3. **`void double_elements(int *arr, size_t len)`**
   - Double each of len elements from the start of the array
   - Do nothing if len is 0

## Tests

```bash
colcon test --packages-select drill_c04_pointer_array
```

## Important notes

- **ASAN enabled**: This project is protected by ASAN (AddressSanitizer), and the program stops immediately if there is an out-of-range array access
- **Safe implementation**: Always check the range with something like `i < len`, based on the `len` parameter
- It is normal behavior for the test to fail by ASAN detection when you run the unfinished version

## Hints

- Pointer arithmetic: `arr[i]` is the same as `*(arr + i)`
- `size_t` is an unsigned integer type and is suitable for array indexes
- An argument qualified with const is read-only
