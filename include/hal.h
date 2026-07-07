/* include/hal.h - generic Hardware Abstraction Layer.
 *
 * The kernel includes only this header; it forwards to the architecture's HAL
 * based on the build-time ARCH_* define. Console primitives are static-inline
 * so the abstraction compiles away to direct register access; trap/timer
 * bring-up lives in arch/<arch>/trap and is declared here.
 */
#pragma once
#include <stdint.h>
#include <stddef.h>

#if defined(ARCH_x86_64)
#  include "arch/x86_64/hal/io.h"
#  include "arch/x86_64/hal/uart.h"
#  include "arch/x86_64/hal/vga.h"
static inline void hal_irq_enable(void)  { __asm__ volatile ("sti"); }
static inline void hal_irq_disable(void) { __asm__ volatile ("cli"); }
static inline void hal_halt(void)        { __asm__ volatile ("hlt"); }

#elif defined(ARCH_riscv64)
#  include "arch/riscv64/hal/uart.h"
/* sstatus.SIE is bit 1; csrsi/csrci take a 5-bit immediate. */
static inline void hal_irq_enable(void)  { __asm__ volatile ("csrsi sstatus, 2"); }
static inline void hal_irq_disable(void) { __asm__ volatile ("csrci sstatus, 2"); }
static inline void hal_halt(void)        { __asm__ volatile ("wfi"); }

#else
#  error "muKern: unsupported ARCH (x86_64, riscv64)"
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

/* ---- Traps & timer (implemented in arch/<arch>/trap) ---- */
void hal_traps_init(void);
void hal_timer_init(uint32_t hz, void (*on_tick)(void));
