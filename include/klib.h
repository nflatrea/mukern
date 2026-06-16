/*
file: klib.h
desc: Freestanding kernel library: formatted output, panic, mem/str.

      Two printf front-ends share one formatter (kvsnprintf):
        kprintf  -- the kernel DEBUG console. Writes straight to the UART via
                    uart_putc(). Used for the boot banner, panic() and trap
                    dumps, i.e. paths that must work with no scheduler or IPC.
        con_printf (con.h) -- normal task output, formatted into a buffer and
                    shipped to the console server over IPC.

      Keeping kprintf hard-wired to the UART is deliberate: a panic must be
      printable even if the IPC system or a driver server is the thing that
      broke.
*/
#ifndef MUKERN_KLIB_H
#define MUKERN_KLIB_H

#include <stdarg.h>
#include <stddef.h>

/* core formatter: render into out[0..n-1], NUL-terminate, return the number
 * of characters that would have been written (C99 vsnprintf semantics).
 * Conversions: %c %s %d %i %u %x %X %p %%, with optional '0' flag, numeric
 * width, and 'l'/'ll' length (both 64-bit on LP64). */
int  kvsnprintf(char *out, size_t n, const char *fmt, va_list ap);
int  ksnprintf (char *out, size_t n, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/* kernel debug console: formatted output straight to the UART. */
void kputc(char c);
void kputs(const char *s);
void kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* fatal error: print and stop the CPU. Never returns. */
void panic(const char *fmt, ...)
    __attribute__((format(printf, 1, 2), noreturn));

/* minimal mem/str */
void  *memset(void *dst, int c, size_t n);
void  *memcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);

#endif /* MUKERN_KLIB_H */
