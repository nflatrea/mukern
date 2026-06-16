/*
file: kernel/srv/console.c
desc: Console multiplexer server. This is the single endpoint the shell talks
      to (svc_console). It implements the whole con.h protocol by fanning
      requests out to the underlying device servers:

        output (PUTC/PUTS/CLEAR) -> serial, and also vga when one exists
        input  (GETC)            -> whichever of serial / keyboard types first

      The shell writes once and lands on every screen, and reads once and
      hears every keyboard -- exactly the "console" abstraction a microkernel
      keeps OUT of the kernel. The call graph

          shell -> console -> { serial, vga, kbd }

      is acyclic, so the synchronous rendezvous can never deadlock: each
      server only calls servers "below" it and replies back up.

      Input multiplexing is the interesting part. A blocking read can only
      wait on one source, so instead the console POLLs each input device in
      turn (a non-blocking op that returns -1 when nothing is ready) and
      yields between sweeps. Whichever device produces a byte first wins.
      That is how the same shell is driven from a serial terminal AND a local
      PS/2 keyboard at once.

      Because every task shares one address space, a PUTS is forwarded by
      passing the original (ptr,len) straight through -- no copy, one message
      per device.
*/
#include <stddef.h>
#include "ipc.h"
#include "con.h"
#include "task.h"
#include "services.h"

/* forward one already-received message to a device server and wait for it */
static void forward(tid_t dev, const msg_t *in)
{
    if (dev == TID_NONE)
        return;
    msg_t m = *in;
    ipc_call(dev, &m);
}

/* non-blocking read from one input device: byte, or -1 if none/absent */
static int poll_dev(tid_t dev)
{
    if (dev == TID_NONE)
        return -1;
    msg_t m = { .tag = CON_POLL };
    ipc_call(dev, &m);
    return (int)m.w[0];
}

void console_server(void *arg)
{
    (void)arg;

    for (;;) {
        msg_t m;
        ipc_recv(TID_ANY, &m);

        switch (m.tag) {
        case CON_PUTC:
        case CON_PUTS:
        case CON_CLEAR:
            forward(svc_serial, &m);
            forward(svc_vga, &m);          /* skipped when TID_NONE */
            break;

        case CON_GETC: {
            int c;
            for (;;) {                     /* watch every input source */
                c = poll_dev(svc_serial);
                if (c >= 0) break;
                c = poll_dev(svc_kbd);
                if (c >= 0) break;
                task_yield();
            }
            m.w[0] = (uint64_t)(unsigned)c;
            break;
        }

        default:
            break;
        }

        ipc_reply(m.from, &m);
    }
}
