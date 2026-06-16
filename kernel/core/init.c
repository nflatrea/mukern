/*
file: kernel/core/init.c
desc: The init (root) task -- the first thing the scheduler runs. In a
      microkernel almost nothing lives in the kernel; init is the user-level
      glue that brings the system up: it starts the driver servers, starts
      the console multiplexer, publishes their endpoints, and finally starts
      the shell. After that its job is done and it exits.

      The three svc_* globals ARE the system's name service. With one address
      space a "name server" is overkill, so the registry collapses to these
      published tids that everyone reads through services.h.
*/
#include <stddef.h>
#include "task.h"
#include "ipc.h"
#include "con.h"
#include "services.h"
#include "arch.h"
#include "banner.h"

/* The service registry, declared extern in services.h, defined here. */
tid_t svc_serial  = TID_NONE;
tid_t svc_vga     = TID_NONE;
tid_t svc_kbd     = TID_NONE;
tid_t svc_console = TID_NONE;

/* server entry points (kernel/srv/) */
void serial_server(void *);
void console_server(void *);
#ifdef HAVE_VGA
void vga_server(void *);
#endif
#ifdef HAVE_KBD
void kbd_server(void *);
#endif

/* the shell, now an ordinary task (kernel/core/repl.c) */
void repl(void *);

void init_task(void *arg)
{
    (void)arg;

    /* Bring up the device servers first so their endpoints exist before any
     * client could call them. */
    svc_serial = task_create(serial_server, NULL, "serial");
#ifdef HAVE_VGA
    svc_vga = task_create(vga_server, NULL, "vga");
#endif
#ifdef HAVE_KBD
    svc_kbd = task_create(kbd_server, NULL, "kbd");
#endif
    svc_console = task_create(console_server, NULL, "console");

    /* Re-emit the banner through the console so it lands on every device,
     * including the VGA screen (the boot-time UART banner never reached it). */
    con_clear(svc_console);
    con_puts(svc_console, MUKERN_MOTD);
    con_printf(svc_console, "arch: %s\n\n", arch_name());

    /* Hand the terminal to the shell. */
    task_create(repl, NULL, "shell");

    task_exit();
}
