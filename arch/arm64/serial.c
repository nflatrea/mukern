/* arch/arm64/serial.c - ARM PL011 UART at the QEMU virt base. */
#include <stdint.h>
#include "console.h"

#define UART0    0x09000000UL
#define UART_DR  (*(volatile uint32_t *)(UART0 + 0x00))
#define UART_FR  (*(volatile uint32_t *)(UART0 + 0x18))
#define FR_RXFE  0x10   /* receive FIFO empty */
#define FR_TXFF  0x20   /* transmit FIFO full */

void console_init(void)
{
    /* QEMU brings the PL011 up in a usable state already. */
}

void console_putc(char c)
{
    while (UART_FR & FR_TXFF)
        ;
    UART_DR = (uint32_t)(uint8_t)c;
}

int console_getc(void)
{
    if (UART_FR & FR_RXFE)
        return -1;
    return (int)(UART_DR & 0xFF);
}

const char *arch_name(void) { return "arm64 (AArch64, PL011)"; }

void arch_halt(void)
{
    for (;;)
        __asm__ volatile("wfe");
}
