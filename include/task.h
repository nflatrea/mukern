/*
file: task.h
desc: Tasks and the scheduler -- the second kernel service (with IPC).

      A task is a thread of control with its own stack and saved context.
      Scheduling is cooperative round-robin: a task runs until it blocks in
      IPC or calls task_yield(). That is exactly the scheduling discipline a
      synchronous-IPC microkernel wants -- control naturally follows the
      message. (Timer-driven preemption is a later milestone; see ROADMAP.)

      There is no heap yet: tasks and their stacks come from fixed arrays,
      which keeps the whole subsystem allocation-free and easy to audit.
*/
#ifndef MUKERN_TASK_H
#define MUKERN_TASK_H

typedef int tid_t;

#define MAX_TASKS  16
#define TID_NONE  (-1)   /* "no task"                                       */
#define TID_ANY   (-2)   /* ipc_recv: accept a message from anyone          */

/* Create a runnable task. Returns its tid, or TID_NONE if the table is full.
 * The task starts at entry(arg) the next time it is scheduled. */
tid_t task_create(void (*entry)(void *), void *arg, const char *name);

/* Cooperatively give up the CPU; the task stays runnable. */
void task_yield(void);

/* Terminate the calling task. Never returns. */
void task_exit(void) __attribute__((noreturn));

/* tid of the calling task. */
tid_t task_self(void);

/* Human-readable state table, dumped by the shell's `ps` command, written
 * through the supplied character sink so it can go out over IPC. */
void task_dump(void (*putc)(char));

/* Bring up the task subsystem (call once, before creating tasks). */
void task_init(void);

/* Hand the CPU to the scheduler. The boot context is abandoned and the first
 * runnable task takes over. Never returns. */
void sched_start(void) __attribute__((noreturn));

#endif /* MUKERN_TASK_H */
