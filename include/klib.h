/*
file: klib.h
desc: Freestanding kernel library: formatted output, panic, mem/str.
      The whole library depends only on console.h (console_putc) and arch_halt.
*/
#ifndef MUKERN_KLIB_H
#define MUKERN_KLIB_H

#include <stdarg.h>
#include <stddef.h>

/* formatted console output -------------------------------------------------
 * Supported conversions: %c %s %d %i %u %x %X %p %%
 * Optional '0' flag, a numeric width, and 'l'/'ll' length (both 64-bit on
 * LP64). Enough to dump registers in neat columns; nothing more.            */
void kputc(char c);
void kputs(const char *s);
void kvprintf(const char *fmt, va_list ap);
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
