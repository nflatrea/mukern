/*
file: task.c
desc: Tasks + cooperative round-robin scheduler. One of the kernel's two
      services (the other is IPC). The state machine here is shared with
      ipc.c, which is why the task struct and a couple of helpers are exposed
      through the private header task_internal.h.

      Scheduling discipline: a task runs until it blocks in IPC or yields.
      pick_next() always finds *something* runnable because task 0 is a
      never-blocking idle task, so the scheduler can never wedge.
*/
#include <stddef.h>
#include <stdint.h>
#include "task.h"
#include "task_internal.h"
#include "arch.h"
#include "klib.h"

struct task tasks[MAX_TASKS];
struct task *current;

static uint8_t stacks[MAX_TASKS][STACK_SIZE]
    __attribute__((aligned(16)));

static const char *state_name(enum task_state s)
{
    switch (s) {
    case TASK_UNUSED:  return "unused";
    case TASK_READY:   return "ready";
    case TASK_RUNNING: return "run";
    case TASK_SENDBLK: return "send-wait";
    case TASK_RECVBLK: return "recv-wait";
    case TASK_DEAD:    return "dead";
    }
    return "?";
}

/* idle task: never blocks, so the run queue is never empty. */
static void idle_entry(void *arg)
{
    (void)arg;
    for (;;) {
        arch_relax();
        task_yield();
    }
}

void task_init(void)
{
    for (int i = 0; i < MAX_TASKS; i++)
        tasks[i].state = TASK_UNUSED;
    /* task 0 is idle */
    tid_t id = task_create(idle_entry, NULL, "idle");
    (void)id;        /* always 0 */
    current = &tasks[0];
}

tid_t task_create(void (*entry)(void *), void *arg, const char *name)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        struct task *t = &tasks[i];
        if (t->state != TASK_UNUSED)
            continue;

        t->tid          = i;
        t->state        = TASK_READY;
        t->recv_from    = TID_NONE;
        t->ipc_dst      = TID_NONE;
        t->call_pending = 0;
        t->sq_next      = NULL;
        t->senders      = NULL;

        int k = 0;
        if (name)
            for (; name[k] && k < (int)sizeof(t->name) - 1; k++)
                t->name[k] = name[k];
        t->name[k] = '\0';

        t->sp = arch_ctx_init(&stacks[i][STACK_SIZE], entry, arg);
        return i;
    }
    return TID_NONE;
}

/* round-robin: first READY task after current, else current if it may run,
 * else idle (task 0, which is always READY-or-RUNNING). */
static struct task *pick_next(void)
{
    int base = current ? current->tid : 0;
    for (int n = 1; n <= MAX_TASKS; n++) {
        struct task *t = &tasks[(base + n) % MAX_TASKS];
        if (t->state == TASK_READY)
            return t;
    }
    if (current && (current->state == TASK_RUNNING ||
                    current->state == TASK_READY))
        return current;
    return &tasks[0];     /* idle */
}

/* The scheduler proper: pick a task and switch to it. Callers that intend to
 * block must set current->state before calling. */
void schedule(void)
{
    struct task *prev = current;
    struct task *next = pick_next();

    if (next == prev) {
        if (prev->state != TASK_RUNNING)
            prev->state = TASK_RUNNING;   /* nobody else: keep running */
        return;
    }
    if (prev->state == TASK_RUNNING)
        prev->state = TASK_READY;
    next->state = TASK_RUNNING;
    current = next;
    arch_switch(&prev->sp, next->sp);
    /* resumes here when `prev` is scheduled again */
}

void task_yield(void)
{
    schedule();
}

tid_t task_self(void)
{
    return current->tid;
}

void task_exit(void)
{
    current->state = TASK_DEAD;
    schedule();
    panic("task_exit: scheduled a dead task");
}

void task_dump(void (*putc)(char))
{
    char line[64];
    const char *hdr = " tid  state      name\n";
    for (const char *p = hdr; *p; p++) putc(*p);
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED)
            continue;
        int n = ksnprintf(line, sizeof line, " %3d  %-9s  %s%s\n",
                          tasks[i].tid, state_name(tasks[i].state),
                          tasks[i].name,
                          &tasks[i] == current ? "  <= self" : "");
        for (int k = 0; k < n && k < (int)sizeof line; k++)
            putc(line[k]);
    }
}

void sched_start(void)
{
    struct task *first = pick_next();
    current = first;
    first->state = TASK_RUNNING;

    /* The boot context is abandoned; give arch_switch a throwaway slot to
     * spill it into and never come back. */
    static void *boot_sp;
    arch_switch(&boot_sp, first->sp);
    panic("sched_start returned");
}
