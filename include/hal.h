/* include/hal.h - generic Hardware Abstraction Layer.
 *
 * The kernel includes only this header; it forwards to the architecture's
 * HAL based on the build-time ARCH_* define. Console primitives are
 * static-inline so the abstraction compiles away to direct register access;
 * trap/timer bring-up lives in arch/<arch>/trap and is declared here.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>

#if defined(ARCH_x86_64)
#  include "arch/x86_64/hal/io.h"
#  include "arch/x86_64/hal/uart.h"
#  include "arch/x86_64/hal/vga.h"
#else
#  error "muKern Phase 1 targets x86_64 only (other arches: ROADMAP Phase 6)"
#endif

/* ---- Console (mechanism only; string/printf policy lives elsewhere) ---- */
static inline void hal_console_init(void)
{
    arch_uart_init();
#ifdef MUKERN_VGA
    arch_vga_init();
#endif
}

static inline void hal_console_putc(char c)
{
    arch_uart_putc(c);
#ifdef MUKERN_VGA
    arch_vga_putc(c);
#endif
}

/* ---- CPU interrupt line ---- */
static inline void hal_irq_enable(void)  { __asm__ volatile ("sti"); }
static inline void hal_irq_disable(void) { __asm__ volatile ("cli"); }
static inline void hal_halt(void)        { __asm__ volatile ("hlt"); }

/* ---- Traps & timer (implemented in arch/<arch>/trap) ---- */
void hal_traps_init(void);
void hal_timer_init(uint32_t hz, void (*on_tick)(void));
