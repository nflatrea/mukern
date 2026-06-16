/*
file: kernel/srv/serial.c
desc: Serial (UART) driver server. A task that owns the UART and is reached
      only through the con.h IPC protocol -- the kernel core never touches
      the serial port again (except the boot banner and panic(), which run
      outside the scheduler).

      This is the heart of the microkernel idea: a device driver is just an
      ordinary scheduled task that happens to execute privileged port I/O.
      Move it, restart it, or run it in user mode later (milestone M7) and
      nothing else changes -- its clients only ever see con.h messages.

      Server loop: receive a request from anyone, service it against the
      hardware, reply to the caller. CON_GETC has no data to return until a
      key is pressed, so we poll uart_getc() and yield between polls; under
      cooperative scheduling that lets every other task keep running while we
      wait. (Interrupt-driven input is a later milestone.)
*/
#include <stddef.h>
#include "ipc.h"
#include "con.h"
#include "task.h"
#include "hal.h"

void serial_server(void *arg)
{
    (void)arg;
    uart_init();

    for (;;) {
        msg_t m;
        ipc_recv(TID_ANY, &m);

        switch (m.tag) {
        case CON_PUTC:
            uart_putc((char)m.w[0]);
            break;

        case CON_PUTS: {
            const char *s = (const char *)m.ptr;
            for (uint64_t i = 0; i < m.len; i++)
                uart_putc(s[i]);
            break;
        }

        case CON_GETC: {
            int c;
            while ((c = uart_getc()) < 0)
                task_yield();         /* nothing yet: let others run */
            m.w[0] = (uint64_t)(unsigned)c;
            break;
        }

        case CON_POLL:                /* non-blocking: -1 when no byte ready */
            m.w[0] = (uint64_t)uart_getc();
            break;

        case CON_CLEAR:
            /* a serial line has no screen to clear; succeed silently */
            break;

        default:
            break;
        }

        ipc_reply(m.from, &m);
    }
}
