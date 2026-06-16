/*
file: con.c
desc: Client side of the con.h protocol. Thin wrappers that turn a console
      operation into an ipc_call() to a server tid. The shell uses these and
      never touches hardware -- it does not even know which devices exist.
*/
#include <stdarg.h>
#include "con.h"
#include "ipc.h"
#include "klib.h"

void con_putc(tid_t srv, char c)
{
    msg_t m = { .tag = CON_PUTC };
    m.w[0] = (uint64_t)(unsigned char)c;
    ipc_call(srv, &m);
}

void con_puts(tid_t srv, const char *s)
{
    msg_t m = { .tag = CON_PUTS, .ptr = (void *)s, .len = strlen(s) };
    ipc_call(srv, &m);
}

int con_getc(tid_t srv)
{
    msg_t m = { .tag = CON_GETC };
    ipc_call(srv, &m);
    return (int)m.w[0];
}

void con_clear(tid_t srv)
{
    msg_t m = { .tag = CON_CLEAR };
    ipc_call(srv, &m);
}

void con_printf(tid_t srv, const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    con_puts(srv, buf);
}
