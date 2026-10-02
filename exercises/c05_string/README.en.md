# c05: Strings

Learn string handling in C. In C, a string is a NUL-terminated (`'\0'`) sequence of bytes.

## Learning goals

- Understanding the structure of NUL-terminated strings
- Managing buffer sizes
- Copying and concatenating strings safely
- Safe implementations of standard library functions (`strlen`, `strncpy`, `strncat`)

## Functions to implement

1. **`int string_length(const char *s)`**
   - Return the length of the C string s (not including the NUL character)
   - Return -1 if s is NULL
   - An implementation of `strlen()`

2. **`int string_copy(char *dest, const char *src, size_t max_len)`**
   - Copy src to dest
   - The buffer size of dest is max_len bytes
   - After copying, always add the NUL terminator (counted within max_len)
   - Return -1 if src is NULL, dest is NULL, or max_len is 0
   - Return 0 on success

3. **`int string_concat(char *dest, const char *src, size_t max_len)`**
   - Append (concatenate) src to dest
   - Assume dest is already a NUL-terminated string
   - The buffer size of dest is max_len bytes (including the NUL)
   - Always add the NUL terminator after concatenation too
   - Return -1 if dest is NULL, src is NULL, or max_len is 0
   - Return 0 on success

## Tests

```bash
colcon test --packages-select drill_c05_string
```

## Important notes

- **NUL termination is required**: After every string operation, always terminate with `'\0'`
- **Managing the buffer size**: At most `max_len - 1` characters are copied or appended (the remaining 1 byte is for the NUL)
- **Safety**: Aim for an implementation that is safer than `strlen()`, `strcpy()`, and `strcat()`

## Hints

- Increment the pointer: `p++` moves to the next character
- Checking for the end of a string: `s[i] != '\0'` or `*s != '\0'`
- Calculating the buffer capacity: check that the length to copy + 1 (for the NUL) is at most max_len
