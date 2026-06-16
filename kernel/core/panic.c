/*
file: panic.c
desc: Last-resort error path. Prints straight to the UART (never via IPC --
      the thing that broke might BE a server) and parks the CPU.
*/
#include <stdarg.h>
#include "klib.h"
#include "arch.h"

void panic(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    kvsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    kputs("\n*** KERNEL PANIC: ");
    kputs(buf);
    kputs(" ***\n");
    arch_halt();
}
