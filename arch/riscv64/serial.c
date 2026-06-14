/* arch/riscv64/serial.c - NS16550 UART at the QEMU virt base (reg-shift 0). */
#include <stdint.h>
#include "console.h"

#define UART      0x10000000UL
#define REG(o)    (*(volatile uint8_t *)(UART + (o)))
#define RBR       0   /* receive buffer (read)  */
#define THR       0   /* transmit hold (write)  */
#define LSR       5   /* line status            */
#define LSR_DR    0x01 /* data ready            */
#define LSR_THRE  0x20 /* transmit hold empty   */

void console_init(void)
{
    /* OpenSBI has already configured the UART for us. */
}

void console_putc(char c)
{
    while (!(REG(LSR) & LSR_THRE))
        ;
    REG(THR) = (uint8_t)c;
}

int console_getc(void)
{
    if (!(REG(LSR) & LSR_DR))
        return -1;
    return (int)REG(RBR);
}

const char *arch_name(void) { return "riscv64 (RV64, NS16550)"; }

void arch_halt(void)
{
    for (;;)
        __asm__ volatile("wfi");
}
