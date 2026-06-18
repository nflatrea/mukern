/* kernel/main.c - muKern bootstrap (Phase 1).
 *
 * Mechanism-only entry point. For Phase 1 this just brings the platform up
 * and proves the boot + interrupt path works (M1a: print on boot; M1b: a
 * dot per second from the timer ISR). From Phase 4 onward kmain's job shrinks
 * to loading the init server and stepping aside.
 */
#include <stdint.h>
#include "hal.h"

#define TIMER_HZ 100u

static void kputs(const char *s)
{
    while (*s)
        hal_console_putc(*s++);
}

/* Called from the timer ISR. Counts ticks and emits one '.' per second. */
static volatile uint32_t g_ticks;

static void on_timer_tick(void)
{
    if (++g_ticks % TIMER_HZ == 0)
        hal_console_putc('.');
}

void kmain(void)
{
    hal_console_init();
    kputs("Kernel Started\n");                 /* M1a success criterion */

    hal_traps_init();                           /* IDT + PIC remap       */
    hal_timer_init(TIMER_HZ, on_timer_tick);    /* PIT @ 100 Hz          */
    hal_irq_enable();

    kputs("[mukern] long mode + interrupts up; ticking (one '.' per second)\n");

    for (;;)
        hal_halt();                             /* idle until next IRQ   */
}
