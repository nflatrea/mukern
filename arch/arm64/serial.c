/* arch/arm64/serial.c - ARM PL011 UART at the QEMU virt base.
 * Raw UART back-end (hal.h); owned by the serial driver server at runtime. */
#include <stdint.h>
#include "hal.h"
#include "arch.h"

#define UART0    0x09000000UL
#define UART_DR  (*(volatile uint32_t *)(UART0 + 0x00))
#define UART_FR  (*(volatile uint32_t *)(UART0 + 0x18))
#define FR_RXFE  0x10   /* receive FIFO empty */
#define FR_TXFF  0x20   /* transmit FIFO full */

void uart_init(void)
{
    /* QEMU brings the PL011 up in a usable state already. */
}

void uart_putc(char c)
{
    while (UART_FR & FR_TXFF)
        ;
    UART_DR = (uint32_t)(uint8_t)c;
}

int uart_getc(void)
{
    if (UART_FR & FR_RXFE)
        return -1;
    return (int)(UART_DR & 0xFF);
}

const char *arch_name(void) { return "arm64 (AArch64, PL011)"; }

void arch_relax(void) { __asm__ volatile("yield"); }

void arch_halt(void)
{
    for (;;)
        __asm__ volatile("wfe");
}
