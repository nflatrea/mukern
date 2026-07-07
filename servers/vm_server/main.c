/* servers/vm_server/main.c - external page-fault handler / pager (M4c;
 * M5a output via console driver). */
#include "usys.h"

#define INIT_TID 0

/* Policy: faults are only honoured inside the demand-paging heap window; any
 * access outside it is treated as a wild pointer and the process is killed. */
#define HEAP_LO 0x0000400000400000ull
#define HEAP_HI 0x0000400000410000ull

int main(void)
{
    u_set_pager();
    struct ipc_msg ready = { .label = MSG_READY };
    u_send(INIT_TID, &ready);
    struct ipc_msg go; u_recv(INIT_TID, &go);   /* wait until init grants caps */
    con_linef("[vm] pager registered (tid=", (unsigned long)u_gettid(), ")\n");

    for (;;) {
        struct ipc_msg m;
        long from = u_recv(-1, &m);
        if (m.label == MSG_FAULT) {
            unsigned long addr = m.data1;
            if (addr >= HEAP_LO && addr < HEAP_HI) {
                con_linef("[vm] fault tid=", m.data0, "");
                con_linex(" addr=", addr, " -> mapped\n");
                u_map((long)addr);
                struct ipc_msg ok = { .label = MSG_REPLY, .data0 = 0 };
                u_send(from, &ok);
            } else {
                con_linef("[vm] ILLEGAL fault tid=", m.data0, "");
                con_linex(" addr=", addr, " -> killing process\n");
                struct ipc_msg kill = { .label = MSG_REPLY, .data0 = 1 };
                u_send(from, &kill);
            }
        }
    }
    return 0;
}
