# c10 volatile and interrupt safety [C track]

This exercise teaches what the `volatile` keyword guarantees and what it does not guarantee.

## Important: volatile is not thread-safe

`volatile` **only prevents compiler optimization**.

- ✅ Prevents the removal of reads by optimization
- ✅ Access to memory-mapped hardware registers
- ❌ Synchronization between several threads
- ❌ Atomicity (operations that cannot be split)

Wrong use: `volatile int x; x++` does not become thread-safe. With `volatile` alone it is a **data race.**

## Do not edit the header

You **do not edit** `include/drill/volatile_state.h`.

It is a state machine that uses a shared variable monitored from outside:

```c
extern volatile state_t g_machine_state;
```

## What to do

The only file you edit is `src/volatile_state.c`.

### 1. Initialization

```c
void machine_init(void)
```

Set `g_machine_state` to `STATE_IDLE`.

### 2. State transitions

```c
void machine_start(void)
void machine_stop(void)
```

Change `g_machine_state`.

### 3. Checking the state

```c
state_t machine_get_state(void)
int machine_is_idle(void)
int machine_is_running(void)
int machine_is_stopped(void)
```

Read from `g_machine_state` and return the current state.

### 4. Change from outside (for tests)

```c
void machine_simulate_external_change(state_t new_state)
```

Simulates a change from an interrupt handler or another process.
Write `g_machine_state = new_state`.

## Why volatile is needed

### Example: without volatile

```c
state_t state = STATE_IDLE;

/* Compiler optimization */
if (state == STATE_IDLE) {
  // We got in here, so inside this { } it is fixed that state is STATE_IDLE
  // Even if there is a change from outside, the compiler does not know (optimization)
  while (state == STATE_IDLE) {
    /* Even if state is changed from outside, the compiler
     * assumes that "state is stuck at STATE_IDLE".
     * It may even remove the while loop. */
  }
}
```

### Example: with volatile

```c
volatile state_t state = STATE_IDLE;

if (state == STATE_IDLE) {
  /* Because of volatile,
   * state is actually read from memory every time.
   * It reliably sees changes from outside. */
  while (state == STATE_IDLE) {
    /* It reads from memory every time, so it notices changes from outside */
  }
}
```

## Run it

```bash
./drill run c10
```

**The tests fail until you start.** You clear it when all the tests are green.

## Common pitfalls

1. **`volatile` is not thread-safe** — For data race protection, use `std::atomic` or a mutex, not `volatile`
2. **Only the optimization side** — `volatile` only says "do perform the reads and writes". It does not provide ordering guarantees or atomicity
3. **Single-thread assumption** — This exercise simulates a "change from outside" (such as an interrupt handler) on a single thread
4. **Do not forget initialization** — Call `machine_init()` first and initialize with `STATE_IDLE`

## Tests

| Test | What it checks |
| --- | --- |
| `初期化と状態確認` (initialization and checking the state) | `machine_init` and the state-checking functions |
| `IDLE_から_RUNNING_に遷移` (transition from IDLE to RUNNING) | `machine_start` |
| `RUNNING_から_STOPPED_に遷移` (transition from RUNNING to STOPPED) | `machine_stop` |
| `複数回の状態遷移` (several state transitions) | That several state changes work correctly |
| `外部から状態が変更されたことを検出できる` (can detect that the state was changed from outside) | **The importance of volatile** — correctly seeing changes from outside |
| `外部から複数回の変更を検出できる` (can detect several changes from outside) | Several external changes |
| `ポーリングループで状態を監視できる` (can monitor the state in a polling loop) | A typical polling pattern |
| `get_state_は毎回読み込みをしている` (get_state reads every time) | The guarantee of reading every time by volatile |

## References

- [10. volatile and interrupt safety](../../docs-en/c/10_volatile_and_interrupt_safety.md)

## Learn more

- **Concurrency** — Use `std::atomic` or a mutex, not `volatile` (in C++, `#include <atomic>`)
- **Interrupt safety** — Used in an RTOS. Share state safely even when a hardware interrupt occurs
- **Memory-mapped hardware** — Access hardware registers with volatile
