/* arch/x86_64/trap/trap.c - interrupt demux + trap/timer HAL (M1b).
 *
 * This is mechanism, not policy: it identifies the interrupt source and (for
 * now) forwards timer ticks to a registered callback. In later phases the
 * timer/IRQ path becomes an IPC message to a user-space driver thread.
 */
#include <stdint.h>
#include "arch/x86_64/trap/idt.h"
#include "arch/x86_64/hal/pic.h"
#include "arch/x86_64/hal/pit.h"
#include "hal.h"

#define VEC_IRQ_BASE 32          /* PIC remapped so IRQ0 == vector 32 */
#define VEC_TIMER    (VEC_IRQ_BASE + 0)

static void (*g_on_tick)(void);

/* Minimal panic-print for unexpected CPU exceptions during bring-up. */
static void put_dec(uint64_t v)
{
    char buf[21];
    int i = 0;
    if (v == 0) { hal_console_putc('0'); return; }
    while (v) { buf[i++] = (char)('0' + (v % 10)); v /= 10; }
    while (i) hal_console_putc(buf[--i]);
}

static void puts_raw(const char *s)
{
    while (*s) hal_console_putc(*s++);
}

void isr_handler(struct trapframe *tf)
{
    if (tf->vec == VEC_TIMER) {
        if (g_on_tick) g_on_tick();
        pic_send_eoi(0);
        return;
    }

    if (tf->vec < VEC_IRQ_BASE) {                 /* CPU exception -> halt  */
        puts_raw("\n[mukern] EXCEPTION vec=");
        put_dec(tf->vec);
        puts_raw(" err=");
        put_dec(tf->err);
        puts_raw(" rip=");
        put_dec(tf->rip);
        hal_console_putc('\n');
        for (;;) { hal_irq_disable(); hal_halt(); }
    }

    /* Any other (currently masked) IRQ: acknowledge and move on. */
    pic_send_eoi((uint8_t)(tf->vec - VEC_IRQ_BASE));
}

/* ---- HAL surface used by the kernel ---- */
void hal_traps_init(void)
{
    pic_remap(VEC_IRQ_BASE, VEC_IRQ_BASE + 8);
    pic_set_mask((uint16_t)~(1u << 0));   /* unmask IRQ0 (timer) only */
    idt_init();
}

void hal_timer_init(uint32_t hz, void (*on_tick)(void))
{
    g_on_tick = on_tick;
    pit_init(hz);
}
