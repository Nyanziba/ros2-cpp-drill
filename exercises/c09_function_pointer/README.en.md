# c09 Function pointers and table-driven programming [C track]

In this exercise you manage the handlers of an LED controller with function pointers and learn table-driven programming.

## Do not edit the header

You **do not edit** `include/drill/function_pointer.h`. Read it first.

```c
/* Callback function type. state is 0 (OFF) or 1 (ON). */
typedef void (*led_callback_t)(int led_id, int state);

/* Registers the ON/OFF handler of an LED. */
void led_register_handler(struct LedController * ctrl, int led_id, led_callback_t handler);

/* Controls an LED. Calls the registered handler if there is one.
 * If it is NULL (not registered), does nothing (safe). */
void led_set(struct LedController * ctrl, int led_id, int state);
```

## What to do

The only file you edit is `src/function_pointer.c`.

### 1. Creating and destroying the LED controller

```c
struct LedController * led_controller_create(void)
```

Allocate a `LedController` with `malloc`, and **initialize all handlers to NULL**.

```c
void led_controller_destroy(struct LedController * ctrl)
```

Release the given pointer with `free`.

### 2. Registering and calling a handler

```c
void led_register_handler(struct LedController * ctrl, int led_id, led_callback_t handler)
```

Assign the function pointer into the handler table. Assume that the range check of `led_id` is done by the caller.

```c
void led_set(struct LedController * ctrl, int led_id, int state)
```

If the registered handler is not NULL, call it through the function pointer.

```c
if (ctrl->handlers[led_id] != NULL) {
  (*ctrl->handlers[led_id])(led_id, state);
}
```

**If it is NULL, do nothing** (safe). This is the most important part.

### 3. Null check of a handler

```c
int led_handler_is_null(const struct LedController * ctrl, int led_id)
```

Return 1 if the handler is NULL, and 0 otherwise.

## How to read a function pointer declaration and call

### Declaration

```c
typedef void (*led_callback_t)(int, int);
```

"`led_callback_t` is a pointer type to a function that takes `int, int` and returns `void`"

Read it from right to left:
- pointer `*`
- function `(int, int) -> void`

### Call

```c
led_callback_t handler = ...;
handler(led_id, state);      /* the usual way to write it */
(*handler)(led_id, state);   /* explicitly dereference the function pointer */
```

Both work. In C, dereferencing a function pointer is done implicitly.

## Run it

```bash
./drill run c09
```

**The tests fail until you start.** You clear it when all the tests are green.

## Common pitfalls

1. **The NULL check is required** — `led_set` on an unregistered slot does nothing. Safety, with no crash, is what matters.
2. **Function pointer declarations are complex** — `int (*fp)(int)` is "a pointer to a function that takes `int` and returns `int`". When you read a declaration, organize your thoughts with a typedef.
3. **Table-driven** — A pattern where you register handlers in a table (an array) and call the matching handler when an event happens. The basis of a dispatcher.
4. **Do not forget initialization** — If you do not initialize all handlers to NULL right after `malloc`, garbage is treated as pointers.

## Tests

| Test | What it checks |
| --- | --- |
| `コントローラー作成と破棄` (creating and destroying the controller) | Allocation in `create`, the NULL check, and initializing all handlers |
| `ハンドラーを登録できる` (can register a handler) | `register_handler` and `is_null` |
| `登録されたハンドラーが呼ばれる` (the registered handler is called) | Calling through a function pointer |
| `未登録のスロットを呼んでも落ちない` (calling an unregistered slot does not crash) | **The importance of the NULL check** |
| `ハンドラーを複数登録して正しく呼び分ける` (register several handlers and call the right one) | The basics of table-driven programming |
| `ハンドラーを NULL で削除できる` (can remove a handler with NULL) | Unregistering by registering NULL again |
| `ハンドラーを上書きできる` (can overwrite a handler) | Registering again to the same slot |

## References

- [9. Function pointers](../../docs-en/c/09_function_pointers.md)
