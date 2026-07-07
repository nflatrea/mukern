/* servers/pm/main.c - Process Manager (M4b; M5a output via console driver). */
#include "usys.h"

int main(void)
{
    struct ipc_msg ready = { .label = MSG_READY };
    u_send(0, &ready);
    struct ipc_msg go; u_recv(0, &go);
    con_linef("[pm] process manager up (tid=", (unsigned long)u_gettid(), ")\n");

    for (;;) {
        struct ipc_msg m;
        long from = u_recv(-1, &m);
        if (m.label == MSG_SPAWN) {
            long tid = u_spawn((long)m.data0);
            if (tid >= 0) {
                u_grant_cap(tid, CON_TID);          /* child may print */
                struct ipc_msg cgo = { .label = MSG_REPLY };
                u_send(tid, &cgo);                  /* release the child */
            }
            struct ipc_msg r = { .label = MSG_REPLY, .data0 = (unsigned long)tid };
            u_send(from, &r);
        }
    }
    return 0;
}
