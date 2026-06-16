/*
file: kprintf.c
desc: One small formatter (kvsnprintf) with two front-ends. kvsnprintf does
      all the work into a caller buffer; kprintf is the kernel debug console
      that renders to a stack buffer and pushes it out the UART. Normal task
      output uses con_printf (kernel/lib/con.c), which reuses kvsnprintf too.
*/
#include <stdint.h>
#include "klib.h"
#include "hal.h"

/* --- bounded sink ------------------------------------------------------ */
struct sink { char *buf; size_t cap; size_t len; };

static void emit(struct sink *s, char c)
{
    if (s->len < s->cap)
        s->buf[s->len] = c;
    s->len++;                 /* always count, for vsnprintf return value */
}

static void emit_str(struct sink *s, const char *str)
{
    while (*str)
        emit(s, *str++);
}

static void emit_uint(struct sink *s, unsigned long long v, unsigned base,
                      int upper, int width, char pad)
{
    char tmp[32];
    int n = 0;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";

    if (v == 0)
        tmp[n++] = '0';
    while (v) {
        tmp[n++] = digits[v % base];
        v /= base;
    }
    for (int i = n; i < width; i++)
        emit(s, pad);
    while (n--)
        emit(s, tmp[n]);
}

static void emit_int(struct sink *s, long long v, int width, char pad)
{
    if (v < 0) {
        emit(s, '-');
        emit_uint(s, (unsigned long long)(-v), 10, 0,
                  width > 0 ? width - 1 : 0, pad);
    } else {
        emit_uint(s, (unsigned long long)v, 10, 0, width, pad);
    }
}

int kvsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
    struct sink s = { out, n ? n - 1 : 0, 0 };

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            emit(&s, *fmt);
            continue;
        }
        fmt++;

        char pad = ' ';
        int width = 0, lng = 0, left = 0;

        for (;;) {                       /* flags */
            if (*fmt == '-')      { left = 1; fmt++; }
            else if (*fmt == '0') { pad = '0'; fmt++; }
            else break;
        }
        while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt - '0'); fmt++; }
        while (*fmt == 'l') { lng++; fmt++; }

        switch (*fmt) {
        case 'c':
            emit(&s, (char)va_arg(ap, int));
            break;
        case 's': {
            const char *str = va_arg(ap, const char *);
            if (!str) str = "(null)";
            int len = 0; while (str[len]) len++;
            if (left) {
                emit_str(&s, str);
                for (int i = len; i < width; i++) emit(&s, ' ');
            } else {
                for (int i = len; i < width; i++) emit(&s, pad);
                emit_str(&s, str);
            }
            break;
        }
        case 'd':
        case 'i': {
            long long v = lng ? va_arg(ap, long long) : (long long)va_arg(ap, int);
            emit_int(&s, v, width, pad);
            break;
        }
        case 'u': {
            unsigned long long v = lng ? va_arg(ap, unsigned long long)
                                       : (unsigned long long)va_arg(ap, unsigned);
            emit_uint(&s, v, 10, 0, width, pad);
            break;
        }
        case 'x':
        case 'X': {
            unsigned long long v = lng ? va_arg(ap, unsigned long long)
                                       : (unsigned long long)va_arg(ap, unsigned);
            emit_uint(&s, v, 16, *fmt == 'X', width, pad);
            break;
        }
        case 'p':
            emit(&s, '0'); emit(&s, 'x');
            emit_uint(&s, (unsigned long long)(uintptr_t)va_arg(ap, void *),
                      16, 0, 16, '0');
            break;
        case '%':
            emit(&s, '%');
            break;
        case '\0':
            goto done;
        default:
            emit(&s, '%'); emit(&s, *fmt);
            break;
        }
    }
done:
    if (s.cap || n)                       /* NUL-terminate when there is room */
        out[s.len < s.cap ? s.len : s.cap] = '\0';
    return (int)s.len;
}

int ksnprintf(char *out, size_t n, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int r = kvsnprintf(out, n, fmt, ap);
    va_end(ap);
    return r;
}

/* --- kernel debug console (direct to UART) ----------------------------- */
void kputc(char c) { uart_putc(c); }

void kputs(const char *s) { while (*s) uart_putc(*s++); }

void kprintf(const char *fmt, ...)
{
    char buf[256];
    va_list ap; va_start(ap, fmt);
    kvsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    kputs(buf);
}
