/*
file: con.h
desc: The "con" IPC protocol -- how clients talk to console/terminal servers,
      and the thin client helpers that wrap it.

      This single protocol abstracts every output/input device. A driver
      server (serial, vga) implements the subset it supports; the console
      multiplexer implements all of it by fanning requests out to the
      drivers. The shell speaks only con.h and so has no idea whether its
      bytes land on a UART, a VGA screen, or both -- the whole point of a
      microkernel HAL.

      Wire format (msg_t):
        tag = CON_*          w[0] = char (for PUTC / reply of GETC)
        ptr,len              the string (for PUTS)
*/
#ifndef MUKERN_CON_H
#define MUKERN_CON_H

#include "task.h"

enum {
    CON_PUTC = 1,   /* w[0] = byte to write                                */
    CON_PUTS,       /* ptr,len = string to write                          */
    CON_GETC,       /* reply: w[0] = byte read (blocks until one arrives)  */
    CON_CLEAR,      /* clear the screen (no-op on serial)                  */
    CON_POLL,       /* reply: w[0] = byte read, or -1 if none ready (never */
                    /*        blocks). Lets the console wait on several    */
                    /*        input devices at once.                       */
};

/* Client helpers (kernel/lib/con.c). They issue ipc_call()s to the server
 * tid given in `srv`, so any task can drive any console server. */
void con_putc (tid_t srv, char c);
void con_puts (tid_t srv, const char *s);
int  con_getc (tid_t srv);
void con_clear(tid_t srv);

/* Formatted output over con (printf-style), buffered then sent as one PUTS. */
void con_printf(tid_t srv, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#endif /* MUKERN_CON_H */
