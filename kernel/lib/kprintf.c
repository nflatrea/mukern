/*
file: kprintf.c
desc: A small printf for the kernel. Writes through console_putc.
*/
#include <stdint.h>
#include "klib.h"
#include "console.h"

void kputc(char c) { console_putc(c); }

void kputs(const char *s)
{
    while (*s)
        console_putc(*s++);
}

static void put_uint(unsigned long long v, unsigned base, int upper,
                     int width, char pad)
{
    char buf[32];
    int n = 0;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";

    if (v == 0)
        buf[n++] = '0';
    while (v) {
        buf[n++] = digits[v % base];
        v /= base;
    }
    for (int i = n; i < width; i++)
        console_putc(pad);
    while (n--)
        console_putc(buf[n]);
}

static void put_int(long long v, int width, char pad)
{
    if (v < 0) {
        console_putc('-');
        put_uint((unsigned long long)(-v), 10, 0,
                 width > 0 ? width - 1 : 0, pad);
    } else {
        put_uint((unsigned long long)v, 10, 0, width, pad);
    }
}

void kvprintf(const char *fmt, va_list ap)
{
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            console_putc(*fmt);
            continue;
        }
        fmt++;

        char pad = ' ';
        int width = 0, lng = 0;

        if (*fmt == '0') {
            pad = '0';
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        while (*fmt == 'l') {
            lng++;
            fmt++;
        }

        switch (*fmt) {
        case 'c':
            console_putc((char)va_arg(ap, int));
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            int len = 0;
            while (s[len])
                len++;
            for (int i = len; i < width; i++)
                console_putc(pad);
            kputs(s);
            break;
        }
        case 'd':
        case 'i': {
            long long v = lng ? va_arg(ap, long long)
                              : (long long)va_arg(ap, int);
            put_int(v, width, pad);
            break;
        }
        case 'u': {
            unsigned long long v = lng ? va_arg(ap, unsigned long long)
                                       : (unsigned long long)va_arg(ap, unsigned);
            put_uint(v, 10, 0, width, pad);
            break;
        }
        case 'x':
        case 'X': {
            unsigned long long v = lng ? va_arg(ap, unsigned long long)
                                       : (unsigned long long)va_arg(ap, unsigned);
            put_uint(v, 16, *fmt == 'X', width, pad);
            break;
        }
        case 'p':
            console_putc('0');
            console_putc('x');
            put_uint((unsigned long long)(uintptr_t)va_arg(ap, void *),
                     16, 0, 16, '0');
            break;
        case '%':
            console_putc('%');
            break;
        case '\0':
            return;
        default:
            console_putc('%');
            console_putc(*fmt);
            break;
        }
    }
}

void kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
}
