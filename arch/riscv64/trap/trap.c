/* arch/riscv64/trap/trap.c - S-mode trap setup, dispatch, and timer.
 *
 * The timer uses the SBI legacy set_timer call (which also clears the pending
 * supervisor timer interrupt) and the `time` CSR. The QEMU 'virt' timebase is
 * 10 MHz. On each tick we re-arm and invoke the scheduler hook.
 */
#include <stdint.h>
#include "hal.h"

#define RV_TIMEBASE 10000000UL          /* QEMU virt: 10 MHz */
#define SCAUSE_INT   (1UL << 63)
#define IRQ_S_TIMER  5

static void   (*g_on_tick)(void);
static uint64_t g_interval;

static inline uint64_t rdtime(void)
{
    uint64_t t;
    __asm__ volatile ("rdtime %0" : "=r"(t));
    return t;
}
static inline void sbi_set_timer(uint64_t when)
{
    register uint64_t a0 __asm__("a0") = when;
    register uint64_t a7 __asm__("a7") = 0;     /* legacy EID 0: set_timer */
    __asm__ volatile ("ecall" : "+r"(a0) : "r"(a7) : "memory");
}

void hal_traps_init(void)
{
    extern void trap_entry(void);
    __asm__ volatile ("csrw stvec, %0" :: "r"((uint64_t)trap_entry));
    __asm__ volatile ("csrs sie, %0"   :: "r"(1UL << IRQ_S_TIMER));  /* STIE */
}

void hal_timer_init(uint32_t hz, void (*on_tick)(void))
{
    g_on_tick = on_tick;
    g_interval = RV_TIMEBASE / hz;
    sbi_set_timer(rdtime() + g_interval);
}

void trap_handler(uint64_t scause)
{
    if ((scause & SCAUSE_INT) && (scause & 0xff) == IRQ_S_TIMER) {
        sbi_set_timer(rdtime() + g_interval);   /* re-arm; clears the pending IRQ */
        if (g_on_tick)
            g_on_tick();
    }
    /* other traps: ignored for this milestone (no user mode / faults yet) */
}
