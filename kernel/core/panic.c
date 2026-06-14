/*
file: panic.c 
*/
#include "klib.h"
#include "console.h"

void panic(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    kprintf("\n*** KERNEL PANIC: ");
    kvprintf(fmt, ap);
    va_end(ap);
    kprintf(" ***\n");
    arch_halt();
}
