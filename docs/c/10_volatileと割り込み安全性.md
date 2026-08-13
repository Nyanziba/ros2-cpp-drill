# 10. volatile と割り込み安全性

> **この章のねらい**: コンパイラは「変わらないはずの変数」の読み出しを最適化で消してしまいます。ハードウェアレジスタや割り込みハンドラから書き換わる変数には `volatile` が必須です。ただし、**`volatile` はアトミック性を保証しない** という落とし穴があります。この誤解が組み込みソフトウェアで最も多いバグの原因になります。

## 10.1 コンパイラの最適化で読み出しが消える

**コンパイラは「変数の値は変わらない」と仮定して、ループを消してしまうことがあります。**

```c
uint32_t flag = 0;

while (flag == 0) {
    // ここで flag が外部から書き換わるのに...
    // コンパイラには見えない
}
```

このコードをコンパイラが見ると、「`flag` は初期化後に変わることはない」と判断します。すると、ループの条件チェックが不要と判断され、最適化で消されてしまいます。

**実測で確認します。** 以下のコードを `-O0`（最適化なし）と `-O2`（最大最適化）でコンパイルして、生成されたアセンブリを比較します。

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

**`-O0` の場合（アセンブリの関連部分）：**

```
cmpl	$0, -4(%rbp)      # flag をメモリから読む
je	.L3                   # 0 なら .L3 にジャンプ
.L3:
	addl	$1, -8(%rbp)   # count をインクリメント
.L2:
	cmpl	$0, -4(%rbp)   # 毎回 flag をメモリから読み直す
	je	.L3                # ...
```

毎ループ `cmpl $0, -4(%rbp)` で flag を読み直しています。

**`-O2` の場合（アセンブリの全体）：**

```
.L2:
	jmp	.L2              # 無限ループに最適化！
```

ショックです。条件チェックが消えて、無限ループになってしまいました。

## 10.2 `volatile` は「毎回メモリから読め」という指示

**`volatile` キーワードをつけると、コンパイラは条件チェックを消しません。**

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

`-O2` でコンパイルしても、アセンブリでは：

```
.L6:
	movl	-12(%rsp), %eax   # volatile なので毎回メモリから読む
	addl	$1, %edx
	testl	%eax, %eax       # flag == 0 かテスト
	je	.L9                   # 0 なら ループ続行
```

毎ループ `movl -12(%rsp), %eax` でメモリから読み直しています。これが `volatile` の効果です。

## 10.3 `volatile` が保証しないこと（最重要）

**`volatile` は「メモリから読む」ことしか保証しません。** 以下のことは保証しません。

### アトミック性を保証しない

**これが最大の誤解です。** `volatile` を付けたからといって、スレッドセーフになりません。

```c
volatile uint32_t counter = 0;

counter++;  // これは3ステップ：読む → +1 → 書く
            // 割り込みで中断されたら、カウント が失われます
```

実際のアセンブリ（`-O2`）：

```asm
movl	counter(%rip), %eax    # 1. メモリから読む
addl	$1, %eax               # 2. +1 する
movl	%eax, counter(%rip)    # 3. メモリに書く
```

割り込みが 2 番と 3 番の間に発生すると、他のコードが `counter` をインクリメントしても、ここで上書きされてしまいます。

### メモリバリアを保証しない

複数の `volatile` 変数の読み書き順序は、コンパイラによって入れ替わる可能性があります。

```c
volatile uint32_t ctrl = 0;
volatile uint32_t data = 0;

data = 0x12345678;    // 先に data をセット
ctrl = 1;              // その後 ctrl を 1 に

// コンパイラは順序を入れ替えるかもしれない
// → ctrl が先になるリスク
```

読み書き順序を固定したい場合は、`asm volatile("" ::: "memory");` でバリアを張る必要があります。

### **`volatile` を付けたからスレッドセーフ、という主張は誤りです。**

スレッドセーフが必要なら：
- `pthread_mutex_t`（ミューテックス）を使う
- `atomic_*` 型（C11 の原子型）を使う
- 割り込みを禁止する

## 10.4 `volatile sig_atomic_t` — シグナルハンドラから安全にアクセス

**シグナルハンドラから値を読み書きできるのは、POSIX では `sig_atomic_t` 型だけです。**

```c
#include <signal.h>

volatile sig_atomic_t signal_received = 0;

void handler(int sig)
{
    signal_received = 1;  // 安全
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

実測値：

```
Signal received
```

**`sig_atomic_t` とは何か：**
- 割り込み不可能な読み書きが保証される型
- 多くのプラットフォームでは `int` と同じ大きさ
- `volatile` と一緒に使う

**ハンドラ内で他の型を使うと、未定義動作になります。**

## 10.5 ハードウェアレジスタを読み書きする

**マイコンでは、メモリアドレスを指定してハードウェアレジスタにアクセスします。**

```c
// STM32 UART 例
volatile uint32_t *uart_dr = (volatile uint32_t *)0x40004000;   // データ
volatile uint32_t *uart_sr = (volatile uint32_t *)0x40004004;   // ステータス

// ステータスレジスタを読む
uint32_t status = *uart_sr;

// データを書く
*uart_dr = 'A';

// ステータスレジスタを読む（2回目）
status = *uart_sr;
```

実測値（`gcc -O2` でのアセンブリ）：

```asm
movl	uart_dr(%rip), %rsi      # pointer をレジスタに
movl	(%rsi), %eax              # 最初の読み出し
movl	(%rsi), %eax              # 2 回目も新しい値を読む
```

**`volatile` を付けないと：**

```asm
movl	uart_dr(%rip), %rsi
movl	(%rsi), %eax
# → 2 回目の読み出しが消える！
```

レジスタには必ず `volatile` を付けます。

## 10.6 Read-Modify-Write（RMW）の危険性

**`volatile` を付けても、RMW 操作はアトミックになりません。**

```c
volatile uint32_t gpio = 0x00000000;

// ビット 3 を立てたい
gpio |= (1 << 3);     // 実は 3 ステップ：読む → OR → 書く
```

アセンブリ（`-O2`）：

```asm
movl	gpio(%rip), %eax         # 1. メモリから読む
orl	    $8, %eax                # 2. OR する
movl	%eax, gpio(%rip)         # 3. メモリに書く
```

ハードウェアレジスタの場合、読み出し時点でのレジスタ状態と、書き込み時点での実際の値が異なっている可能性があります。

**安全にビットを操作するには：**

1. **割り込みを禁止する**
   ```c
   unsigned long flags = disable_irq_save();  // 割り込み禁止
   gpio |= (1 << 3);
   restore_irq(flags);                        // 割り込み再開
   ```

2. **ハードウェアが提供する「ビット設定用レジスタ」を使う**（STM32 など）
   ```c
   // GPIO_BSRR: ビット単位で 1 ステップで設定
   *gpio_bsrr = (1 << 3);
   ```

3. **アトミック操作ライブラリを使う**
   ```c
   #include <stdatomic.h>
   atomic_uint gpio = 0;
   atomic_fetch_or(&gpio, (1 << 3));
   ```

## 手元で試す

`volatile` なしとありの違いを確認します。

```c
// volatile_test.c
#include <stdio.h>
#include <stdint.h>

// テスト 1: ハードウェアレジスタを読む
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

// テスト 2: ビット操作
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

**予想: ハードウェアレジスタから 3 回読んでも `0x11111111` が返るか。GPIO レジスタのビット操作が正しく実行されるか。**

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic volatile_test.c -o volatile_test && ./volatile_test
```

<details markdown="1"><summary>解答（実行結果）</summary>

```
Register reads (volatile):
  Read 1: 0x11111111
  Read 2: 0x11111111
  Read 3: 0x11111111

Bit manipulation (volatile):
Initial: 0x00000000
After |= (1 << 3): 0x00000008
After |= (1 << 7): 0x00000088
After |= ~(1 << 3): 0x00000080

Note: volatile does NOT guarantee atomicity!
RMW operations can be interrupted mid-operation.
```

</details>

3 点確認してください。

<details markdown="1"><summary>解答（答え合わせ）</summary>

1. ハードウェアレジスタから 3 回読んでも、毎回メモリから新しい値を取得していることが確認できるか。
2. ビット操作の結果が期待通りになるか（ただし、割り込みがないので成功するだけで、本来は危険）。
3. `volatile` を外してコンパイルした場合、アセンブリでどう変わるか（読み出しが 1 回に畳まれるはず）。

</details>

## つまずきポイント

**「`volatile` を付ければスレッドセーフになる」という誤解**
`volatile` はメモリ読み出しを強制するだけです。アトミック性やメモリバリアは保証しません。スレッドセーフが必要なら、ミューテックスや原子操作を使ってください。

**RMW 操作が割り込みで中断される**
`counter++` や `gpio |= mask` は、読む・計算・書くの 3 ステップです。割り込みが間に入ると、カウントが失われます。

**ハードウェアレジスタに `volatile` を付け忘れ**
レジスタへの読み出しが最適化で消えて、古い値を使い続けます。結果として、ハードウェアの状態が反映されず、通信失敗や制御誤りになります。

**メモリバリアがない**
複数の `volatile` 変数の操作順序が入れ替わる可能性があります。順序が重要な場合（例：制御フラグの前にデータを書く）、明示的なバリアが必要です。

## 対応する課題

この章を読んだら、対応するドリルで手を動かしてください。

- `c10_volatile` — volatile と共有変数

```bash
./drill run c10
```

詰まったら `./drill hint c10`、課題側からは `./drill read c10` でこの章に戻ってこられます。

## 参考

- [cppreference: volatile type qualifier](https://en.cppreference.com/w/c/language/volatile)
- [POSIX: sig_atomic_t](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/signal.h.html)

---

前章 → [9. 関数ポインタ](09_関数ポインタ.md)
次章 → [11. エンディアンとシリアライズ](11_エンディアンとシリアライズ.md)
