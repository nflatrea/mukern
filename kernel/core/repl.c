/* 
file: repl.c
*/

#include <stddef.h>
#include "console.h"
#include "arch.h"
#include "klib.h"

#define LINE_MAX 128

/* if s starts with prefix, return pointer past it, else NULL */
static const char *after(const char *s, const char *prefix)
{
    while (*prefix) {
        if (*s != *prefix)
            return NULL;
        s++;
        prefix++;
    }
    return s;
}

static int readline(char *buf)
{
    size_t n = 0;
    for (;;) {
        int c = console_getc();
        if (c < 0)
            continue;                 /* poll */
        if (c == '\r' || c == '\n') {
            console_putc('\n');
            break;
        }
        if (c == 0x7f || c == 0x08) { /* DEL / BS */
            if (n) {
                n--;
                kputs("\b \b");
            }
            continue;
        }
        if (c == '\t')
            c = ' ';
        if (c >= 0x20 && c < 0x7f && n < (size_t)(LINE_MAX - 1)) {
            buf[n++] = (char)c;
            console_putc((char)c);    /* echo */
        }
    }
    buf[n] = '\0';
    return (int)n;
}

void repl(void)
{
    char line[LINE_MAX];
    const char *rest;

    kputs("type 'help' for commands\n\n");

    for (;;) {
        kputs("mukern> ");
        if (readline(line) == 0)
            continue;

        if (strcmp(line, "help") == 0) {
            kputs("commands:\n");
            kputs("  help          this list\n");
            kputs("  arch          print architecture\n");
            kputs("  echo <text>   print text\n");
            kputs("  fault         raise a CPU exception (trap demo)\n");
            kputs("  panic         call panic() directly\n");
            kputs("  halt          stop the cpu\n");
            kputs("  exit          breaks REPL loop\n");
                                    
        } else if (strcmp(line, "exit") == 0) {
            break;
        } else if (strcmp(line, "arch") == 0) {
            kprintf("%s\n", arch_name());
        } else if (strcmp(line, "echo") == 0) {
            kputc('\n');
        } else if ((rest = after(line, "echo ")) != NULL) {
            kprintf("%s\n", rest);
        } else if (strcmp(line, "fault") == 0) {
            kputs("triggering a CPU exception...\n");
            arch_trigger_fault();     /* should not return */
            kputs("(returned from fault? trap path is broken)\n");
        } else if (strcmp(line, "panic") == 0) {
            panic("requested from REPL");
        } else if (strcmp(line, "halt") == 0) {
            kputs("halting.\n");
            arch_halt();
        } else {
            kprintf("unknown command: %s  (try 'help')\n", line);
        }
    }
}
