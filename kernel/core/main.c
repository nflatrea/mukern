/*
file: main.c
desc: Portable kernel entry. Its whole job is to make the three kernel
      services usable and then get out of the way: bring up the emergency
      UART console, install traps, start the task subsystem, create the init
      task, and hand control to the scheduler. From sched_start() on, the
      kernel only ever runs because some task invoked IPC or yielded -- all
      policy (drivers, console, shell) lives in tasks.
*/
#include "task.h"
#include "arch.h"
#include "hal.h"
#include "klib.h"
#include "banner.h"

void init_task(void *arg);   /* kernel/core/init.c */

void kmain(void)
{
    uart_init();             /* emergency console for boot + panic           */
    arch_traps_init();       /* M1: catch CPU exceptions, don't triple-fault */

    /* Early sign of life, straight to the UART (no scheduler/IPC yet). */
    kprintf("muKern %s booting on %s\n", MUKERN_VERSION, arch_name());

    task_init();                                  /* M2: tasks + scheduler   */
    task_create(init_task, NULL, "init");         /* M5: root task           */
    sched_start();                                /* never returns           */
}
