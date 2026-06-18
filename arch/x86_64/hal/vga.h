/* arch/x86_64/hal/vga.h - optional VGA text-mode mirror (compile with
 * -DMUKERN_VGA to enable). Kept header-only to match the HAL philosophy. */
#pragma once
#include <stdint.h>

#define VGA_MEM   0xB8000
#define VGA_COLS  80
#define VGA_ROWS  25
#define VGA_ATTR  0x07   /* light grey on black */

static inline void arch_vga_init(void)
{
    volatile uint16_t *m = (volatile uint16_t *)VGA_MEM;
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
        m[i] = (VGA_ATTR << 8) | ' ';
}

static inline void arch_vga_putc(char c)
{
    static int pos;
    volatile uint16_t *m = (volatile uint16_t *)VGA_MEM;
    if (c == '\n') {
        pos += VGA_COLS - (pos % VGA_COLS);
    } else {
        m[pos++] = (VGA_ATTR << 8) | (uint8_t)c;
    }
    if (pos >= VGA_COLS * VGA_ROWS)
        pos = 0;
}
