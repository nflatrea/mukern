/* kernel/ipc.c - synchronous rendezvous IPC (M3a; extended for user IPC M4).
 *
 * No kernel buffering (DESIGN): a sender and receiver must meet. If the peer
 * is already waiting, the message is copied across immediately; otherwise the
 * caller blocks (T_BLOCKED) and yields. The IPC critical section runs with
 * interrupts disabled, and schedule() is always entered with IF=0.
 *
 * The *_locked variants assume the caller already disabled interrupts (the
 * syscall and page-fault paths run inside an interrupt gate); they never
 * re-enable IF, leaving that to the eventual iretq.
 */
#include "ipc.h"
#include "sched.h"
#include "hal.h"

/* Find a thread blocked trying to SEND to `me`, matching `want` (or ANY). */
static struct thread *find_waiting_sender(struct thread *me, int want)
{
    for (int i = 0; i < SCHED_MAX_THREADS; i++) {
        struct thread *t = thread_slot(i);
        if (t->state == T_BLOCKED && t->ipc_state == IPC_SENDING &&
            t->ipc_peer == me->id &&
            (want == IPC_ANY || want == t->id))
            return t;
    }
    return 0;
}

/* Is `dst` blocked waiting to RECEIVE from `from` (or ANY)? */
static int receiver_ready(struct thread *dst, int from)
{
    return dst->state == T_BLOCKED && dst->ipc_state == IPC_RECEIVING &&
           (dst->ipc_peer == IPC_ANY || dst->ipc_peer == from);
}

int ipc_send_locked(struct thread *self, int dest_tid, const struct ipc_msg *msg)
{
    struct thread *dst = thread_by_id(dest_tid);
    if (!dst || dst->state == T_UNUSED || dst->state == T_DEAD)
        return -1;

    if (receiver_ready(dst, self->id)) {        /* rendezvous now */
        dst->ipc_buf   = *msg;
        dst->ipc_from  = self->id;
        dst->ipc_state = IPC_NONE;
        thread_wake(dst);
        return 0;
    }

    /* Receiver not ready: stage the message and block. */
    self->ipc_buf   = *msg;
    self->ipc_state = IPC_SENDING;
    self->ipc_peer  = dest_tid;
    thread_block(self);
    schedule();                                 /* resumes once received */

    self->ipc_state = IPC_NONE;
    return 0;
}

int ipc_recv_locked(struct thread *self, int src_tid, struct ipc_msg *out)
{
    /* A registered IRQ that fired takes priority so devices are drained
     * promptly; it is delivered as a synthetic message with no real sender. */
    if (self->irq_pending) {
        self->irq_pending--;
        out->label = MSG_IRQ;
        return IPC_IRQ_SENDER;
    }

    struct thread *snd = find_waiting_sender(self, src_tid);
    if (snd) {                                  /* sender already waiting */
        *out = snd->ipc_buf;
        int from = snd->id;
        snd->ipc_state = IPC_NONE;
        thread_wake(snd);
        return from;
    }

    /* No sender: block until one delivers into our buffer, or an IRQ wakes us. */
    self->ipc_state = IPC_RECEIVING;
    self->ipc_peer  = src_tid;
    thread_block(self);
    schedule();                                 /* resumes on delivery/IRQ */

    /* A sender clears ipc_state when it delivers; if it is still RECEIVING we
     * were woken by an IRQ notification instead. */
    if (self->ipc_state == IPC_RECEIVING) {
        self->ipc_state = IPC_NONE;
        if (self->irq_pending)
            self->irq_pending--;
        out->label = MSG_IRQ;
        return IPC_IRQ_SENDER;
    }

    *out = self->ipc_buf;
    self->ipc_state = IPC_NONE;
    return self->ipc_from;
}

int ipc_send(int dest_tid, const struct ipc_msg *msg)
{
    hal_irq_disable();
    int r = ipc_send_locked(thread_current(), dest_tid, msg);
    hal_irq_enable();
    return r;
}

int ipc_recv(int src_tid, struct ipc_msg *out)
{
    hal_irq_disable();
    int r = ipc_recv_locked(thread_current(), src_tid, out);
    hal_irq_enable();
    return r;
}
