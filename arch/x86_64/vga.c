/*
file: arch/x86_64/vga.c
desc: VGA text-mode back-end (hal.h), x86_64 only. The classic 80x25 cell
      buffer at physical 0xB8000; each cell is a byte of ASCII plus a byte of
      attribute. boot.S identity-maps the low 1 GiB, so 0xB8000 is directly
      addressable.

      Like serial.c this is raw hardware glue owned at runtime by a driver
      SERVER task (kernel/srv/vga.c); the kernel core never calls it. It
      handles \n, \r, \b, tabs and scrolling so the console server can treat
      it as an ordinary character device.

      We keep a small RAM shadow of the screen and treat it as the source of
      truth, writing every change through to both the shadow and the live
      framebuffer. Scrolling then shifts the shadow and re-blits it. This
      deliberately never *reads* the framebuffer: under the std VGA's
      odd/even text addressing a CPU read does not reliably return the
      char/attr pair that was written, so a read-modify scroll would corrupt
      the screen. The shadow sidesteps that entirely.
*/
#include <stdint.h>
#include "hal.h"

#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_CELLS (VGA_COLS * VGA_ROWS)
#define VGA_ATTR 0x07              /* light grey on black */

static volatile uint16_t *const VGA = (volatile uint16_t *)0xB8000;
static uint16_t shadow[VGA_CELLS];          /* RAM source of truth */
static int cur_row, cur_col;

static uint16_t cell(char c)
{
    return (uint16_t)((uint8_t)c) | ((uint16_t)VGA_ATTR << 8);
}

static void put_cell(int idx, uint16_t v)
{
    shadow[idx] = v;
    VGA[idx] = v;
}

void vga_clear(void)
{
    for (int i = 0; i < VGA_CELLS; i++)
        put_cell(i, cell(' '));
    cur_row = cur_col = 0;
}

void vga_init(void)
{
    vga_clear();
}

/* Shift every row up one, blank the last, re-blit. Operates on the shadow so
 * it never reads from VRAM. */
static void scroll(void)
{
    for (int i = 0; i < VGA_CELLS - VGA_COLS; i++)
        shadow[i] = shadow[i + VGA_COLS];
    for (int i = VGA_CELLS - VGA_COLS; i < VGA_CELLS; i++)
        shadow[i] = cell(' ');
    for (int i = 0; i < VGA_CELLS; i++)
        VGA[i] = shadow[i];
    cur_row = VGA_ROWS - 1;
}

void vga_putc(char c)
{
    switch (c) {
    case '\n':
        cur_col = 0;
        cur_row++;
        break;
    case '\r':
        cur_col = 0;
        break;
    case '\b':
        if (cur_col > 0) {
            cur_col--;
            put_cell(cur_row * VGA_COLS + cur_col, cell(' '));
        }
        break;
    case '\t':
        cur_col = (cur_col + 8) & ~7;
        break;
    default:
        put_cell(cur_row * VGA_COLS + cur_col, cell(c));
        cur_col++;
        break;
    }

    if (cur_col >= VGA_COLS) {
        cur_col = 0;
        cur_row++;
    }
    if (cur_row >= VGA_ROWS)
        scroll();
}
