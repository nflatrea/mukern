/* arch/x86_64/serial.c - COM1 (8250/16550) via port I/O. */
#include <stdint.h>
#include "console.h"

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

void console_init(void)
{
    outb(COM1 + 1, 0x00); /* disable interrupts        */
    outb(COM1 + 3, 0x80); /* enable DLAB               */
    outb(COM1 + 0, 0x03); /* divisor low  (38400 baud) */
    outb(COM1 + 1, 0x00); /* divisor high              */
    outb(COM1 + 3, 0x03); /* 8 bits, no parity, 1 stop */
    outb(COM1 + 2, 0xC7); /* enable + clear FIFOs       */
    outb(COM1 + 4, 0x0B); /* RTS/DSR set                */
}

void console_putc(char c)
{
    while (!(inb(COM1 + 5) & 0x20)) /* wait THR empty */
        ;
    outb(COM1, (uint8_t)c);
}

int console_getc(void)
{
    if (!(inb(COM1 + 5) & 0x01)) /* data ready? */
        return -1;
    return (int)inb(COM1);
}

const char *arch_name(void) { return "x86_64"; }

void arch_halt(void)
{
    for (;;)
        __asm__ volatile("cli; hlt");
}
