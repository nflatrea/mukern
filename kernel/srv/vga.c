/*
file: kernel/srv/vga.c
desc: VGA text driver server (x86_64 only, built where HAVE_VGA is defined).
      The screen counterpart of the serial server: it owns the VGA text
      buffer and speaks con.h. It is output-only -- a CON_GETC would have no
      keyboard behind it here, so the console multiplexer routes input to the
      serial server instead.

      Spawned by init only on arches with a framebuffer; elsewhere svc_vga
      stays TID_NONE and the console server simply never calls it.
*/
#include <stddef.h>
#include "ipc.h"
#include "con.h"
#include "task.h"
#include "hal.h"

void vga_server(void *arg)
{
    (void)arg;
    vga_init();

    for (;;) {
        msg_t m;
        ipc_recv(TID_ANY, &m);

        switch (m.tag) {
        case CON_PUTC:
            vga_putc((char)m.w[0]);
            break;

        case CON_PUTS: {
            const char *s = (const char *)m.ptr;
            for (uint64_t i = 0; i < m.len; i++)
                vga_putc(s[i]);
            break;
        }

        case CON_CLEAR:
            vga_clear();
            break;

        case CON_GETC:        /* no input device on the screen side */
            m.w[0] = (uint64_t)-1;
            break;

        default:
            break;
        }

        ipc_reply(m.from, &m);
    }
}
