/* arch/x86_64/hal/pic.h - 8259A PIC: remap, mask, and end-of-interrupt. */
#pragma once
#include <stdint.h>
#include "arch/x86_64/hal/io.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI   0x20

/* Move the two PICs' vectors to off1/off2 (defaults clash with CPU
 * exception vectors 0x08-0x0F, so this must run before enabling IRQs). */
static inline void pic_remap(uint8_t off1, uint8_t off2)
{
    uint8_t m1 = inb(PIC1_DATA);
    uint8_t m2 = inb(PIC2_DATA);

    outb(PIC1_CMD, 0x11); io_wait();      /* ICW1: init + expect ICW4 */
    outb(PIC2_CMD, 0x11); io_wait();
    outb(PIC1_DATA, off1); io_wait();     /* ICW2: vector offset      */
    outb(PIC2_DATA, off2); io_wait();
    outb(PIC1_DATA, 0x04); io_wait();     /* ICW3: slave on IRQ2      */
    outb(PIC2_DATA, 0x02); io_wait();
    outb(PIC1_DATA, 0x01); io_wait();     /* ICW4: 8086 mode          */
    outb(PIC2_DATA, 0x01); io_wait();

    outb(PIC1_DATA, m1);                  /* restore saved masks      */
    outb(PIC2_DATA, m2);
}

/* 16-bit mask, bit n set => IRQ n disabled. */
static inline void pic_set_mask(uint16_t mask)
{
    outb(PIC1_DATA, (uint8_t)(mask & 0xFF));
    outb(PIC2_DATA, (uint8_t)(mask >> 8));
}

static inline void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}
