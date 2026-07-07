/* servers/hello/main.c - the "Hello World" user process (M4b; M5a output). */
#include "usys.h"

int main(void)
{
    struct ipc_msg go;
    u_recv(PM_TID, &go);                 /* PM delegates a console cap, then releases us */
    con_linef("Hello World -- a user process spawned via PM (tid=",
              (unsigned long)u_gettid(), ")\n");
    return 0;
}
