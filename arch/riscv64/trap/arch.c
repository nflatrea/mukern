/* arch/riscv64/trap/arch.c - scheduler arch hooks for riscv64. */
#include <stdint.h>
#include "sched.h"

extern void thread_trampoline(void);

/* Plant a new thread's stack so context_switch's restore+ret lands in
 * thread_trampoline with s1=entry, s2=arg. Frame layout matches switch.S:
 * ra@0, s0@8, s1@16, s2@24, ... s11@96 (14 slots, 16-byte aligned). */
uint64_t arch_init_stack(uint64_t stack_top, thread_fn fn, void *arg)
{
    uint64_t *sp = (uint64_t *)stack_top - 14;
    for (int i = 0; i < 14; i++) sp[i] = 0;
    sp[0] = (uint64_t)thread_trampoline;    /* ra */
    sp[2] = (uint64_t)fn;                   /* s1 */
    sp[3] = (uint64_t)arg;                  /* s2 */
    return (uint64_t)sp;
}

/* No user mode yet, so traps always land on the current kernel stack. */
void arch_set_kernel_stack(uint64_t top) { (void)top; }

/* The scheduler zeroes a thread's capability table on creation. Capabilities
 * and IPC are not part of this bring-up milestone, so provide just this hook;
 * kernel/cap.c (and ipc.c) join the build when IPC is ported. */
#include "cap.h"
void cap_clear(struct thread *t)
{
    for (int i = 0; i < CAP_MAX; i++) {
        t->ctable[i].type   = CAP_NULL;
        t->ctable[i].target = -1;
        t->ctable[i].rights = 0;
        t->ctable[i].io_base = 0;
        t->ctable[i].io_len  = 0;
    }
}
