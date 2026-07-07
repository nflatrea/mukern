/* arch/x86_64/trap/archstack.c - plant a new thread's initial kernel stack.
 *
 * Builds the hand-crafted frame that context_switch's `ret` unwinds into:
 * thread_trampoline with the entry function in r12 and its argument in r13.
 */
#include <stdint.h>
#include "sched.h"

extern void thread_trampoline(void);

uint64_t arch_init_stack(uint64_t stack_top, thread_fn fn, void *arg)
{
    uint64_t *sp = (uint64_t *)stack_top - 7;
    sp[0] = 0;                          /* r15 */
    sp[1] = 0;                          /* r14 */
    sp[2] = (uint64_t)arg;              /* r13 */
    sp[3] = (uint64_t)fn;               /* r12 */
    sp[4] = 0;                          /* rbx */
    sp[5] = 0;                          /* rbp */
    sp[6] = (uint64_t)thread_trampoline;
    return (uint64_t)sp;
}
