/* arch/x86_64/hal/io.h - x86 port I/O primitives (header-only, zero overhead). */
#pragma once
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t r;
    __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

/* Short delay by bouncing a byte off an unused port (POST diagnostic 0x80). */
static inline void io_wait(void)
{
    outb(0x80, 0);
}
