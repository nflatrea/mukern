/* include/cap.h - capability table + capability-checked IPC (M3b).
 *
 * Per DESIGN: access to a resource (here, the right to message another
 * thread's endpoint) is granted only by holding an unforgeable capability in
 * the thread's table. There are no global namespaces; a thread that lacks a
 * capability cannot send.
 */
#pragma once
#include "types.h"
#include "ipc.h"

#define CAP_MAX 16

/* Capability types. */
enum {
    CAP_NULL = 0,
    CAP_ENDPOINT,               /* right to send to a thread's endpoint */
    CAP_IOPORT,                 /* right to access an I/O port range    */
};

/* Access rights. */
#define CAP_RIGHT_SEND (1u << 0)

struct cap {
    int      type;              /* CAP_NULL / CAP_ENDPOINT / CAP_IOPORT */
    int      target;            /* endpoint = target thread id          */
    uint32_t rights;            /* CAP_RIGHT_*                          */
    uint16_t io_base;           /* ioport = first port in the range     */
    uint16_t io_len;            /* ioport = number of ports             */
};

struct thread;                  /* forward decl (defined in sched.h) */

void cap_clear(struct thread *t);
int  cap_install(struct thread *t, int slot, int target, uint32_t rights);

/* Grant an I/O-port range to a thread (installs a CAP_IOPORT). Returns 0, or
 * -1 if the table is full. */
int  cap_grant_io(struct thread *t, uint16_t base, uint16_t len);

/* Does thread `t` hold a CAP_IOPORT covering `port`? */
int  cap_has_port(struct thread *t, uint16_t port);

/* Endpoint (IPC send) capabilities. */
int  cap_grant_endpoint(struct thread *t, int target);
int  cap_has_endpoint(struct thread *t, int target);

/* Capability-checked send: resolve current thread's cap[slot] and, if it
 * grants send rights, forward via ipc_send. Returns 0, or -1 if denied. */
int  ipc_send_cap(int slot, const struct ipc_msg *msg);
