/*
file: task_internal.h
desc: Private shared state between the scheduler (task.c) and IPC (ipc.c).
      Nothing outside kernel/core/ includes this; the public surface is
      task.h + ipc.h.
*/
#ifndef MUKERN_TASK_INTERNAL_H
#define MUKERN_TASK_INTERNAL_H

#include "task.h"
#include "ipc.h"

#define STACK_SIZE (16 * 1024)   /* per-task stack */

enum task_state {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_SENDBLK,   /* blocked in send/call, queued on dst->senders */
    TASK_RECVBLK,   /* blocked in recv, or a caller awaiting a reply */
    TASK_DEAD,
};

struct task {
    void            *sp;          /* saved stack pointer (the context handle) */
    enum task_state  state;
    tid_t            tid;
    char             name[16];

    /* IPC rendezvous state */
    msg_t            msg;         /* staging slot: in-flight or just-received */
    tid_t            recv_from;   /* RECVBLK: TID_ANY or the awaited tid       */
    tid_t            ipc_dst;     /* SENDBLK: who we are sending to            */
    int              call_pending;/* SENDBLK is an RPC awaiting a reply         */
    struct task     *senders;     /* head of tasks SENDBLK'd on us             */
    struct task     *sq_next;     /* link within a senders list                */
};

extern struct task  tasks[MAX_TASKS];
extern struct task *current;

void schedule(void);             /* defined in task.c, used by ipc.c */

#endif /* MUKERN_TASK_INTERNAL_H */
