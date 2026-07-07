/* kernel/cap.c - capability table management + capability-checked IPC (M3b). */
#include "cap.h"
#include "sched.h"

void cap_clear(struct thread *t)
{
    for (int i = 0; i < CAP_MAX; i++) {
        t->ctable[i].type    = CAP_NULL;
        t->ctable[i].target  = -1;
        t->ctable[i].rights  = 0;
        t->ctable[i].io_base = 0;
        t->ctable[i].io_len  = 0;
    }
}

int cap_grant_io(struct thread *t, uint16_t base, uint16_t len)
{
    if (!t)
        return -1;
    for (int i = 0; i < CAP_MAX; i++) {
        if (t->ctable[i].type == CAP_NULL) {
            t->ctable[i].type    = CAP_IOPORT;
            t->ctable[i].io_base = base;
            t->ctable[i].io_len  = len;
            return 0;
        }
    }
    return -1;                              /* table full */
}

int cap_has_port(struct thread *t, uint16_t port)
{
    if (!t)
        return 0;
    for (int i = 0; i < CAP_MAX; i++) {
        struct cap *c = &t->ctable[i];
        if (c->type == CAP_IOPORT &&
            port >= c->io_base && port < (uint16_t)(c->io_base + c->io_len))
            return 1;
    }
    return 0;
}

int cap_grant_endpoint(struct thread *t, int target)
{
    if (!t)
        return -1;
    if (cap_has_endpoint(t, target))            /* idempotent */
        return 0;
    for (int i = 0; i < CAP_MAX; i++) {
        if (t->ctable[i].type == CAP_NULL) {
            t->ctable[i].type   = CAP_ENDPOINT;
            t->ctable[i].target = target;
            t->ctable[i].rights = CAP_RIGHT_SEND;
            return 0;
        }
    }
    return -1;
}

int cap_has_endpoint(struct thread *t, int target)
{
    if (!t)
        return 0;
    for (int i = 0; i < CAP_MAX; i++)
        if (t->ctable[i].type == CAP_ENDPOINT && t->ctable[i].target == target)
            return 1;
    return 0;
}

int cap_install(struct thread *t, int slot, int target, uint32_t rights)
{
    if (!t || slot < 0 || slot >= CAP_MAX)
        return -1;
    t->ctable[slot].type   = CAP_ENDPOINT;
    t->ctable[slot].target = target;
    t->ctable[slot].rights = rights;
    return 0;
}

int ipc_send_cap(int slot, const struct ipc_msg *msg)
{
    struct thread *self = thread_current();
    if (slot < 0 || slot >= CAP_MAX)
        return -1;

    struct cap *c = &self->ctable[slot];
    if (c->type != CAP_ENDPOINT || !(c->rights & CAP_RIGHT_SEND))
        return -1;                          /* access denied */

    return ipc_send(c->target, msg);
}
