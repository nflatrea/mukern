/* kernel/sched.c - preemptive round-robin scheduler (M2c, extended in M3).
 *
 * Mechanism only: a fixed thread table plus an assembly context switch. The
 * timer ISR calls schedule() on each tick; IPC code may also yield by
 * blocking a thread and calling schedule() with interrupts disabled. The
 * bootstrap kernel context is the idle thread, run only when nothing else
 * is runnable.
 */
#include "sched.h"
#include "hal.h"

#define KSTACK_SIZE 16384

extern void context_switch(uint64_t *old_rsp, uint64_t new_rsp);
extern uint64_t arch_init_stack(uint64_t stack_top, thread_fn fn, void *arg);

static struct thread  threads[SCHED_MAX_THREADS];
static struct thread  idle;
static struct thread *current = &idle;

static uint8_t stacks[SCHED_MAX_THREADS][KSTACK_SIZE] __attribute__((aligned(16)));

void sched_init(void)
{
    for (int i = 0; i < SCHED_MAX_THREADS; i++) {
        threads[i].state       = T_UNUSED;
        threads[i].id          = i;
        threads[i].ipc_call_to = -1;
    }
    idle.state       = T_RUNNING;
    idle.id          = -1;
    idle.ipc_call_to = -1;
    current    = &idle;
}

int thread_create(thread_fn fn, void *arg)
{
    for (int i = 0; i < SCHED_MAX_THREADS; i++) {
        if (threads[i].state != T_UNUSED && threads[i].state != T_DEAD)
            continue;                       /* reclaim exited/killed slots */

        uint64_t top = (uint64_t)&stacks[i][KSTACK_SIZE];
        threads[i].rsp        = arch_init_stack(top, fn, arg);
        threads[i].kstack_top = top;
        threads[i].state      = T_RUNNABLE;
        threads[i].ipc_state  = IPC_NONE;
        threads[i].ipc_peer    = IPC_ANY;
        threads[i].ipc_call_to = -1;
        threads[i].irq_pending = 0;
        cap_clear(&threads[i]);
        return i;
    }
    return -1;
}

void schedule(void)
{
    struct thread *prev = current;
    struct thread *next = 0;
    int from = (prev == &idle) ? 0 : prev->id + 1;

    for (int k = 0; k < SCHED_MAX_THREADS; k++) {
        struct thread *t = &threads[(from + k) % SCHED_MAX_THREADS];
        if (t->state == T_RUNNABLE) {
            next = t;
            break;
        }
    }

    if (!next)
        next = (prev == &idle) ? 0 : &idle;
    if (!next || next == prev)
        return;

    if (prev->state == T_RUNNING && prev != &idle)
        prev->state = T_RUNNABLE;
    if (next != &idle)
        next->state = T_RUNNING;

    arch_set_kernel_stack(next->kstack_top);    /* traps from Ring 3 -> next's kstack */
    current = next;
    context_switch(&prev->rsp, next->rsp);
}

void thread_exit(void)
{
    hal_irq_disable();
    current->state = T_DEAD;
    hal_irq_enable();
    for (;;)
        hal_halt();
}

struct thread *thread_current(void)      { return current; }
struct thread *thread_by_id(int id)
{
    if (id < 0 || id >= SCHED_MAX_THREADS)
        return 0;
    return &threads[id];
}
struct thread *thread_slot(int i)        { return &threads[i]; }
void thread_block(struct thread *t)      { t->state = T_BLOCKED; }
void thread_wake(struct thread *t)       { if (t->state == T_BLOCKED) t->state = T_RUNNABLE; }
