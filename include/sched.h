/* include/sched.h - kernel threads + preemptive scheduler (M2c, extended M3).
 *
 * struct thread is exposed (rather than opaque) because the IPC and
 * capability code in the microkernel core operates directly on thread
 * state. This is the small, deliberate coupling of a microkernel's TCB.
 */
#pragma once
#include "types.h"
#include "ipc.h"
#include "cap.h"

#define SCHED_MAX_THREADS 16

typedef void (*thread_fn)(void *arg);

enum {
    T_UNUSED = 0,
    T_RUNNABLE,
    T_RUNNING,
    T_BLOCKED,                  /* waiting on IPC */
    T_DEAD,
};

struct thread {
    uint64_t rsp;               /* saved stack pointer (context_switch)   */
    int      state;
    int      id;
    uint64_t kstack_top;        /* top of this thread's kernel stack      */

    /* IPC rendezvous state (M3a). */
    int            ipc_state;   /* IPC_NONE / IPC_SENDING / IPC_RECEIVING */
    int            ipc_peer;
    int            ipc_call_to;    /* outstanding call target (reply auth) */    /* tid being communicated with, or IPC_ANY*/
    int            ipc_from;    /* tid of the sender that delivered (M4)  */
    int            irq_pending; /* undelivered IRQ notifications (H5)      */
    struct ipc_msg ipc_buf;     /* staged message                         */

    /* Capability table (M3b). */
    struct cap     ctable[CAP_MAX];
};

void sched_init(void);
int  thread_create(thread_fn fn, void *arg);
void schedule(void);                         /* must be entered with IF=0  */
void thread_exit(void);

/* Accessors used by the IPC / capability core. */
struct thread *thread_current(void);
struct thread *thread_by_id(int id);
struct thread *thread_slot(int i);           /* 0..SCHED_MAX_THREADS-1     */
void           thread_block(struct thread *t);
void           thread_wake(struct thread *t);

/* Arch hook: point the CPU's privilege-0 stack at this thread's kernel stack
 * (x86_64 TSS.rsp0). Called by schedule() so traps from Ring 3 always land on
 * the running thread's own kernel stack. */
void arch_set_kernel_stack(uint64_t rsp0);
