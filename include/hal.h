/*
file: hal.h
desc: Device hardware-abstraction layer -- the raw, per-arch primitives that
      touch real hardware. These are NOT called by the portable core; they
      are the private back-end of the driver SERVER tasks (kernel/srv/) and
      of the emergency boot/panic console.

      Layering:
        hal.h   (this file)   raw device registers, one impl per arch
        con.h                 the IPC protocol that abstracts those devices
        kernel/srv/...        server tasks that own a device and speak con.h
        kernel/lib/con.c      thin client helpers used by the shell

      The kernel core depends on con.h, never on hal.h. Only the boot banner
      and panic() reach for uart_putc() directly, because they must work
      before (and after) the scheduler and IPC exist.
*/
#ifndef MUKERN_HAL_H
#define MUKERN_HAL_H

/* UART -- every arch implements these three (arch/<arch>/serial.c). */
void uart_init(void);   /* bring the UART up (may be a no-op)              */
void uart_putc(char c); /* blocking write of one byte                      */
int  uart_getc(void);   /* non-blocking read; returns -1 if no byte ready  */

/* VGA text framebuffer -- x86_64 only (arch/x86_64/vga.c). Other arches do
 * not compile or reference these; the vga driver server is spawned only
 * where HAVE_VGA is defined. */
#ifdef HAVE_VGA
void vga_init(void);          /* clear screen, home the cursor             */
void vga_putc(char c);        /* write one glyph; handles \n \r \b + scroll */
void vga_clear(void);         /* blank the screen                          */
#endif

/* PS/2 keyboard -- x86_64 only (arch/x86_64/kbd.c). The local-console input
 * counterpart of the VGA screen; spawned only where HAVE_KBD is defined. */
#ifdef HAVE_KBD
void kbd_init(void);          /* bring up / drain the i8042 controller     */
int  kbd_getc(void);          /* non-blocking: ASCII byte, or -1 if none   */
#endif

#endif /* MUKERN_HAL_H */
