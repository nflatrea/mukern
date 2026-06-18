/* arch/x86_64/hal/pit.h - 8253/8254 Programmable Interval Timer (IRQ0). */
#pragma once
#include <stdint.h>
#include "arch/x86_64/hal/io.h"

#define PIT_CH0   0x40
#define PIT_CMD   0x43
#define PIT_HZ    1193182u    /* base oscillator frequency */

/* Program channel 0 to fire IRQ0 at ~hz Hz (square wave, mode 3). */
static inline void pit_init(uint32_t hz)
{
    uint32_t div = PIT_HZ / hz;
    outb(PIT_CMD, 0x36);                       /* ch0, lo/hi byte, mode 3 */
    outb(PIT_CH0, (uint8_t)(div & 0xFF));
    outb(PIT_CH0, (uint8_t)((div >> 8) & 0xFF));
}
