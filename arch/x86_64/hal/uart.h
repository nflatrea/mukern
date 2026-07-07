/* arch/x86_64/hal/uart.h - 8250/16550 UART (COM1) HAL.
 *
 * NOTE: the DESIGN doc sketches the UART as an MMIO struct at 0x3F8. On
 * x86_64 the legacy serial port is reached through the I/O *port* space, not
 * memory, so this implements the same HAL surface (arch_uart_init /
 * arch_uart_putc) using inb/outb. The function names match hal.h's contract.
 */
#pragma once
#include <stdint.h>
#include "arch/x86_64/hal/io.h"

#define UART_COM1   0x3F8
#define UART_DATA   0   /* RBR/THR (and divisor low when DLAB=1) */
#define UART_IER    1   /* interrupt enable (divisor high when DLAB=1) */
#define UART_FCR    2   /* FIFO control */
#define UART_LCR    3   /* line control */
#define UART_MCR    4   /* modem control */
#define UART_LSR    5   /* line status */

#define LSR_THR_EMPTY 0x20

static inline void arch_uart_init(void)
{
    outb(UART_COM1 + UART_IER,  0x00);   /* disable UART interrupts        */
    outb(UART_COM1 + UART_LCR,  0x80);   /* DLAB=1: program baud divisor   */
    outb(UART_COM1 + UART_DATA, 0x03);   /* divisor low  = 3  -> 38400 8N1 */
    outb(UART_COM1 + UART_IER,  0x00);   /* divisor high = 0               */
    outb(UART_COM1 + UART_LCR,  0x03);   /* DLAB=0, 8 bits, no parity, 1stop*/
    outb(UART_COM1 + UART_FCR,  0x01);   /* enable FIFO, 1-byte RX trigger, */
                                         /* do NOT clear: keep early input  */
    outb(UART_COM1 + UART_MCR,  0x0B);   /* DTR/RTS/OUT2 set               */
}

static inline int arch_uart_tx_ready(void)
{
    return inb(UART_COM1 + UART_LSR) & LSR_THR_EMPTY;
}

static inline void arch_uart_putc(char c)
{
    if (c == '\n') {                     /* cook LF -> CRLF for terminals  */
        while (!arch_uart_tx_ready()) { }
        outb(UART_COM1 + UART_DATA, '\r');
    }
    while (!arch_uart_tx_ready()) { }
    outb(UART_COM1 + UART_DATA, (uint8_t)c);
}
