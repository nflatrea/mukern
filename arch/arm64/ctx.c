/* arch/arm64/ctx.c - forge a fresh task stack for arch_switch(). */
#include <stdint.h>
#include "arch.h"

extern void task_trampoline(void);

void *arch_ctx_init(void *stack_top, void (*entry)(void *), void *arg)
{
    uint64_t *s = (uint64_t *)((uintptr_t)stack_top & ~15UL);
    s -= 12;                       /* x19..x30, low->high */
    s[0]  = (uint64_t)entry;            /* x19 */
    s[1]  = (uint64_t)arg;              /* x20 */
    s[2]  = 0;  s[3] = 0;  s[4] = 0;    /* x21..x23 */
    s[5]  = 0;  s[6] = 0;  s[7] = 0;    /* x24..x26 */
    s[8]  = 0;  s[9] = 0;               /* x27..x28 */
    s[10] = 0;                          /* x29 (fp) */
    s[11] = (uint64_t)task_trampoline;  /* x30 (lr) -> ret target */
    return s;
}
