/*
file: ipc.c
desc: Synchronous message-passing IPC. Blocking rendezvous, no kernel-side
      buffering: a transfer happens only when a sender and a receiver meet.

      Each task owns one staging slot (task->msg). A blocked sender parks its
      outgoing message there; a receiver's incoming message is delivered
      there before it is woken. Senders that find no waiting receiver queue
      themselves on the receiver's `senders` list and sleep.

      call/reply ride on the same machinery: a `call` is a send whose sender
      then transitions to RECVBLK awaiting exactly the callee's reply, so the
      whole RPC is one atomic step from the caller's point of view.
*/
#include <stddef.h>
#include "ipc.h"
#include "task.h"
#include "task_internal.h"

static struct task *T(tid_t id) { return &tasks[id]; }

static int receivable(struct task *dst, tid_t from)
{
    return dst->state == TASK_RECVBLK &&
           (dst->recv_from == TID_ANY || dst->recv_from == from);
}

static void enqueue_sender(struct task *dst, struct task *s)
{
    s->sq_next = NULL;
    if (!dst->senders) {
        dst->senders = s;
    } else {
        struct task *p = dst->senders;
        while (p->sq_next)
            p = p->sq_next;
        p->sq_next = s;
    }
}

/* Pull the first sender matching `from` (TID_ANY = first of any) off our
 * senders list. */
static struct task *dequeue_sender(struct task *me, tid_t from)
{
    struct task *p = me->senders, *prev = NULL;
    while (p) {
        if (from == TID_ANY || p->tid == from) {
            if (prev) prev->sq_next = p->sq_next;
            else      me->senders   = p->sq_next;
            p->sq_next = NULL;
            return p;
        }
        prev = p;
        p = p->sq_next;
    }
    return NULL;
}

/* deliver `m` (from `src`) into dst's slot and mark dst runnable. */
static void deliver(struct task *dst, tid_t src, const msg_t *m)
{
    dst->msg = *m;
    dst->msg.from = src;
    dst->recv_from = TID_NONE;
    dst->state = TASK_READY;
}

int ipc_send(tid_t dst, const msg_t *m)
{
    struct task *me = current, *t = T(dst);

    if (receivable(t, me->tid)) {           /* fast path: receiver waiting */
        deliver(t, me->tid, m);
        return 0;
    }
    me->msg = *m;                           /* slow path: park and sleep   */
    me->ipc_dst = dst;
    me->call_pending = 0;
    me->state = TASK_SENDBLK;
    enqueue_sender(t, me);
    schedule();                             /* wakes once the msg is taken */
    return 0;
}

int ipc_call(tid_t dst, msg_t *m)
{
    struct task *me = current, *t = T(dst);

    if (receivable(t, me->tid)) {           /* fast path */
        deliver(t, me->tid, m);
        me->recv_from = dst;                /* now block for the reply     */
        me->state = TASK_RECVBLK;
        schedule();
        *m = me->msg;
        return 0;
    }
    me->msg = *m;                           /* slow path */
    me->ipc_dst = dst;
    me->call_pending = 1;
    me->state = TASK_SENDBLK;
    enqueue_sender(t, me);
    schedule();                             /* resumes after reply lands   */
    *m = me->msg;
    return 0;
}

int ipc_recv(tid_t from, msg_t *m)
{
    struct task *me = current;
    struct task *s = dequeue_sender(me, from);

    if (s) {                                /* a sender was already waiting */
        *m = s->msg;
        m->from = s->tid;
        if (s->call_pending) {
            s->recv_from = me->tid;         /* caller now awaits our reply */
            s->state = TASK_RECVBLK;
        } else {
            s->state = TASK_READY;          /* plain send completes        */
        }
        return 0;
    }
    me->recv_from = from;                   /* nobody waiting: sleep       */
    me->state = TASK_RECVBLK;
    schedule();
    *m = me->msg;
    return 0;
}

int ipc_reply(tid_t dst, const msg_t *m)
{
    struct task *me = current, *t = T(dst);

    if (!(t->state == TASK_RECVBLK && t->recv_from == me->tid))
        return -1;                          /* dst isn't awaiting our reply */
    deliver(t, me->tid, m);
    return 0;
}
