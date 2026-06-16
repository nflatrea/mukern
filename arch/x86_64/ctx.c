/* arch/x86_64/ctx.c - forge a fresh task stack for arch_switch(). */
#include <stdint.h>
#include "arch.h"

extern void task_trampoline(void);

void *arch_ctx_init(void *stack_top, void (*entry)(void *), void *arg)
{
    uint64_t *sp = (uint64_t *)((uintptr_t)stack_top & ~15UL);

    /* Mirror of arch_switch's pop order + the final `ret`:
     *   low -> high :  r15 r14 r13 r12 rbx rbp  ret_addr            */
    *--sp = (uint64_t)task_trampoline;  /* ret -> trampoline */
    *--sp = 0;                          /* rbp               */
    *--sp = 0;                          /* rbx               */
    *--sp = (uint64_t)entry;            /* r12 = entry       */
    *--sp = (uint64_t)arg;              /* r13 = arg         */
    *--sp = 0;                          /* r14               */
    *--sp = 0;                          /* r15               */
    return sp;
}
