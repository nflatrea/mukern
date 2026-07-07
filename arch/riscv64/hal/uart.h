/* arch/riscv64/hal/uart.h - NS16550 UART on the QEMU 'virt' machine.
 * OpenSBI configures the UART before entering S-mode, so init is a no-op.
 * Before paging is enabled the kernel runs in bare mode (VA == PA), so the
 * device is reachable at its physical address.
 */
#pragma once
#include <stdint.h>

#define RV_UART0 0x10000000UL
#define UART_THR 0          /* transmit holding register */
#define UART_LSR 5          /* line status; bit 5 = THR empty */

static inline volatile unsigned char *rv_uart(int off)
{
    return (volatile unsigned char *)(RV_UART0 + (unsigned)off);
}
static inline void arch_uart_init(void) { }
static inline void arch_uart_putc(char c)
{
    while (!(*rv_uart(UART_LSR) & 0x20)) { }
    *rv_uart(UART_THR) = (unsigned char)c;
}
