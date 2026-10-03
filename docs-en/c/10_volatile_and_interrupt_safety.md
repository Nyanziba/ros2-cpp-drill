# 10. volatile and interrupt safety

> **Goal of this chapter**: The compiler removes reads of "variables that should not change" during optimization. `volatile` is required for variables that are rewritten by hardware registers or interrupt handlers. But there is a trap: **`volatile` does not guarantee atomicity**. This misunderstanding is the cause of the most common bugs in embedded software.

## 10.1 Compiler optimization removes the read

**The compiler assumes "the value of the variable does not change", and may remove a loop.**

```c
uint32_t flag = 0;

while (flag == 0) {
    // even though flag is rewritten from outside here...
    // the compiler cannot see it
}
```

When the compiler sees this code, it decides that "`flag` never changes after initialization". Then it decides that the loop condition check is unnecessary, and removes it by optimization.

**Let us check by measurement.** Compile the following code with `-O0` (no optimization) and `-O2` (maximum optimization), and compare the generated assembly.

```c
void wait_without_volatile(void) {
    uint32_t flag = 0;
    int count = 0;
    while (flag == 0) {
        count++;
    }
    printf("Done\n");
}
```

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// wait_without_volatile.c
#include <stdint.h>
#include <stdio.h>

void wait_without_volatile(void) {
    uint32_t flag = 0;
    int count = 0;
    while (flag == 0) {
        count++;
    }
    printf("Done\n");
}
```

```bash
gcc -std=c99 -O0 -S -fcf-protection=none -fno-asynchronous-unwind-tables -o - wait_without_volatile.c | sed -n '/^wait_without_volatile:/,/\.size/p'
gcc -std=c99 -O2 -S -fcf-protection=none -fno-asynchronous-unwind-tables -o - wait_without_volatile.c | sed -n '/^wait_without_volatile:/,/\.size/p'
```

</details>

**Case `-O0` (the assembly of the whole function):**

<!-- measure: filter="head -n 19" -->
```
wait_without_volatile:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$16, %rsp
	movl	$0, -4(%rbp)
	movl	$0, -8(%rbp)
	jmp	.L2
.L3:
	addl	$1, -8(%rbp)
.L2:
	cmpl	$0, -4(%rbp)
	je	.L3
	leaq	.LC0(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	nop
	leave
	ret
	.size	wait_without_volatile, .-wait_without_volatile
```

**How to read it (annotated)**

(Lines picked from the measured output above, with notes.)

```asm
	movl	$0, -4(%rbp)	# flag = 0 (kept in memory at -4(%rbp))
	...
	addl	$1, -8(%rbp)	# count++
	...
	cmpl	$0, -4(%rbp)	# read flag from memory and compare with 0 (every iteration)
	je	.L3	# if 0, go back to .L3 and keep looping
```

**Case `-O2` (the whole assembly):**

<!-- measure: filter="tail -n 6" -->
```
wait_without_volatile:
	.p2align 4,,10
	.p2align 3
.L2:
	jmp	.L2
	.size	wait_without_volatile, .-wait_without_volatile
```

**How to read it (annotated)**

(Lines picked from the measured output above, with notes.)

```asm
.L2:
	jmp	.L2	# the condition check is gone; it just jumps to itself forever
```

This is shocking. The condition check disappeared and it became an infinite loop.

## 10.2 `volatile` is an instruction: "read from memory every time"

**With the `volatile` keyword, the compiler does not remove the condition check.**

```c
void wait_with_volatile(void) {
    volatile uint32_t flag = 0;
    int count = 0;
    while (flag == 0) {
        count++;
    }
    printf("Done\n");
}
```

Even when compiled with `-O2`, in the assembly:

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// wait_with_volatile.c
#include <stdint.h>
#include <stdio.h>

void wait_with_volatile(void) {
    volatile uint32_t flag = 0;
    int count = 0;
    while (flag == 0) {
        count++;
    }
    printf("Done\n");
}
```

```bash
gcc -std=c99 -O2 -S -fcf-protection=none -fno-asynchronous-unwind-tables -o - wait_with_volatile.c | sed -n '/^wait_with_volatile:/,/\.size/p'
```

</details>

<!-- measure: -->
```
wait_with_volatile:
	movl	$0, -12(%rsp)
	.p2align 4,,10
	.p2align 3
.L2:
	movl	-12(%rsp), %eax
	testl	%eax, %eax
	je	.L2
	leaq	.LC0(%rip), %rsi
	movl	$2, %edi
	xorl	%eax, %eax
	jmp	__printf_chk@PLT
	.size	wait_with_volatile, .-wait_with_volatile
```

**How to read it (annotated)**

(Lines picked from the measured output above, with notes.)

```asm
.L2:
	movl	-12(%rsp), %eax	# volatile, so flag is read from memory every time
	testl	%eax, %eax	# test whether flag == 0
	je	.L2	# if 0, keep looping
```

This is the effect of `volatile`.

## 10.3 What `volatile` does not guarantee (most important)

**`volatile` only guarantees "read from memory".** It does not guarantee the following.

### It does not guarantee atomicity

**This is the biggest misunderstanding.** Adding `volatile` does not make code thread-safe.

```c
volatile uint32_t counter = 0;

counter++;  // this is 3 steps: read → +1 → write
            // if an interrupt interrupts it, a count is lost
```

The actual assembly (`-O2`):

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// counter_increment.c
#include <stdint.h>

volatile uint32_t counter = 0;

void increment_counter(void) {
    counter++;  // this is 3 steps: read → +1 → write
                // if an interrupt interrupts it, a count is lost
}
```

```bash
gcc -std=c99 -O2 -S -fcf-protection=none -fno-asynchronous-unwind-tables -o - counter_increment.c | sed -n '/^increment_counter:/,/\.size/p'
```

</details>

<!-- measure: -->
```asm
increment_counter:
	movl	counter(%rip), %eax
	addl	$1, %eax
	movl	%eax, counter(%rip)
	ret
	.size	increment_counter, .-increment_counter
```

**How to read it (annotated)**

(Lines picked from the measured output above, with notes.)

```asm
	movl	counter(%rip), %eax	# 1. read from memory
	addl	$1, %eax	# 2. add 1 (an interrupt here is dangerous)
	movl	%eax, counter(%rip)	# 3. write to memory
```

If an interrupt happens between steps 2 and 3, even if other code increments `counter`, it is overwritten here.

### It does not guarantee a memory barrier

The order of reads and writes of several `volatile` variables may be swapped by the compiler.

```c
volatile uint32_t ctrl = 0;
volatile uint32_t data = 0;

data = 0x12345678;    // set data first
ctrl = 1;              // then set ctrl to 1

// the compiler may swap the order
// → risk that ctrl comes first
```

If you want to fix the order of reads and writes, you need to put up a barrier with `asm volatile("" ::: "memory");`.

### **The claim "it is thread-safe because I added `volatile`" is wrong.**

If you need thread safety:
- Use `pthread_mutex_t` (a mutex)
- Use `atomic_*` types (C11 atomic types)
- Disable interrupts

## 10.4 `volatile sig_atomic_t` — safe access from a signal handler

**In POSIX, the only type you can read and write from a signal handler is `sig_atomic_t`.**

```c
#include <signal.h>

volatile sig_atomic_t signal_received = 0;

void handler(int sig)
{
    signal_received = 1;  // safe
}

int main(void)
{
    signal(SIGUSR1, handler);

    signal_received = 0;
    kill(getpid(), SIGUSR1);
    usleep(100000);

    if (signal_received) {
        printf("Signal received\n");
    }

    return 0;
}
```

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// signal_flag.c
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

volatile sig_atomic_t signal_received = 0;

void handler(int sig)
{
    signal_received = 1;  // safe
}

int main(void)
{
    signal(SIGUSR1, handler);

    signal_received = 0;
    kill(getpid(), SIGUSR1);
    usleep(100000);

    if (signal_received) {
        printf("Signal received\n");
    }

    return 0;
}
```

```bash
gcc -std=gnu99 -Wall -Wpedantic signal_flag.c -o signal_flag && ./signal_flag
```

</details>

Measured values:

<!-- measure: -->
```
Signal received
```

**What `sig_atomic_t` is:**
- A type whose reads and writes are guaranteed not to be interrupted
- On many platforms it is the same size as `int`
- Use it together with `volatile`

**If you use another type inside the handler, it is undefined behavior.**

## 10.5 Reading and writing hardware registers

**On a microcontroller, you access hardware registers by specifying a memory address.**

```c
// STM32 UART example
volatile uint32_t *uart_dr = (volatile uint32_t *)0x40004000;   // data
volatile uint32_t *uart_sr = (volatile uint32_t *)0x40004004;   // status

// read the status register
uint32_t status = *uart_sr;

// write data
*uart_dr = 'A';

// read the status register (second time)
status = *uart_sr;
```

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// uart_register_access.c
#include <stdint.h>

void access_uart(void) {
    // STM32 UART example
    volatile uint32_t *uart_dr = (volatile uint32_t *)0x40004000;   // data
    volatile uint32_t *uart_sr = (volatile uint32_t *)0x40004004;   // status

    // read the status register
    uint32_t status = *uart_sr;

    // write data
    *uart_dr = 'A';

    // read the status register (second time)
    status = *uart_sr;
}
```

```bash
gcc -std=c99 -O2 -S -fcf-protection=none -fno-asynchronous-unwind-tables -o - uart_register_access.c | sed -n '/^access_uart:/,/\.size/p'
```

</details>

Measured values (assembly with `gcc -O2`):

<!-- measure: -->
```asm
access_uart:
	movl	1073758212, %eax
	movl	$65, 1073758208
	movl	1073758212, %eax
	ret
	.size	access_uart, .-access_uart
```

**How to read it (annotated)**

(Lines picked from the measured output above, with notes.)

```asm
	movl	1073758212, %eax	# read uart_sr (1st)
	movl	$65, 1073758208	# write 'A' (65) to uart_dr
	movl	1073758212, %eax	# read uart_sr (2nd)
```

`1073758208` is `0x40004000` (`uart_dr`) and `1073758212` is `0x40004004` (`uart_sr`), written in decimal.

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// uart_register_access_plain.c
#include <stdint.h>

void access_uart(void) {
    // STM32 UART example
    uint32_t *uart_dr = (uint32_t *)0x40004000;   // data
    uint32_t *uart_sr = (uint32_t *)0x40004004;   // status

    // read the status register
    uint32_t status = *uart_sr;

    // write data
    *uart_dr = 'A';

    // read the status register (second time)
    status = *uart_sr;
}
```

```bash
gcc -std=c99 -O2 -S -fcf-protection=none -fno-asynchronous-unwind-tables -o - uart_register_access_plain.c | sed -n '/^access_uart:/,/\.size/p'
```

</details>

**Without `volatile`:**

<!-- measure: -->
```asm
access_uart:
	movl	$65, 1073758208
	ret
	.size	access_uart, .-access_uart
```

**How to read it (annotated)**

(Lines picked from the measured output above, with notes.)

```asm
	movl	$65, 1073758208	# only the write to uart_dr remains (both reads of uart_sr are gone)
```

Both reads disappear, and only the write (`movl $65, 1073758208`) remains.

Always add `volatile` to registers.

## 10.6 The danger of Read-Modify-Write (RMW)

**Even with `volatile`, an RMW operation is not atomic.**

```c
volatile uint32_t gpio = 0x00000000;

// you want to set bit 3
gpio |= (1 << 3);     // actually 3 steps: read → OR → write
```

Assembly (`-O2`):

<details markdown="1"><summary>Full program that produced this output</summary>

```c
// gpio_rmw.c
#include <stdint.h>

volatile uint32_t gpio = 0x00000000;

void set_gpio_bit3(void) {
    // you want to set bit 3
    gpio |= (1 << 3);     // actually 3 steps: read → OR → write
}
```

```bash
gcc -std=c99 -O2 -S -fcf-protection=none -fno-asynchronous-unwind-tables -o - gpio_rmw.c | sed -n '/^set_gpio_bit3:/,/\.size/p'
```

</details>

<!-- measure: -->
```asm
set_gpio_bit3:
	movl	gpio(%rip), %eax
	orl	$8, %eax
	movl	%eax, gpio(%rip)
	ret
	.size	set_gpio_bit3, .-set_gpio_bit3
```

**How to read it (annotated)**

(Lines picked from the measured output above, with notes.)

```asm
	movl	gpio(%rip), %eax	# 1. read from memory
	orl	$8, %eax	# 2. OR in bit 3 (value 8)
	movl	%eax, gpio(%rip)	# 3. write to memory
```

With a hardware register, the register state at the time of the read may differ from the actual value at the time of the write.

**To manipulate bits safely:**

1. **Disable interrupts**
   ```c
   unsigned long flags = disable_irq_save();  // disable interrupts
   gpio |= (1 << 3);
   restore_irq(flags);                        // resume interrupts
   ```

2. **Use the "bit set register" that the hardware provides** (STM32 and others)
   ```c
   // GPIO_BSRR: set per bit in 1 step
   *gpio_bsrr = (1 << 3);
   ```

3. **Use an atomic operation library**
   ```c
   #include <stdatomic.h>
   atomic_uint gpio = 0;
   atomic_fetch_or(&gpio, (1 << 3));
   ```

## Try it yourself

Check the difference between with and without `volatile`.

```c
// volatile_test.c
#include <stdio.h>
#include <stdint.h>

// Test 1: read a hardware register
void test_register_reads(void)
{
    volatile uint32_t data[] = { 0x11111111, 0x22222222, 0x33333333 };
    volatile uint32_t *reg = data;

    uint32_t val1 = *reg;
    uint32_t val2 = *reg;
    uint32_t val3 = *reg;

    printf("Register reads (volatile):\n");
    printf("  Read 1: 0x%08x\n", val1);
    printf("  Read 2: 0x%08x\n", val2);
    printf("  Read 3: 0x%08x\n", val3);
}

// Test 2: bit manipulation
void test_bit_manipulation(void)
{
    volatile uint32_t gpio = 0x00000000;

    printf("\nBit manipulation (volatile):\n");
    printf("Initial: 0x%08x\n", gpio);

    gpio |= (1 << 3);
    printf("After |= (1 << 3): 0x%08x\n", gpio);

    gpio |= (1 << 7);
    printf("After |= (1 << 7): 0x%08x\n", gpio);

    gpio &= ~(1 << 3);
    printf("After &= ~(1 << 3): 0x%08x\n", gpio);
}

int main(void)
{
    test_register_reads();
    test_bit_manipulation();

    printf("\nNote: volatile does NOT guarantee atomicity!\n");
    printf("RMW operations can be interrupted mid-operation.\n");

    return 0;
}
```

**Predict: If you read 3 times from a hardware register, does it return `0x11111111` every time? Are the bit operations on the GPIO register executed correctly?**

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic volatile_test.c -o volatile_test && ./volatile_test
```

<details markdown="1"><summary>Answer (actual output)</summary>

<!-- measure: -->
```
Register reads (volatile):
  Read 1: 0x11111111
  Read 2: 0x11111111
  Read 3: 0x11111111

Bit manipulation (volatile):
Initial: 0x00000000
After |= (1 << 3): 0x00000008
After |= (1 << 7): 0x00000088
After &= ~(1 << 3): 0x00000080

Note: volatile does NOT guarantee atomicity!
RMW operations can be interrupted mid-operation.
```

</details>

Check these 3 points.

<details markdown="1"><summary>Answer (check yourself)</summary>

1. Can you confirm that, even when you read 3 times from a hardware register, a new value is fetched from memory every time?
2. Do the results of the bit operations match what you expected? (They only succeed here because there are no interrupts. In real use it is dangerous.)
3. How does the assembly change when you compile without `volatile`? (The reads should be folded into one.)

</details>

## Common pitfalls

**The misunderstanding that "adding `volatile` makes it thread-safe"**
`volatile` only forces a memory read. It does not guarantee atomicity or memory barriers. If you need thread safety, use a mutex or atomic operations.

**An RMW operation is interrupted by an interrupt**
`counter++` and `gpio |= mask` are 3 steps: read, compute, write. If an interrupt comes in between, a count is lost.

**Forgetting `volatile` on a hardware register**
The read from the register is removed by optimization, and the old value keeps being used. As a result, the hardware state is not reflected, which leads to communication failures and control errors.

**No memory barrier**
The order of operations on several `volatile` variables may be swapped. When the order matters (for example, writing data before a control flag), you need an explicit barrier.

## Matching exercise

After reading this chapter, practice with the matching drill.

- `c10_volatile` — volatile and shared variables

```bash
./drill run c10
```

If you get stuck, use `./drill hint c10`. From the exercise side, `./drill read c10` brings you back to this chapter.

## References

- [cppreference: volatile type qualifier](https://en.cppreference.com/w/c/language/volatile)
- [POSIX: sig_atomic_t](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/signal.h.html)

---

Previous chapter → [9. Function pointers](09_function_pointers.md)
Next chapter → [11. Endianness and serialization](11_endianness_and_serialization.md)
