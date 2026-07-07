/* arch/riscv64/kmain.c - riscv64 bring-up milestone (Phase 1+2 equivalent).
 *
 * Boots in S-mode, brings up the console, installs traps, starts the timer,
 * and runs two kernel threads under the *generic* round-robin scheduler. The
 * threads never yield voluntarily, so seeing their output interleave proves
 * the timer is preempting them - real preemptive multitasking on a second ISA,
 * driven by the architecture-neutral kernel/sched.c.
 */
#include <stdint.h>
#include "hal.h"
#include "sched.h"

static void kputs(const char *s) { while (*s) hal_console_putc(*s++); }

static void worker(void *arg)
{
    const char *tag = (const char *)arg;
    for (int i = 0; i < 30; i++) {
        kputs(tag);
        for (volatile int d = 0; d < 1500000; d++) { }   /* burn ~a timeslice */
    }
    kputs(arg == 0 ? "" : "");
}

static void on_tick(void) { schedule(); }

void kmain(void)
{
    hal_console_init();
    kputs("\nmuKern/riscv64 booting in S-mode (QEMU virt)\n");

    hal_traps_init();
    sched_init();
    thread_create(worker, (void *)"A");
    thread_create(worker, (void *)"B");

    hal_timer_init(100, on_tick);   /* 100 Hz */
    hal_irq_enable();
    kputs("[rv] timer armed; preempting 2 kernel threads via generic scheduler:\n");

    for (;;)
        hal_halt();                 /* idle thread */
}
