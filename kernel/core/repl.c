/*
file: repl.c
desc: The shell -- now an ordinary task, not kernel code. It speaks only the
      con.h protocol to the console server (svc_console) and so has no idea
      whether its characters touch a UART, a VGA screen, or both. Every
      keystroke it reads and every line it prints is a round trip of
      synchronous IPC: shell -> console -> {serial, vga}. That is the whole
      microkernel claim made concrete -- the shell is a client of servers,
      with zero hardware knowledge.
*/
#include <stddef.h>
#include "task.h"
#include "ipc.h"
#include "con.h"
#include "services.h"
#include "arch.h"
#include "klib.h"

#define LINE_MAX 128

/* character sink for task_dump(): routes each byte to the console server. */
static void con_sink(char c)
{
    con_putc(svc_console, c);
}

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
        int c = con_getc(svc_console);
        if (c < 0)
            continue;
        if (c == '\r' || c == '\n') {
            con_putc(svc_console, '\n');
            break;
        }
        if (c == 0x7f || c == 0x08) {     /* DEL / BS */
            if (n) {
                n--;
                con_puts(svc_console, "\b \b");
            }
            continue;
        }
        if (c == '\t')
            c = ' ';
        if (c >= 0x20 && c < 0x7f && n < (size_t)(LINE_MAX - 1)) {
            buf[n++] = (char)c;
            con_putc(svc_console, (char)c);   /* echo */
        }
    }
    buf[n] = '\0';
    return (int)n;
}

void repl(void *arg)
{
    (void)arg;
    char line[LINE_MAX];
    const char *rest;

    con_puts(svc_console, "type 'help' for commands\n\n");

    for (;;) {
        con_puts(svc_console, "mukern> ");
        if (readline(line) == 0)
            continue;

        if (strcmp(line, "help") == 0) {
            con_puts(svc_console,
                "commands:\n"
                "  help          this list\n"
                "  arch          print architecture\n"
                "  echo <text>   print text\n"
                "  ps            list tasks and their states\n"
                "  clear         clear the screen\n"
                "  fault         raise a CPU exception (trap demo)\n"
                "  panic         call panic() directly\n"
                "  halt          stop the cpu\n"
                "  exit          terminate the shell task\n");
        } else if (strcmp(line, "exit") == 0) {
            con_puts(svc_console, "shell exiting; system idle.\n");
            break;
        } else if (strcmp(line, "arch") == 0) {
            con_printf(svc_console, "%s\n", arch_name());
        } else if (strcmp(line, "ps") == 0) {
            task_dump(con_sink);
        } else if (strcmp(line, "clear") == 0) {
            con_clear(svc_console);
        } else if (strcmp(line, "echo") == 0) {
            con_putc(svc_console, '\n');
        } else if ((rest = after(line, "echo ")) != NULL) {
            con_printf(svc_console, "%s\n", rest);
        } else if (strcmp(line, "fault") == 0) {
            con_puts(svc_console, "triggering a CPU exception...\n");
            arch_trigger_fault();             /* should not return */
            con_puts(svc_console, "(returned from fault? trap path broken)\n");
        } else if (strcmp(line, "panic") == 0) {
            panic("requested from shell");
        } else if (strcmp(line, "halt") == 0) {
            con_puts(svc_console, "halting.\n");
            arch_halt();
        } else {
            con_printf(svc_console, "unknown command: %s  (try 'help')\n", line);
        }
    }

    task_exit();
}
