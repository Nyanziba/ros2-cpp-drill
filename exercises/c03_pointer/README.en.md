# c03: Pass an address with a pointer

Use C pointers to learn how to rewrite a variable through a function argument.

## Learning goals

- Basic use of pointers (the `&` and `*` operators)
- Checking for NULL pointers and safe checks
- The difference between pass by value and pass by reference (via a pointer)
- Changing a variable directly from a function

## Functions to implement

1. **`int swap_values(int *a, int *b)`**
   - Swap the values of the two variables
   - Return -1 if a or b is NULL
   - Return 0 on success

2. **`int multiply(int x, int *result)`**
   - Double x and store it in the place that result points to
   - Return -1 if result is NULL
   - Return 0 on success

3. **`int triple_pointer(int *p)`**
   - Triple the value that p points to
   - Return -1 if p is NULL
   - Return 0 on success

## Tests

```bash
colcon test --packages-select drill_c03_pointer
```

## Hints

- To change a value through a pointer argument, assign in the form `*p = ...`
- Do the `NULL` check first
- Using a temporary variable makes swapping values easy
