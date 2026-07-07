/* servers/app/main.c - a user program that reads a file via the FS server (M5c).
 *
 * Demonstrates the whole stack: app -> fs_server -> block driver -> disk, with
 * every component a user process talking over IPC. The kernel only routes
 * messages and faults.
 */
#include "usys.h"

int main(void)
{
    const char *name = "hello.txt";

    struct ipc_msg om = { .label = MSG_OPEN, .data0 = (unsigned long)name };
    struct ipc_msg orep;
    u_send(FS_TID, &om);
    u_recv(FS_TID, &orep);
    long handle = (long)orep.data0;
    if (handle < 0) {
        con_puts("[app] open(\"hello.txt\") failed\n");
        return 1;
    }
    con_linef("[app] open(\"hello.txt\") -> handle=", (unsigned long)handle, "");
    con_linef(" size=", orep.data1, "\n");

    static unsigned char fbuf[1024];
    struct ipc_msg rm = { .label = MSG_FREAD, .data0 = (unsigned long)handle,
                          .data1 = (unsigned long)fbuf };
    struct ipc_msg rrep;
    u_send(FS_TID, &rm);
    u_recv(FS_TID, &rrep);

    con_puts("[app] read() contents: ");
    con_write((const char *)fbuf, (unsigned)rrep.data0);
    return 0;
}
