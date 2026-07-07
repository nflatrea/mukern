/* kernel/main.c - muKern bootstrap (Phase 4).
 *
 * The kernel is now frozen: it brings up hardware, initialises the physical
 * allocator and scheduler, loads the user-space servers from the embedded
 * rootfs, and hands control to the init server. From here all policy lives in
 * user space (init, pm, vm_server); the kernel only services syscalls,
 * interrupts, and page faults forwarded to the pager.
 */
#include <stdint.h>
#include "hal.h"
#include "vm.h"
#include "sched.h"
#include "proc.h"

#define TIMER_HZ 100u

static void kputs(const char *s) { while (*s) hal_console_putc(*s++); }

static void on_timer_tick(void) { schedule(); }

void kmain(void)
{
    hal_console_init();
    kputs("Kernel Started\n");

    hal_traps_init();                   /* GDT/TSS, PIC, IDT */
    pmm_init();                         /* physical frames   */
    sched_init();                       /* threads + idle    */

    rootfs_load_all();                  /* map servers into user memory */
    proc_start_init();                  /* spawn init as the first task */

    hal_timer_init(TIMER_HZ, on_timer_tick);
    hal_irq_enable();

    kputs("[mukern] init loaded; kernel idle.\n");
    for (;;)
        hal_halt();                     /* the idle thread */
}
