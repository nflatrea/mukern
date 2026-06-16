/* arch/riscv64/ctx.c - forge a fresh task stack for arch_switch(). */
#include <stdint.h>
#include "arch.h"

extern void task_trampoline(void);

void *arch_ctx_init(void *stack_top, void (*entry)(void *), void *arg)
{
    uint64_t *s = (uint64_t *)((uintptr_t)stack_top & ~15UL);
    s -= 14;                            /* ra + s0..s11 (+pad), low->high */
    s[0] = (uint64_t)task_trampoline;   /* ra -> ret target */
    s[1] = (uint64_t)entry;             /* s0 */
    s[2] = (uint64_t)arg;               /* s1 */
    for (int i = 3; i < 14; i++)
        s[i] = 0;                       /* s2..s11 + pad */
    return s;
}
