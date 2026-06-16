/*
file: kernel/srv/kbd.c
desc: PS/2 keyboard driver server (x86_64 only, built where HAVE_KBD is
      defined). The input counterpart of the VGA server: it owns the i8042
      keyboard and speaks con.h. Together they make a usable LOCAL console --
      type on the keyboard, see it on the VGA screen -- with no serial line
      involved.

      It answers two input ops:
        CON_POLL  non-blocking; reply the next ASCII byte or -1 if none.
        CON_GETC  blocking; poll-and-yield until a key is pressed.
      The console multiplexer uses CON_POLL so it can watch the keyboard and
      the serial line simultaneously.
*/
#include <stddef.h>
#include "ipc.h"
#include "con.h"
#include "task.h"
#include "hal.h"

void kbd_server(void *arg)
{
    (void)arg;
    kbd_init();

    for (;;) {
        msg_t m;
        ipc_recv(TID_ANY, &m);

        switch (m.tag) {
        case CON_POLL:
            m.w[0] = (uint64_t)kbd_getc();        /* -1 stays -1 */
            break;

        case CON_GETC: {
            int c;
            while ((c = kbd_getc()) < 0)
                task_yield();
            m.w[0] = (uint64_t)(unsigned)c;
            break;
        }

        default:                                  /* output ops: no screen */
            m.w[0] = (uint64_t)-1;
            break;
        }

        ipc_reply(m.from, &m);
    }
}
