# c04 Receive with a reference and const [C++]

Learn const references and const member functions.

## What to do

Implement two functions in `src/copycounter.cpp`:

1. **copy_and_uppercase()**
   - Receive a string by const reference
   - Convert the text to all uppercase
   - Return the new string as the return value

2. **get_description()**
   - Implement it as a const member function
   - Return the description in the form `コピー回数: X` (Copy count: X)

## Run it

```bash
./drill run c04
```

## Common pitfalls

- If you receive by const reference `const std::string&`, no copy happens.
- A const member function has the form `int foo() const { ... }`.
- Inside a const member function, you cannot change member variables.

## Tests

```bash
./drill run c04
```

| Test | What it checks |
| --- | --- |
| `constReferencePolicyで大文字に変換` (convert to uppercase with constReferencePolicy) | efficient receiving by const reference |
| `constMemberFunctionが説明を返す` (constMemberFunction returns the description) | the syntax of const member functions |

## References

- [cppreference: const qualifier](https://en.cppreference.com/w/cpp/language/const)
- [cppreference: Reference](https://en.cppreference.com/w/cpp/language/reference)
- [4. Receive with a reference and const](../../docs-en/cpp/04_references_and_const.md)
